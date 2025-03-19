// Generic parts of the HTML are placed into <template> container, which requires
// us to 'import' it into the currently loaded page to actually use it.
// (and notice that document.querySelector(...) won't be able to read inside of these)

import {
    listenEnumerable,
    onGroupSettingsDel,
    setGroupElement,
    setInputValue,
    setOriginalsFromValuesForNode,
    setSelectValue,
    setSpanValue,
} from './settings.mjs';

import { moreElem } from './core.mjs';

/**
 * @param {Event} event
 */
function moreParent(event) {
    event.preventDefault();
    event.stopPropagation();

    const target = event.target;
    if (!(target instanceof HTMLElement)) {
        return;
    }

    const parent = target?.parentElement?.parentElement;
    if (parent) {
        moreElem(parent);
    }
}

/**
 * @param {string} name
 * @returns {DocumentFragment}
 */
export function loadTemplate(name) {
    const template = /** @type {HTMLTemplateElement} */
        (document.getElementById(`template-${name}`));
    return document.importNode(template.content, true);
}

/** @import { InputOrSelect } from './settings.mjs' */

/**
 * @param {string} name
 * @returns {DocumentFragment}
 */
export function loadConfigTemplate(name) {
    const template = loadTemplate(name);
    for (let elem of /** @type {NodeListOf<InputOrSelect>} */(template.querySelectorAll("input,select"))) {
        setGroupElement(elem);
    }

    for (let elem of template.querySelectorAll("button.button-del-settings-group")) {
        elem.addEventListener("click", onGroupSettingsDel);
    }

    for (let elem of template.querySelectorAll("button.button-more-parent")) {
        elem.addEventListener("click", moreParent);
    }

    for (let elem of template.querySelectorAll("[data-enumerable]")) {
        if (!(elem instanceof HTMLElement)) {
            continue;
        }

        listenEnumerable(elem);
    }

    return template;
}

/**
 * @import { DisplayValue } from './settings.mjs'
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

    setOriginalsFromValuesForNode(fragment);
}

/**
 * @param {HTMLElement} target
 * @param {DocumentFragment} template
 * @returns {Element | null}
 */
export function mergeTemplate(target, template) {
    for (let child of Array.from(template.children)) {
        target.appendChild(child);
    }

    return target.lastElementChild;
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
 * @param {DisplayValue[][]} source
 * @param {string[]} schema
 * @returns {TemplateConfig[]}
 */
export function prepareFromSchema(source, schema) {
    return source.map((values) => fromSchema(values, schema));
}

/**
 * @param {HTMLElement} container
 * @param {string} name
 * @param {DisplayValue[][]} entries
 * @param {string[]} schema
 * @param {number} max
 */
export function addFromTemplateWithSchema(container, name, entries, schema, max = 0) {
    const prepared = prepareFromSchema(entries, schema);
    if (!prepared) {
        return;
    }

    if (max > 0) {
        container.dataset["settingsMax"] = max.toString();
    }

    prepared.forEach((cfg) => {
        addFromTemplate(container, name, cfg);
    });
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
