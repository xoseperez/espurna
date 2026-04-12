/** @import { InputOrSelect } from '../settings.mjs' */
/** @import { ElementValue } from './value.mjs' */

/**
 * @typedef {{name: string, value: ElementValue}} NamedElementValue
 */

import {
    isGroupElement,
    parentGroup,
    parentGroupElements,
} from './group.elem.mjs';

/**
 * @param {InputOrSelect} elem
 * @returns {number}
 */
export function maybeGroupIndex(elem) {
    if (isGroupElement(elem)) {
        const elements = parentGroupElements(elem);
        const group = elements instanceof HTMLElement
            ? parentGroup(elements)
            : null;

        if ((group !== null) && (elements !== null)) {
            return Array.from(group.children).indexOf(elements);
        }
    }

    return -1;
}

/**
 * @param {InputOrSelect} elem
 * @returns {string}
 */
export function relativeOrRawName(elem) {
    const index = maybeGroupIndex(elem);
    if (index >= 0) {
        return `${elem.name}${index}`;
    }

    return elem.name;
}

/**
 * @param {InputOrSelect} elem
 * @returns {string}
 */
export function getElementName(elem) {
    return elem.dataset["key"] ?? relativeOrRawName(elem);
}
