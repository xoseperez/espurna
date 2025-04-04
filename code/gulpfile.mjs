/*

ESP8266 file system builder

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2024 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

// -----------------------------------------------------------------------------
// Dependencies
// -----------------------------------------------------------------------------

import {
    dest as orig_dest,
    series,
    parallel,
    src,
} from 'gulp';

import { inlineSource } from 'inline-source';
import { build as esbuildBuild } from 'esbuild';
import { minify as htmlMinify } from 'html-minifier-terser';
import { JSDOM } from 'jsdom';

import * as convert from 'convert-source-map';
import log from 'fancy-log';

import { Transform } from 'node:stream';
import { pipeline } from 'node:stream/promises';

import * as fs from 'node:fs';
import * as http from 'node:http';
import * as path from 'node:path';
import * as zlib from 'node:zlib';

import { stat as fsStat } from 'node:fs/promises';

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

/**
 * @import { default as File } from 'vinyl'
 */

/**
 * module names used internally, usually as element class-name
 * @typedef {{[k: string]: boolean}} Modules
 */

/**
 * name mapping for output filename, when specific module is used
 * @typedef {{[k: string]: string}} NamedBuild
 */

/**
 * declare `MODULE_${NAME}` boolean consts in the source, allowing esbuild to strip unused code
 * @typedef {{[k: string]: string}} Defines
 */

/**
 * helper functions that deal with 'module' elements
 * @typedef {function(JSDOM): boolean} HtmlModify
 */

/**
 * resulting blob compression type
 * @typedef {'br' | 'gz'} Compression
 */

/**
 * build pipeline common options
 * @typedef BuildOptions
 * @property {string} name
 * @property {boolean} minify
 * @property {Compression} compress
 * @property {Modules} modules
 */

/**
 * shared pipeline file stats, set up to figure out 'latest' mtime for the resulting file
 * @typedef {{[k: string]: fs.Stats}} BuildStats
 */

/**
 * declare some modules as optional, only to be included for specific builds
 * @constant
 * @type Modules
 */
const DEFAULT_MODULES = {
    'api': true,
    'cmd': true,
    'curtain': false,
    'dbg': true,
    'dcz': true,
    'garland': false,
    'ha': true,
    'idb': true,
    'led': true,
    'light': false,
    'lightfox': false,
    'local': false,
    'mqtt': true,
    'nofuss': true,
    'ntp': true,
    'ota': true,
    'relay': true,
    'rfb': false,
    'rfm69': false,
    'rpn': true,
    'sch': true,
    'sns': false,
    'thermostat': false,
    'tspk': true,
};

/**
 * special type of build when multiple single-module files are used
 * currently, only possible way to combine both (besides modifying the targets manually)
 * @constant
 * @type Modules
 */
const MODULES_ALL = Object.fromEntries(
    Object.entries(DEFAULT_MODULES).map(
        ([key, _]) => {
            if ('local' === key) {
                return [key, false];
            }

            return [key, true];
        }));

/**
 * used for the locally served .html, that is already merged but not yet inlined
 * @constant
 * @type Modules
 */
const MODULES_LOCAL =
    Object.assign({}, MODULES_ALL, {local: true});

/**
 * generic output, usually this includes a single module
 * @constant
 * @type {NamedBuild}
 */
const NAMED_BUILD = {
    'curtain': 'curtain',
    'garland': 'garland',
    'light': 'light',
    'lightfox': 'lightfox',
    'rfbridge': 'rfb',
    'rfm69': 'rfm69',
    'sensor': 'sns',
    'thermostat': 'thermostat',
};

// vendored sources from node_modules/ need explicit paths
const NODE_DIR = path.join('node_modules');

// importmap manifest for dev server. atm, explicit list
// TODO import.meta.resolve wants umd output for some reason
const IMPORT_MAP = {
    '@jaames/iro': '/@jaames/iro/dist/iro.es.js',
};

