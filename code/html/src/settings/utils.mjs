/** @import { InputOrSelect } from '../settings.mjs' */

import { count, stringToBoolean } from '../core.mjs';

/**
 * @param {HTMLElement} elem
 */
export function setChangedElement(elem) {
    elem.dataset["changed"] = "true";
}

/**
 * @param {HTMLElement} elem
 */
export function resetChangedElement(elem) {
    elem.dataset["changed"] = "false";
}

/**
 * @param {HTMLElement} elem
 */
function resetGroupPending(elem) {
    delete elem.dataset["settingsGroupPending"];
}

export function resetSettingsGroup() {
    const elems = document.getElementsByClassName("settings-group");
    for (let elem of elems) {
        if (!(elem instanceof HTMLElement)) {
            continue;
        }

        resetChangedElement(elem);
        resetGroupPending(elem);
    }
}

/**
 * @param {HTMLElement} elem
 */
export function isChangedElement(elem) {
    return stringToBoolean(elem.dataset["changed"] ?? "");
}

/**
 * @param {Element} node
 */
export function getElements(node) {
    return /** @type {Array<InputOrSelect>} */(
        Array.from(node.querySelectorAll(
            "input[data-changed],select[data-changed]")));
}

/**
 * @param {Element} node
 */
export function countChangedElements(node) {
    return count(getElements(node), isChangedElement);
}

const SETTINGS_GROUP_ELEMENT = "settingsGroupElement";

/**
 * @param {HTMLElement} elem
 */
export function setGroupElement(elem) {
    elem.dataset[SETTINGS_GROUP_ELEMENT] = "true";
}

/**
 * @param {HTMLElement} elem
 */
export function resetGroupElement(elem) {
    delete elem.dataset[SETTINGS_GROUP_ELEMENT];
}

/**
 * @param {HTMLElement} elem
 * @returns {boolean}
 */
export function isGroupElement(elem) {
    return elem.dataset[SETTINGS_GROUP_ELEMENT] !== undefined;
}

const SETTINGS_IGNORED_ELEMENT = "settingsIgnore";

/**
 * @param {HTMLElement} elem
 */
export function setIgnoredElement(elem) {
    elem.dataset[SETTINGS_IGNORED_ELEMENT] = "true";
}

/**
 * @param {HTMLElement} elem
 */
export function resetIgnoredElement(elem) {
    delete elem.dataset[SETTINGS_IGNORED_ELEMENT];
}

/**
 * @param {HTMLElement} elem
 * @returns {boolean}
 */
export function isIgnoredElement(elem) {
    return elem.dataset[SETTINGS_IGNORED_ELEMENT] !== undefined;
}
