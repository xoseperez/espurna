/** @import { ElementValue } from './value.mjs' */

import { capitalize } from '../core.mjs';

/**
 * @param {HTMLSpanElement} span
 * @param {ElementValue} value
 * @returns {string}
 */
function prepareSpanValue(span, value) {
    value = value ?? "";
    value = value.toString();

    if (value) {
        value = span.dataset[`value${capitalize(value)}`] ?? value;
    }

    const out = [
        `${span.dataset["pre"] || ""}`,
        `${value}`,
        `${span.dataset["post"] || ""}`,
    ];

    return out.join("");
}

/**
 * generic value to be set to an element. usually cannot be edited after setting, expected to be updated from the device side
 * @typedef { ElementValue | ElementValue[] } DisplayValue
 */

/**
 * @param {HTMLSpanElement} span
 * @param {DisplayValue} value
 */
export function setSpanValue(span, value) {
    if (Array.isArray(value)) {
        /** @type {Node[]} */
        const nodes = [];

        value.forEach((entry) => {
            nodes.push(new Text(prepareSpanValue(span, entry)));
            nodes.push(document.createElement("br"));
        });
        span.replaceChildren(...nodes);
    } else {
        span.textContent = prepareSpanValue(span, value);
    }
}