// output .html w/ inline sourcemaps (for development only)
// output .html.{gz,br}, cleaned-up for firmware use
const BUILD_DIR = path.join('html', 'build');

// input sources, making sure relative inline paths start from here
const SRC_DIR = path.join('html', 'src');

// spec aka test files, make sure only these are used when running tests
const SPEC_DIR = path.join('html', 'spec');

// main source file used by inline-source
const ENTRYPOINT = path.join(SRC_DIR, 'index.html')

// .ipp compiled from the .html.{br,gz}, providing static u8[] for the firmware to use
const STATIC_DIR = path.join('espurna', 'static');

// -----------------------------------------------------------------------------
// Build
// -----------------------------------------------------------------------------

/**
 * build pipeline usually works with file inputs and its transformations
 * main function to compose all these separate tasks is `stream.pipeline`
 * - https://nodejs.org/api/stream.html#streampipelinesource-transforms-destination-options
 *
 * transformations most commonly happen on vynil-fs `File` objects, which means `{objectMode: true}`
 * should always be set for any new stream; per https://nodejs.org/api/stream.html#object-mode
 * > All streams created by Node.js APIs operate exclusively on strings, <Buffer>, <TypedArray> and <DataView> objects
 * (nb. async generator functions do not seem to care, though, only stream Writable/Transform classes)
 *
 * while gulp documentation recommends using `src.pipe(dst)` as a general pattern, and error handling
 * *may* just work correctly for 'streamx'-originated readers / writers... it breaks for 'node:stream'
 * - https://github.com/gulpjs/gulp/issues/359
 * - https://github.com/gulpjs/gulp/issues/2812
 * - https://github.com/gulpjs/gulp/issues/2812#issuecomment-2445451930
 *
 * also note that any `Error` in the writer / transformer function has to pass through the 'callback(...)'
 * otherwise, exception would be raised from the pipeline internal task and its origin may not be obvious
 */

const ERR_CONTENTS_TYPE =
    new Error('expecting source contents to be a buffer!');

const ERR_EMPTY =
    new Error('source contents cannot be empty');

const ERR_EMPTY_BUNDLE =
    new Error('js bundle cannot be empty');

/**
 * after destination finishes, log everything written so far
 * @param {string} dstdir)
 */
function dest(dstdir) {
    const out = orig_dest(dstdir);

    out.on('data', data);
    out.on('finish', finish);

    let name = '';
    let size = 0;

    /** @param {File} source */
    function data(source) {
        name = path.relative('.', source.path);
        if (source.isBuffer()) {
            size = source.contents.length;
        }
    };

    function finish() {
        if (!name || !size) {
            return;
        }

        if (name.startsWith(BUILD_DIR)) {
            log(`${name}: ${size} bytes`);
        } else {
            log(`written ${name}`);
        }
    }

    return out;
}

/**
 * @param {import("html-minifier-terser").Options} options
 * @returns {Transform}
 */
function toMinifiedHtml(options) {
    return new Transform({
        objectMode: true,
        async transform(source, _, callback) {
            if (!source.contents) {
                callback(ERR_EMPTY);
                return;
            }

            const contents = source.contents.toString();
            const minified = await htmlMinify(contents, options);

            source.contents = Buffer.from(minified);

            callback(null, source);
        }});
}

const ERR_COMPRESSION =
    new Error('unknown compression type');

/**
 * @param {BuildOptions} options
 * @returns {Transform}
 */
