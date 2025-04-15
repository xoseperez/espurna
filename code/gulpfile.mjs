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

import { rollup } from 'rollup';
import { default as rollupEsbuild } from 'rollup-plugin-esbuild';
import { default as rollupVirtual } from '@rollup/plugin-virtual';
import { default as rollupAlias } from '@rollup/plugin-alias';

import { minify as htmlMinify } from 'html-minifier-terser';
import { JSDOM } from 'jsdom';

import * as convert from 'convert-source-map';
import log from 'fancy-log';

import { Transform } from 'node:stream';
import { escape as queryEscape } from 'node:querystring';
import { parseArgs } from 'node:util';
import { pipeline } from 'node:stream/promises';

import * as fs from 'node:fs';
import * as http from 'node:http';
import * as path from 'node:path';
import * as url from 'node:url';
import * as zlib from 'node:zlib';

import {
    maybeInline,
    needElement,
    stripModules as stripModulesImpl,
} from './html/inline.mjs';

import {
    MODULE_PRESETS,
    MODULE_DEV,
    build as buildPresets,
    makeModules,
} from './html/preset.mjs';

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
 * helper functions that deal with 'module' elements
 * @typedef {function(JSDOM): (boolean | Promise<boolean>)} HtmlModify
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

// directory where this file is located
export const ROOT = path.dirname(url.fileURLToPath(import.meta.url));

// vendored sources from node_modules/ need explicit paths
export const NODE_DIR = path.join(ROOT, 'node_modules');

// ui sources root
export const HTML_DIR = path.join(ROOT, 'html');

// build preset environment files
export const PRESET_DIR = path.join(HTML_DIR, 'preset');

// output .html w/ inline sourcemaps (for development only)
// output .html.{gz,br}, cleaned-up for firmware use
export const BUILD_DIR = path.join(HTML_DIR, 'build');

// vendored sources, usually injected as-is without any minification or compression
export const VENDOR_DIR = path.join(HTML_DIR, 'vendor');

// input sources, making sure relative inline paths start from here
export const SRC_DIR = path.join(HTML_DIR, 'src');

// spec aka test files, make sure only these are used when running tests
export const SPEC_DIR = path.join(HTML_DIR, 'spec');

// main source file used by inline-source
export const ENTRYPOINT = path.join(HTML_DIR, 'index.html')

// .ipp compiled from the .html.{br,gz}, providing static u8[] for the firmware to use
export const STATIC_DIR = path.join(ROOT, 'espurna', 'static');

// importmap manifest for dev server. atm, explicit overrides list
// based on known locations for the MODULE_DEV preset
export const IMPORT_MAP = {
    '@jaames/iro': path.join(NODE_DIR, '/@jaames/iro/dist/iro.es.js'),
    '@build-preset/constants.mjs': path.join(PRESET_DIR, MODULE_DEV, 'constants.mjs'),
};

// files relevant to the build
export const BUILD_SCRIPTS = [
    'eslint.config.mjs',
    'gulpfile.mjs',
    'vite.config.mjs',
    'vitest.config.mjs',
    `${HTML_DIR}/*.mjs`,
    `${PRESET_DIR}/**/*.mjs`,
];

// files relevant to the vitest
export const TEST_SCRIPTS = [
    `${SPEC_DIR}/**/*.mjs`,
];

// files relevant to the webui
export const SOURCE_SCRIPTS = [
    `${SRC_DIR}/**/*.mjs`,
];

export const SOURCE_HTML = [
    `${SRC_DIR}/**/*.html`,
];

// dev server lives on localhost by default; note that it accepts one-of 127.0.0.1 or ::1
const DEV_HOST = 'localhost';
const DEV_PORT = 8080;

// MODULE_DEV is a special case, not intended for the writtable output
const MODULE_BUILD_PRESETS = new Set(
    Array.from(MODULE_PRESETS)
        .filter((x) => x !== MODULE_DEV));

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

const ERR_SINGLE_BUNDLE =
    new Error('js bundle only supports a single output');

/**
 * after destination finishes, log everything written so far
 * @param {string} dstdir)
 */
