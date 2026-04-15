/** @import { InputOrSelect } from '../settings.mjs' */

import { findPanel, showPanel } from '../core.mjs';

const CUSTOM_VALIDITY = "customValidity";

/**
 * @param {InputOrSelect} elem
 * @param {string} message
 */
export function reportValidityForInputOrSelect(elem, message = "") {
    findPanel(elem, (panel) => {
        showPanel(panel);

        if (message.length !== 0) {
            elem.setCustomValidity(message);
            elem.dataset[CUSTOM_VALIDITY] = message;
        }

        elem.focus();
        elem.reportValidity();
    });
}

/**
 * @param {InputOrSelect} elem
 */
export function resetCustomValidity(elem) {
    delete elem.dataset[CUSTOM_VALIDITY];
    elem.setCustomValidity("");
}

