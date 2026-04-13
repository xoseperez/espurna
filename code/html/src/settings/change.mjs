/** @import { InputOrSelect } from '../settings.mjs' */

import { resetCustomValidity } from '../validate/utils.mjs';
import { getNamedElementValue } from './value.mjs';

import {
    getNamedElementOriginal,
    isChangedElement,
    setChangedElement,
    resetChangedElement,
} from './dataset.mjs';

/**
 * @param {InputOrSelect} elem
 * @returns {boolean}
 */
export function checkAndSetElementChanged(elem) {
    const prev = isChangedElement(elem);

    const lhs = getNamedElementOriginal(elem);
    const rhs = getNamedElementValue(elem);

    if (lhs.name !== rhs.name) {
        return false;
    }

    let changed = false;
    if (typeof lhs.value === "number"
     && typeof rhs.value === "number"
     && isNaN(lhs.value)
     && isNaN(rhs.value))
    {
        /* normalized values are the same, even if input is not */
    } else {
        changed = lhs.value !== rhs.value;
    }

    if (changed) {
        setChangedElement(elem);
    } else {
        resetChangedElement(elem);
    }

    return prev !== isChangedElement(elem);
}

/**
 * @param {Event} event
 */
export function onElementChange(event) {
    const target = event.target;
    if (!(target instanceof HTMLInputElement)
     && !(target instanceof HTMLSelectElement))
    {
        return;
    }

    if (target instanceof HTMLInputElement && target.readOnly) {
        return;
    }

    const action = target.dataset["action"];
    if ("none" === action) {
        return;
    }

    resetCustomValidity(target);

    if (!checkAndSetElementChanged(target)) {
        return;
    }

    if (target.required) {
        target.reportValidity();
    }
}
