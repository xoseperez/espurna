/** @import { InputOrSelect } from './settings.mjs' */

import { findPanel, showPanel } from './core.mjs';

import { isChangedElement } from './settings/dataset.mjs';
import { isIgnoredElement, findInputOrSelect } from './settings/utils.mjs';
import { reportValidityForInputOrSelect } from './validate/utils.mjs';

import {
    filterForm,
    formPassPair,
    validatePassword,
} from './password/utils.mjs';

/**
 * @param {InputOrSelect} elem
 * @returns {boolean}
 */
function validateInputOrSelect(elem) {
    if (elem.checkValidity()) {
        return true;
    }

    reportValidityForInputOrSelect(elem);
    return false;
}

/**
 * @param {HTMLFormElement[]} forms
 * @returns {boolean}
 */
export function validateFormsReportValidity(forms) {
    const elems = forms
        .flatMap((form) => getElements(form))
        .filter((x) => isChangedElement(x) && !isIgnoredElement(x))

    if (!elems.length) {
        return false;
    }

    return elems.every(validateInputOrSelect);
}

/**
 * @typedef {{strict?: boolean, assumeChanged?: boolean}} ValidationOptions
 */

const DIFFERENT_PASSWORD = "Passwords are different!";
const EMPTY_PASSWORD = "Password cannot be empty!";
const INVALID_PASSWORD = "Invalid password!";

/**
 * Try to validate password pair in the given list of forms. Alerts when validation fails.
 * With initial setup, this usually happens to be the only validation func w/ optional strict mode.
 * With normal panel, strict is expected to be false.
 * Only 'changed' elements affect validation, password fields can remain empty and still pass validation.
 * @param {HTMLFormElement[]} forms
 * @param {ValidationOptions} options
 * @returns {boolean}
 */
export function validateFormsPasswords(forms, {strict = true, assumeChanged = false} = {}) {
    const [form] = filterForm(forms);
    if (!form) {
        return true;
    }

    let err = "";

    const inputs = formPassPair(form);
    if (!inputs || inputs.length !== 2) {
        err = EMPTY_PASSWORD;
    } else if (assumeChanged || inputs.some(isChangedElement)) {
        if (!inputs[0].value.length || !inputs[1].value.length) {
            err = EMPTY_PASSWORD;
        } else if (inputs[0].value !== inputs[1].value) {
            err = DIFFERENT_PASSWORD;
        } else if (strict && !validatePassword(inputs[0].value)) {
            err = INVALID_PASSWORD;
        }
    }

    if (!err) {
        return true;
    }

    if (inputs.length === 2) {
        const first = inputs[0];
        findPanel(first, (panel) => {
            showPanel(panel);
            first.focus();
        });
    }

    alert(err);
    return false;
}


