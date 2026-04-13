/** @import { InputOrSelect } from '../settings.mjs' */

import { findInputOrSelect } from './utils.mjs';

export const SETTINGS_GROUP = "settings-group";

/**
 * @param {HTMLElement} elem
 * @returns {boolean}
 */
export function isGroup(elem) {
    return elem.classList.contains(SETTINGS_GROUP);
}

/**
 * @param {HTMLElement} elem
 */
export function setGroup(elem) {
    return elem.classList.add(SETTINGS_GROUP);
}

/**
 * @param {HTMLElement} elem
 * @returns {Element | null}
 */
export function parentGroup(elem) {
    return elem.closest(`.${SETTINGS_GROUP}`);
}

export const SETTINGS_GROUP_ELEMENTS = "settings-group-elements";

/**
 * @param {HTMLElement} elem
 * @returns {Element | null}
 */
export function parentGroupElements(elem) {
    return elem.closest(`.${SETTINGS_GROUP_ELEMENTS}`);
}

/**
 * @param {HTMLElement} elem
 * @returns {boolean}
 */
export function isGroupElements(elem) {
    return elem.classList.contains(SETTINGS_GROUP_ELEMENTS);
}

/**
 * @param {HTMLElement} elem
 */
export function setGroupElements(elem) {
    return elem.classList.add(SETTINGS_GROUP_ELEMENTS);
}

export const SETTINGS_GROUP_ELEMENT = "settings-group-element";

/**
 * @param {HTMLElement} elem
 */
export function setGroupElement(elem) {
    elem.classList.add(SETTINGS_GROUP_ELEMENT);
}

/**
 * @param {HTMLElement} elem
 */
export function resetGroupElement(elem) {
    elem.classList.remove(SETTINGS_GROUP_ELEMENT);
}

/**
 * @param {HTMLElement} elem
 * @returns {boolean}
 */
export function isGroupElement(elem) {
    return elem.classList.contains(SETTINGS_GROUP_ELEMENT);
}

const SETTINGS_MAX = "settingsMax";

/**
 * @param {HTMLElement} elem
 * @returns {number}
 */
export function getGroupMax(elem) {
    const max = elem.dataset[SETTINGS_MAX];
    if ((max === undefined) || (max === "0")) {
        return 0;
    }

    const out = parseInt(max, 10);
    if (Number.isNaN(out) || (out <= 0)) {
        return 0;
    }

    return out;
}

/**
 * @param {HTMLElement} elem
 * @param {number} value
 */
export function setGroupMax(elem, value) {
    elem.dataset[SETTINGS_MAX] = `${value}`;
}

/**
 * @param {HTMLElement} elem
 */
export function resetSettingsMax(elem) {
    delete elem.dataset[SETTINGS_MAX];
}

const SETTINGS_GROUP_CLEANUP = "settings-group-cleanup";

/**
 * @param {HTMLElement} elem
 */
export function setGroupCleanup(elem) {
    elem.classList.add(SETTINGS_GROUP_CLEANUP);
}

/**
 * @param {HTMLElement} elem
 */
export function resetGroupCleanup(elem) {
    elem.classList.remove(SETTINGS_GROUP_CLEANUP);
}

/**
 * @param {HTMLElement | InputOrSelect[]} elem_or_elems
 * @returns {InputOrSelect[]}
 */
export function groupCleanupElements(elem_or_elems) {
    let elems = elem_or_elems instanceof HTMLElement
        ? findInputOrSelect(elem_or_elems)
        : elem_or_elems;

    const cleanup = elems.filter(
        (x) => x.classList.contains(SETTINGS_GROUP_CLEANUP));
    if (cleanup.length !== 0) {
        elems = cleanup;
    }

    return elems;
}