function toCompressed(options) {
    return new Transform({
        objectMode: true,
        transform(/** @type {File} */source, _, callback) {
            if (!(source.contents instanceof Buffer)) {
                callback(ERR_CONTENTS_TYPE);
                return;
            }

            if (!options.compress) {
                callback(null, source.contents);
                return;
            }

            /**
             * gzip inserts an OS-dependant byte in the header, ref.
             * - https://datatracker.ietf.org/doc/html/rfc1952
             * - https://github.com/nodejs/node/blob/e46c680bf2b211bbd52cf959ca17ee98c7f657f5/deps/zlib/deflate.c#L901
             * - windowBits description in the https://zlib.net/manual.html#Advanced
             *
             * python3.14+ sets this to 255 (0xff), so is this handler
             * - https://github.com/python/cpython/pull/120486
             * - https://github.com/python/cpython/commit/08d09cf5ba041c9c5c3860200b56bab66fd44a23
             *
             * additionally, set mtime to 0 (in case it ever comes up)
             * - https://github.com/python/cpython/pull/125261
             * - https://github.com/python/cpython/commit/dcd58c50844dae0d83517e88518a677914ea594b
             *
             * @param {Buffer} buf
             * @returns {Buffer}
             */
            function normalize(buf) {
                if (options.compress === 'gz') {
                    // mtime
                    buf[4] = 0;
                    buf[5] = 0;
                    buf[6] = 0;
                    buf[7] = 0;
                    // os
                    buf[9] = 0xff;
                }

                return buf;
            }

            /** @type {zlib.CompressCallback} */
            function compress_callback(error, result) {
                if (!error) {
                    source.contents = normalize(result);
                    source.extname += `.${options.compress}`;
                    callback(null, source);
                } else {
                    callback(error);
                }
            };

            switch (options.compress) {
            case 'br':
                zlib.brotliCompress(
                    source.contents.buffer,
                    {params: {
                        [zlib.constants.BROTLI_PARAM_QUALITY]: zlib.constants.BROTLI_MAX_QUALITY}
                    },
                    compress_callback);
                break;

            case 'gz':
                zlib.gzip(
                    source.contents.buffer,
                    {level: zlib.constants.Z_BEST_COMPRESSION},
                    compress_callback);
                break;

            default:
                callback(ERR_COMPRESSION);
                break;
            }
        }});
}

/**
 * @param {Buffer} buffer
 * @param {number} every
 * @returns {string}
 */
function formatBufferLines(buffer, every = 20) {
    /** @type {string[]} */
    let lines = [];

    for (let i = 0; i < buffer.length; i += every) {
        lines.push(
            Array.from(buffer.subarray(i, i + every))
                .map((x) => '0x' + x.toString(16).padStart(2, '0'))
                .join(','));
    }

    return lines.join(',\n');
}

/**
 * @param {File} file
 * @returns {string}
 */
function formatLastModified(file) {
    return (file?.stat?.mtime ?? (new Date())).toUTCString()
}

/**
 * @param {Compression} compress
 * @returns {string}
 */
function formatContentEncoding(compress) {
    switch (compress) {
    case 'br':
        return 'br';

    case 'gz':
        return 'gzip';

    default:
        throw ERR_COMPRESSION;
    }
}

/**
 * generates c++-friendly output from the stream contents
 * @param {BuildOptions} options
 * @returns {Transform}
 */
function toOutput(options) {
    return new Transform({
        objectMode: true,
        transform(/** @type {File} */source, _, callback) {
            if (!(source.contents instanceof Buffer)) {
                callback(ERR_CONTENTS_TYPE);
                return;
            }

            // make sure to include both type and data
            const output = [
                '#pragma once',
                '#include <sys/pgmspace.h>',
                '#include <cstdint>',
                `alignas(4) static constexpr char webui_last_modified[] PROGMEM = "${formatLastModified(source)}";`,
                `alignas(4) static constexpr char webui_content_encoding[] PROGMEM = "${formatContentEncoding(options.compress)}";`,
                'alignas(4) static constexpr uint8_t webui_data[] PROGMEM = {',
                formatBufferLines(source.contents),
                '};\n'
            ];

            // resulting file ext hides the compression option
            const extname = '.ipp';

            // replace source stream with a different one, also replacing contents
            const dest = source.clone({contents: false});

            dest.contents = Buffer.from(output.join('\n'));
            switch (dest.extname) {
            case `.${options.compress}`:
                dest.extname = extname;
                break;

            default:
                dest.extname += extname;
                break;
            }

            callback(null, dest);
        }});
}

