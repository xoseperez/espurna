import * as fs from 'node:fs/promises';
import * as path from 'node:path';
import * as url from 'node:url';

const ROOT_DIR = path.dirname(url.fileURLToPath(import.meta.url));
const PRESETS_DIR = path.join(ROOT_DIR, 'preset');

/**
 * module names used internally, usually as element class-name
 * @typedef {{[k: string]: boolean}} Modules
 */

/* used for the locally served .html, that is already merged but not yet inlined */
export const MODULE_DEV = 'dev';

/**
 * declare some modules as optional, only to be included for specific builds
 * @constant
 * @type Modules
 */
export const DEFAULT_MODULES = {
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
    'telnet': true,
    'thermostat': false,
    'tspk': true,
    [MODULE_DEV]: false,
};

/**
 * special type of build when multiple single-module files are used
 * currently, only possible way to combine both (besides modifying the targets manually)
 * @constant
 * @type Modules
 */
export const MODULES_ALL = Object.fromEntries(
    Object.entries(DEFAULT_MODULES).map(
        ([key, _]) => {
            if (MODULE_DEV === key) {
                return [key, false];
            }

            return [key, true];
        }));
/**
 * always assume dev build is all plus dev
 * @constant
 * @type Modules
 */
export const MODULES_DEV =
    Object.assign({}, MODULES_ALL, {[MODULE_DEV]: true});

/**
 * name mapping for output filename, when specific module is used
 * @typedef {{[k: string]: string}} NamedBuild
 */

/**
 * generic output, usually this includes a single module
 * @constant
 * @type {NamedBuild}
 */
export const NAMED_BUILD = {
    'curtain': 'curtain',
    'garland': 'garland',
    'light': 'light',
    'lightfox': 'lightfox',
    'rfbridge': 'rfb',
    'rfm69': 'rfm69',
    'sensor': 'sns',
    'thermostat': 'thermostat',
};

/**
 * @param {string} name
 * @returns {Modules}
 */
export function makeModules(name) {
    switch (name) {
    case 'all':
        return MODULES_ALL;

    case MODULE_DEV:
        return MODULES_DEV;

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
 * @param {string} name
 * @returns {string}
 */
function nameToConstant(name) {
    return `MODULE_${name.toUpperCase()}`;
}

/**
 * ref. html/src/index.mjs
 * @param {Modules} modules
 */
function asConstants(modules) {
    const entries = Object.entries(modules);
    const exports =
        entries.map(([key, value]) => {
            return [nameToConstant(key), value.toString()];
        })
        .map(([name, value]) => {
            return `export const ${name} = ${value};`;
        });

    return exports.join('\n');
}

/**
 * ref. vite.config.mjs
 * @param {Modules} modules
 */
function asDictionary(modules) {
    const entries = Object.keys(modules)
        .map((name) => [name, nameToConstant(name)])
        .map(([name, value]) => `    '${name}': ${value},`);

    return `export const MODULES = {\n${entries.join('\n')}\n};`;
}

export const MODULE_PRESETS = new Set([
    'all',
    'small',
    'curtain',
    'garland',
    'light',
    'lightfox',
    'rfbridge',
    'rfm69',
    'sensor',
    'thermostat',
    MODULE_DEV,
]);

/** @param {any} e */
function maybeRethrow(e) {
    if (e instanceof Error) {
        return e;
    }

    return new Error(e?.toString() ?? 'unknown error');
}

/** 
 * @param {string} target
 * @param {string} data
 */
async function maybe_write(target, data) {
    let existing = '';

    /** @type {fs.FileHandle?} */
    let readable = null;

    try {
        readable = await fs.open(target, 'r');
        existing = (await readable.read()).toString();
    } catch (_) {
        /* whatever */
    } finally {
        if (readable) {
            await readable.close();
        }
    }

    if (existing === data) {
        return;
    }

    try {
        await fs.mkdir(path.dirname(target));
    } catch (_) {
        /* whatever */
    }

    /** @type {Error?} */
    let rethrow = null;

    /** @type {fs.FileHandle?} */
    let writable = null;

    try {
        writable = await fs.open(target, 'w');
        await writable.write(data);
    } catch (e) {
        rethrow = maybeRethrow(e);
    } finally {
        if (writable) {
            await writable.close();
        }
    }

    if (rethrow) {
        throw rethrow;
    }
}

/**
 * @param {string} name
 * @param {string} [outdir]
 */
async function builder(name, outdir = PRESETS_DIR) {
    if (!outdir) {
        outdir = PRESETS_DIR;
    }

    const target_dir = path.join(outdir, name);

    const modules = makeModules(name);
    const targets = [
        [
            path.join(target_dir, 'constants.mjs'),
            [asConstants(modules), asDictionary(modules)].join('\n'),
        ],
    ];

    let out = `Written: ${target_dir}`;

    try {
        for (const [target, data] of targets) {
            await maybe_write(target, data);
        }
    } catch (e) {
        out = `Error ${target_dir}: ${(e ?? 'unknown error').toString()}`;
        for (const [target] of targets) {
            await fs.rm(target, {force: true});
        }
    }

    return out;
}

export async function build() {
    return Promise.all(Array.from(MODULE_PRESETS).map((x) => builder(x)));
}

//if (process.argv[1] === import.meta.filename) {
//    await build();
//}
