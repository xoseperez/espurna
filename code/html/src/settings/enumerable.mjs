/** @import { ElementOption } from './select.mjs' */

import { getElementOriginal } from './dataset.mjs';
import { initElementOptions, setSelectValue } from './select.mjs';
import { setSpanValue } from './span.mjs';

/**
 * Automatically updates element contents using named entries.
 *
 * Consumer is expected to
 * - set 'data-<NAME>' attribute on the element, either statically or dynamically
 * - install default or custom listener for the respective <NAME>d event
 *
 * Element behaviour varies
 * - <select> recreates <options> with value=$key and labeled with the string contents
 * - <span> inner text is updated with the all of the string contents joined together
 *
 * @typedef {[string, string]} EnumerableTuple
 * @typedef {{[k: string]: string}} EnumerableNames
 */

/** @type {{[k: string]: EnumerableNames}} */
let __enumerable_names = {};

/** PREFIX + NAME propogated to every "data-enumerable='NAME'" */
const ENUMERABLE_EVENT_PREFIX = 'enumerable-update-';

// <select> initialization from simple {id: ..., name: ...} that map as <option> value=... and textContent
// To avoid depending on order of incoming messages, always cache the value and provide a way to re-initialize every 'enumerable' <select> element on the page
//
// Notice that <select multiple> input and output format is u32 number, but the 'original' string is comma-separated <option> value=... attributes

/**
 * @param {EnumerableNames} names
 * @returns {ElementOption[]}
 */
function elementOptionsFromEnumerable(names) {
    /** @type {ElementOption[]} */
    const out = [];

    Object.entries(names)
        .forEach(([id, name]) => {
            out.push({
                "value": id,
                "text": name,
            });
        });

    return out;
}

/**
 * @param {HTMLElement} elem
 */
function cleanupChildElements(elem) {
    while (elem.childElementCount && elem.firstElementChild) {
        elem.removeChild(elem.firstElementChild);
    }
}

/**
 * @param {HTMLSelectElement} select
 * @param {EnumerableNames} names
 */
function onEnumerableUpdateSelect(select, names) {
    cleanupChildElements(select);
    initElementOptions(select, elementOptionsFromEnumerable(names));

    const value = getElementOriginal(select);
    if (value !== null) {
        setSelectValue(select, value);
    }
}

/**
 * @param {HTMLSpanElement} span
 * @param {EnumerableNames} names
 */
function onEnumerableUpdateSpan(span, names) {
    const id = span.dataset["enumerableId"] ?? "";
    if (id.length === 0) {
        return;
    }

    const name = names[id];
    if (!name) {
        return;
    }

    setSpanValue(span, name);
}

/**
 * @param {HTMLDataListElement} datalist
 * @param {EnumerableNames} names
 */
function onEnumerableUpdateDataList(datalist, names) {
    cleanupChildElements(datalist);
    initElementOptions(datalist, elementOptionsFromEnumerable(names));
}

/**
 * @callback EnumerableElemCallback
 * @param {HTMLElement} elem
 * @param {EnumerableNames} names
 * @returns {void}
 */

/**
 * @type {EnumerableElemCallback}
 */
function onEnumerableUpdateElem(elem, names) {
    if (elem instanceof HTMLSelectElement) {
        onEnumerableUpdateSelect(elem, names);
    } else if (elem instanceof HTMLSpanElement) {
        onEnumerableUpdateSpan(elem, names);
    } else if (elem instanceof HTMLDataListElement) {
        onEnumerableUpdateDataList(elem, names);
    }
}

/**
 * @param {Event} event
 * @param {EnumerableElemCallback} callback
 */
function onEnumerableUpdate(event, callback) {
    const elem = /** @type {!HTMLElement} */(event.target);
    const enumerables = /** @type {CustomEvent<{enumerables: EnumerableNames}>} */
        (event).detail.enumerables;
    callback(elem, enumerables);
}

/**
 * @param {string} name
 * @param {EnumerableNames} enumerables
 */
function notifyEnumerables(name, enumerables) {
    document.querySelectorAll(`[data-enumerable=${name}]`)
        .forEach((elem) => {
            if (!(elem instanceof HTMLElement)) {
                return;
            }

            elem.dispatchEvent(
                new CustomEvent(
                    `${ENUMERABLE_EVENT_PREFIX}${name}`,
                    {detail: {enumerables}}));
        });
}

/**
 * @param {HTMLElement} elem
 * @param {string} name
 * @param {EnumerableElemCallback?} callback
 */
export function listenEnumerableName(elem, name, callback = null) {
    callback = callback ?? onEnumerableUpdateElem;
    elem.addEventListener(
        `${ENUMERABLE_EVENT_PREFIX}${name}`,
        (event) => onEnumerableUpdate(event, callback));

    const current = __enumerable_names[name];
    if (!current) {
        return;
    }

    callback(elem, current);
}

/**
 * @param {HTMLElement} elem
 * @param {number} id
 * @param {string} name
 */
export function prepareEnumerableTarget(elem, id, name) {
    elem.dataset["enumerableId"] = id.toString();
    elem.dataset["enumerable"] = name;
}

/**
 * @param {HTMLElement} elem
 * @param {number} id
 * @param {string} name
 * @param {EnumerableElemCallback?} callback
 */
export function listenEnumerableTarget(elem, id, name, callback = null) {
    const span = document.createElement("span");
    prepareEnumerableTarget(span, id, name);
    listenEnumerableName(span, name, callback);
    elem.appendChild(span);
}

/**
 * @param {HTMLElement} elem
 * @param {EnumerableElemCallback?} callback
 */
export function listenEnumerable(elem, callback = null) {
    const name = elem.dataset["enumerable"];
    if (!name) {
        return;
    }

    listenEnumerableName(elem, name, callback);
}

/**
 * @param {string} name
 * @returns {EnumerableNames}
 */
export function getEnumerables(name) {
    return __enumerable_names[name] ?? {};
}

/**
 * @param {string} name
 * @param {EnumerableNames | EnumerableTuple[]} enumerables
 */
export function addEnumerables(name, enumerables) {
    /** @type {EnumerableNames} */
    let names = {};

    if (Array.isArray(enumerables)) {
        enumerables.forEach(([id, name]) => {
            names[id] = name;
        });
    } else {
        names = enumerables;
    }

    __enumerable_names[name] = names;
    notifyEnumerables(name, names);
}

/**
 * @param {string} name
 * @param {string} prettyName
 * @param {number} count
 */
export function addSimpleEnumerables(name, prettyName, count) {
    if (count <= 0) {
        return;
    }

    /** @type {EnumerableNames} */
    const names = {};
    for (let id = 0; id < count; ++id) {
        names[id.toString()] = `${prettyName} #${id}`;
    }

    addEnumerables(name, names);
}