/**
 * @param {BuildStats} stats
 * @returns {Transform}
 */
function trackFileStats(stats) {
    return new Transform({
        objectMode: true,
        transform(source, _, callback) {
            if (source.stat) {
                stats[path.relative('.', source.path)] = source.stat;
            }

            callback(null, source);
        }});
}

/**
 * by default, destination preserves stat.*time of the source. which is obviosly bogus here as gulp
 * only knows about the entrypoint and not about every include happenning through inline-source
 * @param {BuildStats} stats
 * @returns {Transform}
 */
function adjustFileStats(stats) {
    return new Transform({
        objectMode: true,
        transform(source, _, callback) {
            const values = Object.values(stats);
            if (values.length) {
                const latest = Object.values(stats)
                    .reduce((prev, x) => {
                        if (prev.mtime.valueOf() > x.mtime.valueOf()) {
                            return prev;
                        }

                        return x;
                    });

                source.stat = latest;
            }

            callback(null, source);
        }});
}

/**
 * updates source filename to a different one
 * @param {string} name
 * @returns {Transform}
 */
function rename(name) {
    return new Transform({
        objectMode: true,
        transform(source, _, callback) {
            const out = source.clone({contents: false});

            out.path = path.join(out.base, name);
            if (out.sourceMap) {
                out.sourceMap.file = out.relative;
            }

            callback(null, out);
        }});
}

/**
 * ref. https://github.com/evanw/esbuild/issues/1895
 * from our side, html/src/*.mjs (with the exception of index.mjs) require 'init()' call to be actually set up and used
 * as the result, no code from the module should be bundled into the output when module was not initialized
 * however, since light module depends on iro.js and does not have `sideEffects: false` in package.json, it would still get bundled because of top-level import
 * (...and since module modifying something in global scope is not unheard of...)
 * @returns {import("esbuild").Plugin}
 */
function forceNoSideEffects() {
    return {
        name: 'no-side-effects',
        setup(build) {
            build.onResolve({filter: /@jaames\/iro/, namespace: 'file'},
                async ({path, ...options}) => {
                    const result = await build.resolve(path, {...options, namespace: 'noRecurse'});
                    return {...result, sideEffects: false};
                });
        },
    };
}

/**
 * ref. html/src/index.mjs
 * TODO exportable values, e.g. in build.mjs? right now, false-positive of undeclared values, plus see 'forceNoSideEffects()'
 * @param {Modules} modules
 * @returns {Defines}
 */
function makeDefine(modules) {
    return Object.fromEntries(
        Object.entries(modules).map(
            ([key, value]) => {
                return [`MODULE_${key.toUpperCase()}`, value.toString()];
            }));
}

/**
 * @param {string} sourcefile
 * @param {string} contents
 * @param {string} resolveDir
 * @param {Defines} define
 * @param {boolean} minify
 */
async function inlineJavascriptBundle(sourcefile, contents, resolveDir, define, minify) {
    return await esbuildBuild({
        stdin: {
            contents,
            loader: 'js',
            resolveDir,
            sourcefile,
        },
        format: 'esm',
        bundle: true,
        plugins: [
            forceNoSideEffects(),
        ],
        define,
        minify,
        sourcemap: minify
            ? 'inline'
            : undefined,
        platform: minify
            ? 'browser'
            : 'neutral',
        external: minify
            ? undefined
            : ['./*.mjs'],
        write: false,
    });
}

/**
 * @param {string} srcdir
 * @param {BuildStats} stats
 * @param {BuildOptions} options
 * @returns {import("inline-source").Handler}
 */
