/** @import { ElementValue } from './value.mjs' */

const SETTINGS_SELECT = "settingsSelect";

/**
 * @param {HTMLSelectElement} elem
 */
export function setBitsetSelect(elem) {
    elem.dataset[SETTINGS_SELECT] = "bitset";
}

/**
 * @param {HTMLSelectElement} elem
 */
export function resetBitsetSelect(elem) {
    delete elem.dataset[SETTINGS_SELECT];
}

/**
 * @param {HTMLSelectElement} elem
 */
export function isBitsetSelect(elem) {
    return elem.dataset[SETTINGS_SELECT] === "bitset";
}

/** @typedef {string | number | null} SelectValue */

/**
 * generally, comma-separated string. in special cases - number representing an u32 bitset
 * @param {HTMLSelectElement} elem
 * @returns {SelectValue}
 */
export function getSelectElementValue(elem) {
    if (elem.multiple) {
        const values = 
            Array.from(elem.selectedOptions)
            .map(option => option.value);
        if (isBitsetSelect(elem)) {
            return bitsetFromSelectedValues(values);
        }

        return values.join(",");

    } else if (elem.selectedIndex >= 0) {
        const option = elem.options[elem.selectedIndex];
        if (!option.disabled) {
            return option.value;
        }
    }

    return null;
}

/**
 * @param {HTMLSelectElement} elem
 * @param {ElementValue} value
 * @returns {SelectValue}
 */
export function preparedSelectElementValue(elem, value) {
    /** @type {SelectValue} */
    let out = null;

    if (elem.multiple && isBitsetSelect(elem)) {
        switch (typeof value) {
        case "boolean":
            out = value ? 1 : 0;
            break;

        case "string":
            out = bitsetFromSelectedValues(value.split(","));
            break;

        case "number":
            out = value;
            break;
        }
    } else {
        switch (typeof value) {
        case "boolean":
            out = value ? "1" : "0";
            break;

        case "string":
        case "number":
            out = value;
            break;
        }
    }

    return out;
}

// When receiving / returning data, sometimes <select multiple=true> <option> values are treated as bitset (u32) indexes (i.e. individual bits that are set)
// For example 0b101 is translated to ["0", "2"], or 0b1111 is translated to ["0", "1", "2", "3"]

/**
 * @param {number} bitset
 * @returns {string[]}
 */
function bitsetToSelectedValues(bitset) {
    let values = [];
    for (let index = 0; index < 31; ++index) {
        if (bitset & (1 << index)) {
            values.push(index.toString());
        }
    }

    return values;
}

/**
 * @param {string[]} values
 * @returns {number}
 */
function bitsetFromSelectedValues(values) {
    let result = 0;
    for (let value of values) {
        result |= 1 << parseInt(value);
    }

    return result;
}

/**
 * @param {HTMLSelectElement} select
 * @param {ElementValue} value
 */
export function setSelectValue(select, value) {
    /** @type string[] */
    const values = [];

    switch (typeof value) {
    case "boolean":
        values.push(value.toString());
        break;

    case "string":
        if (select.multiple) {
            values.push(...value.split(","))
        } else {
            values.push(value);
        }
        break;

    case "number":
        if (select.multiple && isBitsetSelect(select)) {
            values.push(...bitsetToSelectedValues(value));
        } else {
            values.push(value.toString());
        }
        break;
    }

    for (const option of select.options) {
        option.selected = values.includes(option.value);
    }
}

/**
 * @typedef {{value: string, text: string}} ElementOption
 */

/**
 * @param {HTMLSelectElement | HTMLDataListElement} elem
 * @param {ElementOption[]} options
 */
export function initElementOptions(elem, options) {
    const initial = document.createElement("option");
    initial.disabled = true;
    initial.value = "";

    // distinguish empty values from empty sets of options
    elem.appendChild(initial);
    if (elem instanceof HTMLSelectElement) {
        elem.selectedIndex = 0;
    }

    for (const option of options) {
        const child = document.createElement("option");
        child.value = option.value;
        child.textContent = option.text;
        elem.appendChild(child);
    }
}
