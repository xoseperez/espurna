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
    dest as destination,
    series,
    parallel,
    src as source,
} from 'gulp';

import { inlineSource } from 'inline-source';
import { build as esbuildBuild } from 'esbuild';
import { minify as htmlMinify } from 'html-minifier-terser';
import { JSDOM } from 'jsdom';

import * as convert from 'convert-source-map';
import fancyLog from 'fancy-log';

import { ESLint } from 'eslint';
import { formatterFactory, FileSystemConfigLoader, HtmlValidate } from 'html-validate';

import { Transform } from 'node:stream';
import { pipeline } from 'node:stream/promises';

import * as fs from 'node:fs';
import * as zlib from 'node:zlib';
import * as http from 'node:http';
import * as path from 'node:path';

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
 * build pipeline common options
 * @typedef BuildOptions
 * @property {boolean} compress
 * @property {Modules} modules
 */

/**
 * build pipeline usually works with file inputs and its transformations
 * @typedef {Transform | NodeJS.ReadStream | NodeJS.ReadWriteStream} BuildStream
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
// output .html.gz, cleaned-up for .gz.h generation
const BUILD_DIR = path.join('html', 'build');

// input sources, making sure relative inline paths start from here
const SRC_DIR = path.join('html', 'src');

// main source file used by inline-source
const ENTRYPOINT = path.join(SRC_DIR, 'index.html')

// .gz.h compiled from the .html.gz, providing a static u8[] for the firmware to use
const STATIC_DIR = path.join('espurna', 'static');

// -----------------------------------------------------------------------------
// Build
// -----------------------------------------------------------------------------

/**
 * @param {import("html-minifier-terser").Options} options
 * @returns {Transform}
 */
function toMinifiedHtml(options) {
    return new Transform({
        objectMode: true,
        async transform(source, _, callback) {
            if (!source.contents) {
                callback(new Error('source contents cannot be empty'));
                return;
            }

            const contents = source.contents.toString();
            const minified = await htmlMinify(contents, options);

            source.contents = Buffer.from(minified);

            callback(null, source);
        }});
}

/**
 * @param {string} name
 * @returns {string}
 */
function safename(name) {
    return path.basename(name).replaceAll('.', '_');
}

/**
 * @returns {Transform}
 */
function toGzip() {
    return new Transform({
        objectMode: true,
        transform(/** @type {File} */source, _, callback) {
            if (!(source.contents instanceof Buffer)) {
                callback(new Error('expecting source contents to be a buffer!'));
                return;
            }

            zlib.gzip(source.contents.buffer, {level: zlib.constants.Z_BEST_COMPRESSION},
                (error, result) => {
                    if (!error) {
                        source.contents = result;
                        source.path += '.gz';
                        callback(null, source);
                    } else {
                        callback(error);
                    }
                });
        }});
}

/**
 * generates c++-friendly header output from the stream contents
 * @param {string} name
 * @returns {Transform}
 */
function toHeader(name) {
    return new Transform({
        objectMode: true,
        transform(/** @type {File} */source, _, callback) {
            if (!(source.contents instanceof Buffer)) {
                callback(new Error('expecting source contents to be a buffer!'));
                return;
            }

            let output = `alignas(4) static constexpr uint8_t ${safename(name)}[] PROGMEM = {`;
            for (let i = 0; i < source.contents.length; i++) {
                if (i > 0) { output += ','; }
                if (0 === (i % 20)) { output += '\n'; }
                output += '0x' + ('00' + source.contents[i].toString(16)).slice(-2);
            }
            output += '\n};\n';

            // replace source stream with a different one, also replacing contents
            const dest = source.clone();
            dest.path = `${source.path}.h`;
            dest.contents = Buffer.from(output);

            callback(null, dest);
        }});
}

/**
 * by default, destination preserves stat.*time of the source. which is obviosly bogus here as gulp
 * only knows about the entrypoint and not about every include happenning through inline-source
 * @returns {Transform}
 */