function dest(dstdir) {
    const out = orig_dest(dstdir);

    out.on('data', data);
    out.on('finish', finish);

    let srcpath = '';
    let size = 0;

    /** @param {File} source */
    function data(source) {
        srcpath = source.path;
        if (source.isBuffer()) {
            size = source.contents.length;
        }
    };

    function finish() {
        if (!srcpath || !size) {
            return;
        }

        const name = path.relative(ROOT, srcpath);
        if (srcpath.startsWith(BUILD_DIR)) {
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
 * @param {HtmlModify[]} handlers
 * @returns {Transform}
 */
function modifyHtml(handlers) {
    return new Transform({
        objectMode: true,
        async transform(source, _, callback) {
            if (!(source.contents instanceof Buffer)) {
                callback(ERR_CONTENTS_TYPE);
                return;
            }

            const dom = new JSDOM(source.contents, {includeNodeLocations: true});

            let results = [];
            for (const handler of handlers) {
                results.push(await Promise.resolve(handler(dom)));
            }

            if (results.some((x) => x)) {
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
 * @param {boolean} minify
 * @returns {HtmlModify}
 */
function injectVendor(minify) {
    return function(dom) {
        if (minify) {
            return false;
        }

        const script = dom.window.document.getElementsByTagName('script');

        const importmap = dom.window.document.createElement('script');
        importmap.setAttribute('type', 'importmap');

        const imports = Object.fromEntries(
            Object.keys(IMPORT_MAP).map((key) => [key, `./${key}`]));
        importmap.textContent = JSON.stringify({imports});

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
        return stripModulesImpl(dom, modules);
    };
}

/**
 * inline and render index.html from the source template
 * expected to be called before any other html parsing happens
 *
 * @param {BuildOptions} options
 * @param {BuildStats} stats
 * @returns {HtmlModify}
 */
function makeIndexHtml(options, stats) {
    return async function(dom) {
        let changed = false;

        for (const elem of dom.window.document.querySelectorAll('inline-source')) {
            let src = elem.getAttribute('src');
            if (!src) {
                continue;
            }

            if (path.isAbsolute(src)) {
                src = src.slice(1);
            }

            src = path.join(HTML_DIR, src);

            stats[src] = await fs.promises.stat(src);

            if (!needElement(elem, options.modules)) {
                elem.parentElement?.removeChild(elem);
                changed = true;
                continue;
            }

            const data = await fs.promises.readFile(src);
            elem.outerHTML = data.toString();

            changed = true;
        }

        return changed;
    };
}

/**
 * make an explicit list of all available imports, even the ones using uncommon names
 * @param {string} preset
 */
function makeImportAlias(preset) {
    return [
        {find: '/vendor', replacement: VENDOR_DIR},
        {find: '@build-preset', replacement: path.join(PRESET_DIR, preset)},
        {find: '@jaames/iro', replacement: IMPORT_MAP['@jaames/iro']},
    ];
}

/**
 * Inline every external resource into the entrypoint html.
 * Works outside of gulp context, track everything that passes through manually.
 *
 * nb. '@build-preset' consts module does not work properly w/ esbuild --bundle
 * Instead of creating noop statements when module is disabled, unused import
 * would still be injected into the resulting bundle.
 *
 * Rollup bundling prevents this from happening, allowing external consts file.
 * Previous implementation used 'globals' / 'define' as a workaround.
 *
 * ref. https://github.com/evanw/esbuild/issues/1420
 * ref. https://github.com/evanw/esbuild/issues/1895
 * ref. html/inline.mjs`forceNoSideEffects()`
 *
 * @param {BuildStats} stats
 * @param {BuildOptions} options
 * @returns {HtmlModify}
 */
function makeInlineSource(options, stats) {
    /**
     * @param {Buffer<ArrayBufferLike>} code
     * @param {string} type
     */
    function encode(code, type) {
        const encoded = type.includes('base64')
            ? code.toString('base64')
            : queryEscape(code.toString());
        return `data:${type},${encoded}`;
    }

    /**
     * @param {string} src
     * @param {string} tag
     * @param {string} type
     */
    async function load(src, tag, type) {
        const code = await fs.promises.readFile(src);
        if (tag !== 'SCRIPT' && type) {
            return encode(code, type);
        }

        switch (tag) {
        // by default, expect urlquotable svg. override type=... when it is not
        case 'LINK':
            return encode(code, 'image/svg+xml');

        // current build always produces a single entrypoint, minify asap
        case 'SCRIPT':
            if (!options.minify) {
                return code.toString();
            }

            const result = await rollup({
                input: src,
                treeshake: {
                    moduleSideEffects: () => false,
                },
                plugins: [
                    rollupAlias({
                        entries: makeImportAlias(options.name),
                    }),
                    rollupVirtual({
                        src: code.toString(),
                    }),
                    rollupEsbuild({
                        platform: 'browser',
                        target: 'es2022',
                        minify: true,
                        format: 'esm',
                    }),
                ],
            });

            const bundle = await result.generate({
                sourcemap: 'inline',
            });

            if (bundle.output.length !== 1) {
                throw ERR_SINGLE_BUNDLE;
            }

            return bundle.output[0].code;
        }

        return code.toString();
    }

    /**
     * @param {string} src
     */
    function resolve(src) {
        if (src.startsWith('/')) {
            src = src.slice(1);
        }

        src = path.join(HTML_DIR, src);

        // prevent '?...' query params from appearing,
        // vite assets would sometimes use '?inline'
        const asUrl = new URL(`file:///${src}`);
        for (const [param] of asUrl.searchParams) {
            asUrl.searchParams.delete(param);
        }

        return url.fileURLToPath(asUrl.href);
    }

    return async function(dom) {
        let changed = false;

        for (const elem of dom.window.document.querySelectorAll('link,script')) {
            await maybeInline(dom, elem, {
                load,
                resolve,
                async post(src) {
                    stats[src] = await fs.promises.stat(src);
                    changed = true;
                }
            });
        }

        return changed;
    };
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
 * @param {BuildOptions} options
 */
function buildHtml(options) {
    /** @type {BuildStats} */
    const stats = {};

    const out = [
        src(ENTRYPOINT),
        trackFileStats(stats),
        modifyHtml([
            makeIndexHtml(options, stats),
            makeInlineSource(options, stats),
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

/** @typedef ServeOptions
 * @property {string} name
 * @property {string} host
 * @property {number} port
 *
 * @param {ServeOptions} options
 */
function serveWebUI({name, host, port}) {
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
                ...buildHtml({name, modules: makeModules(name), compress: false, minify: false}),
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

        // in case importmap script was injected into the html
        const relpath = url.pathname.slice(1);
        const imported = IMPORT_MAP[/** @type {keyof IMPORT_MAP} */(relpath)];
        if (imported) {
            await responseJsFile(response, imported);
            return;
        }

        // everything else is attempted as html/src/${path}
        if (url.pathname.endsWith('.mjs')) {
            await responseJsFile(response, path.join(SRC_DIR, relpath));
            return;
        }

        response.writeHead(500, {'content-type': 'text/plain'});
        response.end('500');
    });

    server.on('listening', () => {
        log.info(`Serving ${SRC_DIR} index and *.mjs at`, server.address());
    });

    server.listen(port, host);
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
        async function* (/** @type {AsyncIterable<File>} */source) {
            for await (const chunk of source) {
                yield chunk.path;
            }
        },
    ];
}

// .spec.mjs vitest tests
export async function vitest() {
    return pipeline(
        /** @ts-ignore, types/node/stream/promises/pipeline.d.ts hates 'args' / '...args' */
        ...sourcePath(TEST_SCRIPTS),
        async function* (/** @type {AsyncIterable<any>} */source) {
            // ref. 'vitest/node' parseVitestCLI('vitest --environment jsdom --dir html/spec --run')
            let filter = [];
            for await (const chunk of source) {
                filter.push(chunk);
            }

            const { startVitest } = await import('vitest/node');
            const runner = await startVitest('test', filter, {
                config: path.join(ROOT, 'vitest.config.mjs'),
                run: true,
            });
            await runner.close();
        }
    );
}

// Generic javascript linting. *Could* happen at inline stage, but only without compression / minification
export async function eslint() {
    const files = [
        ...BUILD_SCRIPTS,
        ...TEST_SCRIPTS,
        ...SOURCE_SCRIPTS,
    ];

    return pipeline(
        /** @ts-ignore, types/node/stream/promises/pipeline.d.ts hates 'args' / '...args' */
        ...sourcePath(files),
        async function* (/** @type {AsyncIterable<string>} */source) {
            let paths = [];
            for await (const chunk of source) {
                paths.push(chunk);
            }

            const { ESLint } = await import('eslint');

            const runner = new ESLint({});
            const format = await runner.loadFormatter('stylish');

            const results = await runner.lintFiles(paths);

            const formatted = await format.format(results);
            if (formatted.length) {
                log(formatted);
            }

            const fatal = results.some((x) => x.errorCount > 0);
            if (fatal) {
                throw new Error(`eslint: ${path} failed`);
            }
        },
    );
}

// Validate all HTML sources. *Cannot* happen at inline stage, since JSDOM modifications break some style rules
async function html_validate() {
    return pipeline(
        /** @ts-ignore, types/node/stream/promises/pipeline.d.ts hates 'args' / '...args' */
        ...sourcePath(SOURCE_HTML),
        async function* (/** @type {AsyncIterable<string>} */source) {
            const {
                FileSystemConfigLoader,
                HtmlValidate,
                formatterFactory,
            } = await import('html-validate');

            const html = new HtmlValidate(new FileSystemConfigLoader());
            const format = formatterFactory('stylish');

            for await (const chunk of source) {
                const report = await html.validateFile(chunk);
                if (!report.valid) {
                    log(format(report.results));
                    throw new Error(`html-validate: ${path} failed`);
                }
            }
        },
    );
}

export { html_validate as 'html-validate' };

// -----------------------------------------------------------------------------
// Tasks
// -----------------------------------------------------------------------------

export async function presets() {
    /** @type {Error?} */
    let rethrow = null;

    let results = ['No presets generated'];
    try {
        results = await buildPresets();
    } catch (e) {
        if (e instanceof Error) {
            rethrow = e;
        } else {
            rethrow = new Error(e?.toString() ?? 'unknown error');
        }
    } finally {
        for (const result of results) {
            log(result);
        }
    }

    if (rethrow) {
        throw rethrow;
    }
}

presets.description = 'generate all of the required preset files, based on the html/preset.mjs configuration';

/** @import { ParseArgsOptionsConfig } from 'node:util' */

const ERR_PRESET_EMPTY = new Error('preset flag cannot be empty');

const ERR_PARSE_STRING = new Error('flag type !== string');
const ERR_PARSE_NUMBER = new Error('flag type !== number');

export function build() {
    const PRESET = 'preset';

    /** @type {ParseArgsOptionsConfig} */
    const options = {
        [PRESET]: {
            type: 'string',
            multiple: true,
            default: Array.from(MODULE_BUILD_PRESETS),
        },
    };

    // note that this only expects 'build' ...
    // any extra tasks launched in parallel *may* break
    // either parsing or the resulting task promise
    const { values } = parseArgs({
        allowPositionals: false,
        args: process.argv.slice(3),
        options,
    });

    const { preset } = values;
    if ((preset == null) || !preset) {
        throw ERR_PRESET_EMPTY;
    }

    /** @param {any} name */
    async function run(name) {
        if (typeof name === 'string') {
            return buildWebUI(name);
        }

        throw ERR_PARSE_STRING;
    }

    if (Array.isArray(preset)) {
        return Promise.all(preset.map(run));
    }

    return run(preset);
}

build.description = `builds one or more of the available presets: ${Array.from(MODULE_BUILD_PRESETS).join(', ')}`;
build.flags = {
    '--preset NAME': 'NAME of the build preset; can be specified multiple times',
};

export function dev() {
    const PRESET = 'preset';

    const HOST = 'host';
    const PORT = 'port';

    /** @type {ParseArgsOptionsConfig} */
    const options = {
        [PRESET]: {
            type: 'string',
            multiple: false,
            default: MODULE_DEV,
        },
        [HOST]: {
            type: 'string',
            multiple: false,
            default: DEV_HOST,
        },
        [PORT]: {
            type: 'string',
            multiple: false,
            default: DEV_PORT.toString(),
        },
    };

    const { values } = parseArgs({
        allowPositionals: false,
        args: process.argv.slice(3),
        options,
    });

    const { preset, port, host } = values;
    if (typeof preset !== 'string') {
        throw ERR_PARSE_STRING;
    }

    if (typeof port !== 'string') {
        throw ERR_PARSE_STRING;
    }

    if (typeof host !== 'string') {
        throw ERR_PARSE_STRING;
    }

    const parsed = parseInt(port, 10);
    if (!parsed) {
        throw ERR_PARSE_NUMBER;
    }

    return serveWebUI({name: preset, port: parsed, host});
}

dev.flags = {
    '--host HOST': `"${DEV_HOST}" by default`,
    '--port PORT': `${DEV_PORT} by default`,
    '--preset PRESET': `"${MODULE_DEV}" by default`,
};

export const test =
    parallel(
        eslint,
        html_validate,
        vitest);

export default
    series(test, build);