function inlineHandler(srcdir, stats, options) {
    return async function(source) {
        // TODO split handlers
        if (source.content) {
            return;
        }

        if (typeof source.sourcepath === 'string') {
            const srcpath = path.isAbsolute(source.sourcepath)
                ? path.relative(srcdir, source.sourcepath)
                : path.normalize(path.join(srcdir, source.sourcepath));
            stats[srcpath] = await fsStat(srcpath);
        }

        // specific elements can be excluded at this point
        // (although, could be handled by jsdom afterwards; top elem does not usually have classList w/ module)
        const source_module = source.props.module;
        if (typeof source_module === 'string') {
            for (let module of source_module.split(',')) {
                if (!options.modules[module]) {
                    source.content = '';
                    source.replace = '<div></div>';
                    return;
                }
            }
        }

        // main entrypoint of the app, usually a script bundle
        if (source.sourcepath && typeof source.sourcepath === 'string' && source.format === 'mjs') {
            const define = makeDefine(options.modules);

            const result = await inlineJavascriptBundle(
                source.sourcepath,
                source.fileContent,
                srcdir, define, options.minify);
            if (!result.outputFiles.length) {
                throw ERR_EMPTY_BUNDLE;
            }

            let content = Buffer.from(result.outputFiles[0].contents);

            if (!options.minify) {
                let prepend = '';
                for (const [key, value] of Object.entries(define)) {
                    prepend += `const ${key} = ${value};\n`;
                }

                content = Buffer.concat([
                    Buffer.from(prepend), content]);
            }

            source.content = content.toString();
            return;
        }

        // <object type=text/html>. not handled by inline-source directly, only image blobs are expected
        if (source.props.raw) {
            source.content = source.fileContent;
            source.replace = source.content.toString();
            source.format = 'text';
            return;
        }

        // TODO import svg icon?
        if (source.sourcepath === 'favicon.ico') {
            source.format = 'x-icon';
            return;
        }
    };
}

/**
 * @param {HtmlModify[]} handlers
 * @returns {Transform}
 */
function modifyHtml(handlers) {
    return new Transform({
        objectMode: true,
        transform(source, _, callback) {
            if (!(source.contents instanceof Buffer)) {
                callback(ERR_CONTENTS_TYPE);
                return;
            }

            const dom = new JSDOM(source.contents, {includeNodeLocations: true});

            let changed = false;

            for (let handler of handlers) {
                if (handler(dom)) {
                    changed = true;
                }
            }

            if (changed) {
                source.contents = Buffer.from(dom.serialize());
            }

            callback(null, source);
        }});
}

/**
 * when minification is enabled, make sure sourcemaps are not written to the resulting file
 * @returns {HtmlModify}
 */
function dropSourcemap() {
    return function(dom) {
        const scripts = dom.window.document.getElementsByTagName('script');
        for (let script of scripts) {
            if (script.getAttribute('type') === 'importmap') {
                continue;
            }

            if (!script.textContent) {
                continue;
            }

            script.textContent =
                convert.removeMapFileComments(script.textContent);

            return true;
        }

        return false;
    }
}

/**
 * optionally inject external libs paths
 * @param {boolean} compress
 * @returns {HtmlModify}
 */
function injectVendor(compress) {
    return function(dom) {
        if (compress) {
            return false;
        }

        const script = dom.window.document.getElementsByTagName('script');

        const importmap = dom.window.document.createElement('script');
        importmap.setAttribute('type', 'importmap');
        importmap.textContent = JSON.stringify({imports: IMPORT_MAP});

        const head = dom.window.document.getElementsByTagName('head')[0];
        head.insertBefore(importmap, script[0]);

        return true;
    }
}

/**
 * generally a good idea to mitigate "tab-napping" attacks
 * per https://www.chromestatus.com/feature/6140064063029248
 * @returns {HtmlModify}
 */
