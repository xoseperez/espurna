/** @import { InputOrSelect } from '../settings.mjs' */

/**
 * generic value type to set to or get from an element. usually an editable type, like input or select
 * @typedef { string | number | boolean | null } ElementValue
 */

/**
 * @typedef {{name: string, value: ElementValue}} NamedElementValue
 */

import { getElementName } from './name.mjs';
import { getInputElementValue } from './input.mjs';
import { getSelectElementValue } from './select.mjs';

/**
 * @param {InputOrSelect} elem
 * @returns {ElementValue}
 */
export function getElementValue(elem) {
    if (elem instanceof HTMLInputElement) {
        return getInputElementValue(elem);
    } else if (elem instanceof HTMLSelectElement) {
        return getSelectElementValue(elem);
    }

    return null;
}

/**
 * @param {InputOrSelect} elem
 * @returns {NamedElementValue}
 */
export function getNamedElementValue(elem) {
    return {
        name: getElementName(elem),
        value: getElementValue(elem),
    };
}
