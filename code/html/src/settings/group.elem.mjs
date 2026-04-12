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