function externalBlank() {
    return function(dom) {
        let changed = false;

        for (let elem of dom.window.document.getElementsByTagName('a')) {
            if (elem.href.startsWith('http')) {
                elem.setAttribute('target', '_blank');
                elem.setAttribute('rel', 'noopener');
                elem.setAttribute('tabindex', '-1');
                changed = true;
            }
        }

        return changed;
    }
}

/**
 * with an explicit list of modules, strip anything not used by the bundle
 * @param {Modules} modules
 * @returns {HtmlModify}
 */
function stripModules(modules) {
    return function(dom) {
        let changed = false;

        for (const [module, value] of Object.entries(modules)) {
            if (value) {
                continue;
            }

            const className = `module-${module}`;
            for (let elem of dom.window.document.getElementsByClassName(className)) {
                elem.classList.remove(className);

                let remove = true;
                for (let name of elem.classList) {
                    if (name.startsWith('module-')) {
                        remove = false;
                        break;
                    }
                }

                if (remove) {
                    elem.parentElement?.removeChild(elem);
                    changed = true;
                }
            }
        }

        return changed;
    }
}

/**
 * inline every external resource in the entrypoint.
 * works outside of gulp context, so used sources are only known after this is actually called
 * @param {string} srcdir
 * @param {BuildStats} stats
 * @param {BuildOptions} options
 * @returns {Transform}
 */
function makeInlineSource(srcdir, stats, options) {
    return new Transform({
        objectMode: true,
        async transform(source, _, callback) {
            if (!source.contents) {
                callback(ERR_EMPTY);
                return;
            }

            const contents = await inlineSource(
                source.contents.toString(),
                {
                    'compress': options.minify,
                    'handlers': [inlineHandler(srcdir, stats, options)],
                    'rootpath': srcdir,
                });

            source.contents = Buffer.from(contents);
            callback(null, source);
        }});
}

/**
 * @param {string} lhs
 * @param {string} rhs
 * @returns {Transform}
 */
function replace(lhs, rhs) {
    return new Transform({
        objectMode: true,
        transform(source, _, callback) {
            if (!(source.contents instanceof Buffer)) {
                callback(ERR_CONTENTS_TYPE);
                return;
            }

            const before = source.contents.toString();
            source.contents = Buffer.from(before.replaceAll(lhs, rhs));

            callback(null, source);
        }});
}

/**
 * @param {string} name
 * @returns {Modules}
 */
function makeModules(name) {
    switch (name) {
    case 'all':
        return MODULES_ALL;

    case 'local':
        return MODULES_LOCAL;

    case 'small':
        return DEFAULT_MODULES;
    }

    if (NAMED_BUILD[name] === undefined) {
        throw new Error(`NAMED_BUILD['${name}'] is missing`);
    }

    const out = Object.assign({}, DEFAULT_MODULES);
    out[NAMED_BUILD[name]] = true;

    return out;
}

/**
 * @param {BuildOptions} options
 */
function buildHtml(options) {
    /** @type {BuildStats} */
    const stats = {};

    const out = [
        src(ENTRYPOINT),
        trackFileStats(stats),
        makeInlineSource(SRC_DIR, stats, options),
        modifyHtml([
            injectVendor(options.minify),
            stripModules(options.modules),
            externalBlank(),
        ]),
        adjustFileStats(stats),
    ];

    if (options.minify) {
        out.push(
            toMinifiedHtml({
                collapseWhitespace: true,
                removeComments: true,
                minifyCSS: true,
                minifyJS: false
            }),
            replace('pure-', 'p-'));
    }

    return out;
}

/**
 * @param {BuildOptions} options
 */
function buildOutputs(options) {
    const out = [
        rename(`index.${options.name}.html`),
        dest(BUILD_DIR),
        modifyHtml([
            dropSourcemap(),
        ]),
    ];

    if (options.compress) {
        out.push(
            toCompressed(options),
            dest(BUILD_DIR));
    }

    out.push(
        toOutput(options),
        dest(STATIC_DIR));

    return out;
}

