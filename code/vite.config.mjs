import * as path from 'node:path';
import * as url from 'node:url';
import * as fs from 'node:fs/promises';

import { minify as htmlMinifyImpl } from 'html-minifier-terser';

import { MODULE_PRESETS } from './html/preset.mjs';
import { maybeInline, needElement } from './html/inline.mjs';
import { defineConfig } from 'vite';
import { JSDOM } from 'jsdom';

import {
    HTML_DIR,
    VENDOR_DIR,
    PRESET_DIR,
    BUILD_DIR,
} from './gulpfile.mjs';

// vite build pipeline is simplified compared to the gulp one
// assets are mostly handled through the rollup <-> vite
//
// two parts are not
// - index.html pre-generation has to happen before build starts
// - modules stripping before and after index.html is pre-gened
//
// compared to (some) of the external plugins
// - https://www.npmjs.com/package/vite-plugin-singlefile
//   https://www.npmjs.com/package/vite-plugin-inline-source
//
//   html assets are sourced from the OutputBundle after the build
//   no extra config modifications; html inline handled differently
//
// - https://www.npmjs.com/package/vite-plugin-html
//   https://www.npmjs.com/package/vite-plugin-html-inject
//
//   inline-source custom element used instead of templating
//   package JSDOM dependency instead of plugin parser(s)

/** @import { PluginOption } from 'vite' */

/**
 * @param {import("./html/preset.mjs").Modules} modules
 * @returns {PluginOption}
 */
function stripModules(modules) {
    return {
        name: stripModules.name,
        enforce: 'pre',
        async transformIndexHtml(html) {
            const dom = new JSDOM(html);
            const doc = dom.window.document;

            for (const elem of Array.from(doc.querySelectorAll('[class*=module-]'))) {
                if (!needElement(elem, modules)) {
                    elem.parentElement?.removeChild(elem);
                }
            }

            return dom.serialize();
        }
    };
}

/**
 * @returns {PluginOption}
 */
function inlineHtmlPre() {
    const sources = [];

    return {
        name: inlineHtmlPre.name,
        enforce: 'pre',
        configureServer(server) {
            server.watcher.on('change', (file) => {
                if (file.endsWith('.html')) {
                    server.ws.send({
                        type: 'full-reload',
                        path: '*',
                    });
                }
            });
        },
        async transformIndexHtml(html) {
            const dom = new JSDOM(html);
            const doc = dom.window.document;

            for (const elem of Array.from(doc.querySelectorAll('inline-source'))) {
                const src = elem.getAttribute('src');
                if (!src) {
                    continue;
                }

                const data = await fs.readFile(path.join(HTML_DIR, src));
                elem.outerHTML = data.toString();

                sources.push(src);
            }

            return dom.serialize();
        }
    };
}

/**
 * @returns {PluginOption}
 */
function inlineAssetsPost() {
    return {
        name: inlineAssetsPost.name,
        apply: 'build',
        enforce: 'post',
        async transformIndexHtml(html, ctx) {
            // possibly, everything was already handled by some other plugin
            if (!ctx.bundle) {
                return html;
            }

            /**
             * assuming, everything left inlineable is in the generated bundle(s)
             * vite is expected to clean-up src=..., so no need to resolve(src)
             * @param {string} src
             */
            function load(src) {
                const output = (ctx.bundle ?? {})[src];
                if (!output) {
                    return '';
                }

                let code = '';
                if (output.type === 'asset' && typeof output.source === 'string') {
                    code = output.source;
                }
                if (output.type === 'chunk') {
                    code = output.code;
                }

                return code;
            }

            /**
             * output bundle should be kept empty, only index.html is written
             * @param {string} src
             */
            function post(src) {
                delete (ctx.bundle ?? {})[src];
            }

            const dom = new JSDOM(html);
            const doc = dom.window.document;

            for (const elem of Array.from(doc.querySelectorAll('link,script'))) {
                await maybeInline(dom, elem, {load, post});
            }

            return dom.serialize();
        }
    };
}

/**
 * @returns {PluginOption}
 */
function htmlMinify() {
    return {
        name: htmlMinify.name,
        apply: 'build',
        enforce: 'post',
        async transformIndexHtml(html) {
            html = await htmlMinifyImpl(html, {
                collapseWhitespace: true,
                removeComments: true,
                minifyCSS: false,
                minifyJS: false,
            });

            return html;
        },
    };
}

/**
 * @param {string} name
 * @returns {PluginOption}
 */
function rename(name) {
    return {
        name: rename.name,
        apply: 'build',
        enforce: 'post',
        generateBundle(_, bundle) {
            bundle['index.html'].fileName = `index.${name}.html`;
        },
    };
}

export default defineConfig(async ({ mode }) => {
    if (!MODULE_PRESETS.has(mode)) {
        throw new Error(`'mode' has to be one of: ${Array.from(MODULE_PRESETS).join(', ')}`);
    }

    const preset = path.join(PRESET_DIR, mode);
    const constants = url.pathToFileURL(
        path.join(preset, 'constants.mjs'));

    const { MODULES: modules } = await import(constants.href);

    return {
        // build order is declared by the plugin, but make sure 'pre' happen exactly like this
        // stripModules has to also help out the inlineHtmlPre, since it is not aware of the module deps
        plugins: [
            stripModules(modules),
            inlineHtmlPre(),
            stripModules(modules),
            inlineAssetsPost(),
            htmlMinify(),
            rename(mode),
        ],
        // vite-specific overrides, both for the build and importmap helper script
        resolve: {
            alias: [
                {find: '/vendor', replacement: VENDOR_DIR},
                {find: '@build-preset', replacement: preset},
            ],
        },
        // one should be very careful when injecting relative paths in the html / js / configs
        // step into the HTML_DIR by default (aka `vite ... html/` in the cli)
        root: HTML_DIR,
        envDir: preset,
        // rollup settings stay mostly the same, with the exception of the @jaames/iro workaround
        build: {
            modulePreload: {
                polyfill: false,
            },
            emptyOutDir: true,
            outDir: BUILD_DIR,
            rollupOptions: {
                treeshake: {
                    moduleSideEffects: () => false,
                },
            },
        },
    };
});