function adjustFileStat() {
    return new Transform({
        objectMode: true,
        transform(source, _, callback) {
            if (source.stat) {
                const now = new Date();
                source.stat.atime = now;
                source.stat.mtime = now;
                source.stat.ctime = now;
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
 * @param {BuildOptions} options
 * @returns {import("inline-source").Handler}
 */
function inlineHandler(srcdir, options) {
    return async function(source) {
        // TODO split handlers
        if (source.content) {
            return;
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
                srcdir, define, options.compress);
            if (!result.outputFiles.length) {
                throw new Error('js bundle cannot be empty');
            }

            let content = Buffer.from(result.outputFiles[0].contents);

            if (!options.compress) {
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
                callback(new Error('expecting source contents to be a buffer!'));
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
 * when compression is on, make sure sourcemaps are not written to the resulting .gz.h
 * @returns {HtmlModify}
 */
function dropSourcemap() {
    return function(dom) {
        let changed = false;

        const scripts = dom.window.document.getElementsByTagName('script');
        for (let script of scripts) {
            if (changed) {
                break;
            }

            if (script.getAttribute('type') === 'importmap') {
                continue;
            }

            if (!script.textContent) {
                continue;
            }

            script.textContent =
                convert.removeMapFileComments(script.textContent);

            changed = true;
        }

        return changed;
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
 * @param {BuildOptions} options
 * @returns {Transform}
 */
function makeInlineSource(srcdir, options) {
    return new Transform({
        objectMode: true,
        async transform(source, _, callback) {
            if (!source.contents) {
                callback(new Error('expecting non-empty source contents'))
                return;
            }

            const contents = await inlineSource(
                source.contents.toString(),
                {
                    'compress': options.compress,
                    'handlers': [inlineHandler(srcdir, options)],
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
                callback(new Error('expecting source contents to be a buffer!'));
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
 * @returns {BuildStream[]}
 */
function buildHtml(options) {
    const out = [
        source(ENTRYPOINT),
        makeInlineSource(SRC_DIR, options),
        modifyHtml([
            injectVendor(options.compress),
            stripModules(options.modules),
            externalBlank(),
        ]),
    ];

    if (options.compress) {
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
 * @param {string} name
 * @returns {BuildStream[]}
 */
function buildOutputs(name) {
    /** @type {{[k: string]: number}} */
    const sizes = {};

    const logSize = () => new Transform({
        objectMode: true,
        transform(source, _, callback) {
            sizes[path.relative('.', source.path)] = source?.contents?.length ?? 0;
            callback(null, source);
        }});

    const dumpSize = () => new Transform({
        objectMode: true,
        transform(source, _, callback) {
            for (const [name, size] of Object.entries(sizes)) {
                fancyLog(`${name}: ${size} bytes`);
            }
            callback(null, source);
        }});

    return [
        rename(`index.${name}.html`),
        adjustFileStat(),
        destination(BUILD_DIR),
        logSize(),
        modifyHtml([
            dropSourcemap(),
        ]),
        toGzip(),
        destination(BUILD_DIR),
        logSize(),
        toHeader('webui_image'),
        destination(STATIC_DIR),
        logSize(),
        dumpSize(),
    ];
}

/**
 * @param {string} name
 */
function buildWebUI(name) {
    return pipeline([
        ...buildHtml({
            compress: true,
            modules: makeModules(name),
        }),
        ...buildOutputs(name),
    ]);
}

/**
 * @param {string} name
 */
function serveWebUI(name) {
    const server = http.createServer();

    /**
     * @param {http.ServerResponse<http.IncomingMessage>} response
     * @param {string} path
     */
    async function responseJsFile(response, path) {
        response.writeHead(200, {
            'Content-Type': 'text/javascript',
        });

        await pipeline(
            fs.createReadStream(path), response);
    }

    /**
     * @param {http.ServerResponse<http.IncomingMessage>} response
     */
    async function responseIndex(response) {
        response.writeHead(200, {
            'Content-Type': 'text/html',
        });

        await pipeline([
            ...buildHtml({modules: makeModules(name), compress: false}),
            new Transform({
                objectMode: true,
                transform(source, _, callback) {
                    callback(null, source.contents);
                }}),
            response,
        ]);
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
        console.log(`Serving ${SRC_DIR} index and *.mjs at`, server.address());
    });

    server.listen(8080, 'localhost');
}

// -----------------------------------------------------------------------------
// Source code validation.
// -----------------------------------------------------------------------------

/**
 * map input pattern to paths, do not populate source.contents
 * @param {string | string[]} pattern
 * @returns {BuildStream[]}
 */
function sourcePath(pattern) {
    return [
        source(pattern, {read: false, buffer: false}),
        new Transform({
            objectMode: true,
            transform(source, _, callback) {
                callback(null, source.path);
            }}),
    ];
}

// Generic javascript linting. *Could* happen at inline stage, but no real reason b/c of modules
export async function eslint() {
    const runner = new ESLint({});
    const format = await runner.loadFormatter('stylish');

    return pipeline([
        ...sourcePath([
            'gulpfile.mjs',
            'html/src/*.mjs',
            'html/spec/*.mjs',
        ]),
        new Transform({
            objectMode: true,
            async transform(path, _, callback) {
                const results = await runner.lintFiles([path]);
                const resultText = await format.format(results);

                if (resultText.length) {
                    fancyLog(resultText);
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
export async function html_validate() {
    const html = new HtmlValidate(new FileSystemConfigLoader());
    const format = formatterFactory('stylish');

    return pipeline([
        ...sourcePath('html/src/*.html'),
        new Transform({
            objectMode: true,
            async transform(path, _, callback) {
                const report = await html.validateFile(path);
                if (!report.valid) {
                    fancyLog(format(report.results));
                    callback(new Error(`html-validate: ${path} failed`));
                    return;
                }

                callback(null);
            }}),
    ]);
}

// -----------------------------------------------------------------------------
// Tasks
// -----------------------------------------------------------------------------

export function webui_serve() {
    return serveWebUI('local');
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

export default
    series(
        eslint,
        html_validate,
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
            webui_thermostat));