/**
 * @param {string} name
 */
function buildWebUI(name) {
    /** @type {BuildOptions} */
    const opts = {
        compress: 'gz',
        minify: true,
        modules: makeModules(name),
        name: name,
    };

    return pipeline([
        ...buildHtml(opts),
        ...buildOutputs(opts),
    ]);
}

/**
 * @param {string} name
 */
function serveWebUI(name) {
    const server = http.createServer();

    /** @param {any} e */
    function log_error(e) {
        if (e instanceof Error) {
            log.error(e.message);
        } else {
            log.error(e);
        }
    }

    /**
     * @param {http.ServerResponse<http.IncomingMessage>} response
     * @param {string} path
     */
    async function responseJsFile(response, path) {
        const reader = fs.createReadStream(path);

        // by default, status 200 would be sent out w/ the data
        response.setHeader('Content-Type', 'application/javascript');

        // assume that 'readable' already sent status and headers, error handler only cares about 'open'
        reader.once('error', (e) => {
            if (('syscall' in e) && (typeof e.syscall === 'string') && e.syscall === 'open') {
                const outer = e;

                try {
                    response.writeHead(404, {
                        'Content-Type': 'text/plain',
                    });
                    response.end('not found');
                } catch (e) {
                    log_error(outer);
                    log_error(e);
                }
            } else {
                log_error(e);
            }
        });

        try {
            await pipeline(reader, response);
        } catch (e) {
            log_error(e);
        }
    }

    /**
     * @param {http.ServerResponse<http.IncomingMessage>} response
     */
    async function responseIndex(response) {
        response.setHeader('Content-Type', 'text/html');

        try {
            await pipeline(
                /** @ts-ignore, types/node/stream/promises/pipeline.d.ts hates 'args' / '...args' */
                ...buildHtml({modules: makeModules(name), compress: false, minify: false}),
                // convert the original vinyl-fs stream back into something nodejs understands
                async function* (/** @type {Transform} */source) {
                    for await (const chunk of source) {
                        if (('contents' in chunk) && chunk.contents instanceof Buffer) {
                            yield chunk.contents;
                        } else {
                            throw ERR_CONTENTS_TYPE;
                        }
                    }
                },
                response
            );
        } catch (e) {
            log_error(e);
        }
    }

    server.on('request', async (request, response) => {
        const url = new URL(`http://localhost${request.url}`);

        // serve bundled html as-is, do not minify
        switch (url.pathname) {
        case '/':
        case '/index.htm':
        case '/index.html':
            await responseIndex(response);
            return;
        }

        // when module files need browser repl. note the bundling scope,
        // only this way modules are actually modules and not inlined

        // external libs should be searched in node_modules/
        for (let value of Object.values(IMPORT_MAP)) {
            if (value === url.pathname) {
                await responseJsFile(response, path.join(NODE_DIR, value));
                return;
            }
        }

        // everything else is attempted as html/src/${module}.mjs
        if (url.pathname.endsWith('.mjs')) {
            const tail = url.pathname.split('/').at(-1);
            if (tail !== undefined) {
                await responseJsFile(response, path.join(SRC_DIR, tail));
                return;
            }
        }

        response.writeHead(500, {'content-type': 'text/plain'});
        response.end('500');
    });

    server.on('listening', () => {
        log.info(`Serving ${SRC_DIR} index and *.mjs at`, server.address());
    });

    server.listen(8080, 'localhost');
}

// -----------------------------------------------------------------------------
// Source code validation
// -----------------------------------------------------------------------------

/**
 * map input pattern to paths, do not populate source.contents
 * @param {string | string[]} pattern
 */
function sourcePath(pattern) {
    return [
        src(pattern, {read: false, buffer: false}),
        new Transform({
            objectMode: true,
            transform(source, _, callback) {
                callback(null, source.path);
            }}),
    ];
}

