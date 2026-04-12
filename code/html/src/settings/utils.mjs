/** @import { InputOrSelect } from '../settings.mjs' */

// Not all of available forms are used for settings:
// - terminal input, which is implemented with an input field. it is attributed with `action="none"`, so settings handler never treats it as 'changed'
// - initial setup. it is shown programatically, but is still available from the global list of forms

const INPUT_OR_SELECT_QUERY_HINT = "[name]:not([readOnly]):not([data-action='none'])";
const INPUT_OR_SELECT_QUERY = `input${INPUT_OR_SELECT_QUERY_HINT},select${INPUT_OR_SELECT_QUERY_HINT}`;

/**
 * @param {Element | DocumentFragment} node
 * @returns {Array<InputOrSelect>}
 */
export function findInputOrSelect(node) {
    return Array.from(node.querySelectorAll(INPUT_OR_SELECT_QUERY));
}

/**
 * @param {HTMLElement} elem
 * @returns {boolean}
 */
export function isIgnoredElement(elem) {
    return elem.dataset["action"] === "none"
        || ((elem instanceof HTMLInputElement) && elem.readOnly)
        || (((elem instanceof HTMLInputElement) 
          || (elem instanceof HTMLSelectElement)) && !elem.name.length);
}
