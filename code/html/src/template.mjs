// Generic parts of the HTML are placed into <template> container, which requires
// us to 'import' it into the currently loaded page to actually use it.
// (and notice that document.querySelector(...) won't be able to read inside of these)

import {
    setOriginalsFromValuesForNode,
} from './settings/dataset.mjs';

import { setInputValue } from './settings/input.mjs';
import { setSelectValue } from './settings/select.mjs';
import { setSpanValue } from './settings/span.mjs';

import { listenEnumerable } from './settings/enumerable.mjs';

import { setGroupElement, setGroupElements, setGroupMax } from './settings/group.elem.mjs';
import { onGroupSettingsDel } from './settings/group.mjs';

import {
    loadTemplate,
    mergeTemplate,
} from './settings/template.mjs';

import { moreParent } from './core.mjs';
import { passwordReveal } from './password.mjs';

/**
 * @import { InputOrSelect } from './settings.mjs'
 */

/**
 * @param {string} name
 * @returns {DocumentFragment}
 */
export function loadConfigTemplate(name) {
    const template = loadTemplate(name);

    if (template.children.length === 1 && template.firstElementChild instanceof HTMLElement) {
        setGroupElements(template.firstElementChild);
    }

    for (const elem of /** @type {NodeListOf<InputOrSelect>} */(template.querySelectorAll("input,select"))) {
        setGroupElement(elem);
    }

    for (const elem of template.querySelectorAll("button.button-del-settings-group")) {
        elem.addEventListener("click", onGroupSettingsDel);
    }

    for (const elem of template.querySelectorAll("[data-enumerable]")) {
        if (!(elem instanceof HTMLElement)) {
            continue;
        }

        listenEnumerable(elem);
    }

    moreParent(template);
    passwordReveal(template);

    return template;
}

/**
 * @import { DisplayValue } from './settings/span.mjs'
 * @typedef {{[k: string]: DisplayValue}} TemplateConfig
 */

/** @typedef {InputOrSelect | HTMLSpanElement} TemplateLineElement */

/**
 * @param {DocumentFragment} fragment
 * @param {number} id
 * @param {TemplateConfig} cfg
 */
export function fillTemplateFromCfg(fragment, id, cfg = {}) {
    const local = {"template-id": id};
    cfg = Object.assign({}, local, cfg);

    for (let elem of /** @type {NodeListOf<TemplateLineElement>} */(fragment.querySelectorAll("input,select,span"))) {
        const key =
           ((elem instanceof HTMLInputElement)
         || (elem instanceof HTMLSelectElement))
                ? (elem.name) :
            (elem instanceof HTMLElement)
                ? elem.dataset["key"]
                : "";

        if (!key) {
            continue;
        }

        const value = cfg[key];
        if ((value === undefined) || (value === null)) {
            continue;
        }

        const is_array = Array.isArray(value);
        if (!is_array && elem instanceof HTMLInputElement) {
            setInputValue(elem, value);
        } else if (!is_array && elem instanceof HTMLSelectElement) {
            setSelectValue(elem, value);
        } else if (elem instanceof HTMLSpanElement) {
            setSpanValue(elem, value);
        }
    }
}

/**
 * @param {HTMLElement} container
 * @param {string} name
 * @param {TemplateConfig} cfg
 * @returns {Element | null}
 */
export function addFromTemplate(container, name, cfg) {
    const fragment = loadConfigTemplate(name);
    fillTemplateFromCfg(fragment, container.childElementCount, cfg);
    return mergeTemplate(container, fragment);
}

// TODO: note that we also include kv schema as 'data-settings-schema' on the container.
// produce a 'set' and compare instead of just matching length?

/**
 * @param {DisplayValue[]} values
 * @param {string[]} schema
 * @returns {TemplateConfig}
 */
export function fromSchema(values, schema) {
    if (schema.length !== values.length) {
        throw `Schema mismatch! Expected length ${schema.length} vs. ${values.length}`;
    }

    /** @type {{[k: string]: any}} */
    const out = {};
    schema.forEach((key, index) => {
        out[key] = values[index];
    });

    return out;
}

/**
 * @param {DisplayValue[][]} entries
 * @param {string[]} schema
 * @returns {TemplateConfig[]}
 */
export function prepareFromSchema(entries, schema) {
    return entries.map((x) => fromSchema(x, schema));
}

/**
 * @param {HTMLElement} container
 * @param {string} name
 * @param {TemplateConfig[]} prepared
 */
export function addOriginalsFromTemplateWithPreparedSchema(container, name, prepared) {
    prepared.forEach((cfg) => {
        addFromTemplate(container, name, cfg);
    });
    setOriginalsFromValuesForNode(container);
}

/**
 * @param {HTMLElement} container
 * @param {string} name
 * @param {{entries: DisplayValue[][], schema: string[], max?: number}} opts
 */
export function addOriginalsFromTemplate(container, name, { entries, schema, max = 0 }) {
    if (max > 0) {
        setGroupMax(container, max);
    }

    addOriginalsFromTemplateWithPreparedSchema(container, name, prepareFromSchema(entries, schema));
}

export class BaseInput {
    /** @param {string} name */
    constructor(name) {
        this.fragment = loadConfigTemplate(name);
    }

    /**
     * @param {function(HTMLLabelElement, HTMLInputElement, HTMLSpanElement): void} callback
     * @returns {DocumentFragment}
     */
    with(callback) {
        const out = document.createDocumentFragment();
        out.appendChild(this.fragment.cloneNode(true));

        const root = /** @type {!HTMLDivElement} */
            (out.children[0]);

        callback(
            /** @type {!HTMLLabelElement} */(root.children[0]),
            /** @type {!HTMLInputElement} */(root.children[1]),
            /** @type {!HTMLSpanElement} */(root.children[2]));

        return out;
    }
}

export class TextInput extends BaseInput {
    constructor() {
        super("text-input");
    }
}

export class NumberInput extends BaseInput {
    constructor() {
        super("number-input");
    }
}