// .spec.mjs vitest tests
export async function vitest() {
    return pipeline(
        /** @ts-ignore, types/node/stream/promises/pipeline.d.ts hates 'args' / '...args' */
        ...sourcePath([
            `${SPEC_DIR}/*.mjs`,
        ]),
        async function* (/** @type {AsyncIterable<string>} */source) {
            // ref. 'vitest/node' parseVitestCLI('vitest --environment jsdom --dir html/spec --run')
            const opts = {
                /** @type {string[]} */
                filter: [],
                options: {
                    '--': [],
                    color: true,
                    environment: 'jsdom',
                    dir: SPEC_DIR,
                    run: true,
                }
            };

            for await (const chunk of source) {
                opts.filter.push(chunk);
            }

            yield opts;
        },
        async function* (/** @type {AsyncIterable<any>} */source) {
            const { startVitest } = await import('vitest/node');
            for await (const opts of source) {
                const runner = await startVitest('test', opts.filter, opts.options);
                await runner.close();
                break;
            }
        }
    );
}

// Generic javascript linting. *Could* happen at inline stage, but only without compression / minification
export async function eslint() {
    const { ESLint } = await import('eslint');

    const runner = new ESLint({});
    const format = await runner.loadFormatter('stylish');

    return pipeline([
        ...sourcePath([
            'gulpfile.mjs',
            `${SRC_DIR}/*.mjs`,
            `${SPEC_DIR}/*.mjs`,
        ]),
        new Transform({
            objectMode: true,
            async transform(path, _, callback) {
                const results = await runner.lintFiles([path]);
                const resultText = await format.format(results);

                if (resultText.length) {
                    log(resultText);
                }

                const errorCount =
                    results.filter((x) => x.errorCount > 0)
                    .length > 0;
                if (errorCount) {
                    callback(new Error(`eslint: ${path} failed`));
                    return;
                }

                callback(null);
            }}),
    ]);
}

// Validate all HTML sources. *Cannot* happen at inline stage, since JSDOM modifications break some style rules
async function html_validate() {
    const {
        FileSystemConfigLoader,
        HtmlValidate,
        formatterFactory,
    } = await import('html-validate');

    const html = new HtmlValidate(new FileSystemConfigLoader());
    const format = formatterFactory('stylish');

    return pipeline([
        ...sourcePath('html/src/*.html'),
        new Transform({
            objectMode: true,
            async transform(path, _, callback) {
                const report = await html.validateFile(path);
                if (!report.valid) {
                    log(format(report.results));
                    callback(new Error(`html-validate: ${path} failed`));
                    return;
                }

                callback(null);
            }}),
    ]);
}

export { html_validate as 'html-validate' };

// -----------------------------------------------------------------------------
// Tasks
// -----------------------------------------------------------------------------

export function dev() {
    return serveWebUI('local');
}

export function serve() {
    return dev();
}

export function webui_all() {
    return buildWebUI('all');
}

export function webui_small() {
    return buildWebUI('small');
}

export function webui_curtain() {
    return buildWebUI('curtain');
}

export function webui_garland() {
    return buildWebUI('garland');
}

export function webui_light() {
    return buildWebUI('light');
}

export function webui_lightfox() {
    return buildWebUI('lightfox');
}

export function webui_rfbridge() {
    return buildWebUI('rfbridge');
}

export function webui_rfm69() {
    return buildWebUI('rfm69');
}

export function webui_sensor() {
    return buildWebUI('sensor');
}

export function webui_thermostat() {
    return buildWebUI('thermostat');
}

export const webui =
    parallel(
        webui_all,
        webui_small,
        webui_curtain,
        webui_garland,
        webui_light,
        webui_lightfox,
        webui_rfbridge,
        webui_rfm69,
        webui_sensor,
        webui_thermostat);

export const test =
    parallel(
        eslint,
        html_validate,
        vitest);

export default
    series(test, webui);
