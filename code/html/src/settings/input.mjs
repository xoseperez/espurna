/** @import { ElementValue } from './value.mjs' */

import { stringToBoolean } from '../core.mjs';

// TODO: <input type="radio"> is a special beast, since the actual value is one of 'checked' elements with the same name=... attribute.
// Right now, WebUI does not use this kind of input, but in case it does this needs a once-over that the actual input value is picked up correctly through all of changed / original comparisons.

/**
 * naïve value retrieval from the given subset of input types
 * @param {HTMLInputElement} elem
 * @returns {ElementValue}
 */
export function getInputElementValue(elem) {
    switch (elem.type) {
    case "checkbox":
        return elem.checked;

    case "radio":
        if (elem.checked) {
            return elem.value;
        }

        return null;

    case "text":
    case "password":
    case "hidden":
        return elem.value;

    case "number":
    case "range":
        return elem.valueAsNumber;

    }

    return null;
}

/**
 * prepare element value data for the setter, in case there are any doubts that it is the correct type
 * @param {HTMLInputElement} elem
 * @param {ElementValue} value
 * @returns {ElementValue}
 */
export function preparedInputElementValue(elem, value) {
    /** @type {ElementValue} */
    let out = null;

    switch (elem.type) {
    case "radio":
    case "text":
    case "password":
    case "hidden":
        out = value ?? "";
        break;

    case "checkbox":
        out = (typeof value === "string")
                ? stringToBoolean(value)
            : (typeof value === "boolean")
                ? value
            : (typeof value === "number")
                ? (value !== 0) : false;
        break;

    case "number":
    case "range":
        out = (typeof value === "string")
            ? parseInt(value)
            : (typeof value === "number")
                ? value : NaN;
        break;
    }

    return out;
}

/**
 * @param {HTMLInputElement} input
 * @param {ElementValue} value
 */
export function setInputValue(input, value) {
    switch (input.type) {
    case "radio":
        input.checked = (value === input.value);
        break;

    case "checkbox":
        input.checked =
            (typeof value === "boolean") ? value :
            (typeof value === "string") ? stringToBoolean(value) :
            (typeof value === "number") ? (value !== 0) : false;
        break;

    case "number":
    case "range":
        input.valueAsNumber =
            (typeof value === "string") ? parseInt(value) :
            (typeof value === "number") ? value : NaN;
        break;

    case "password":
    case "text":
        input.value =
            (value ?? "").toString();
        break;
    }
}
