/** @import { InputOrSelect } from '../settings.mjs' */

import { parentGroupElements, getGroupMax } from './group.elem.mjs';
import { checkAndSetElementChanged } from './change.mjs';
import { findInputOrSelect } from './utils.mjs';
import {
    maybeGetFormId,
    getOriginalForName,
    resetDatasetCleanup,
    resetChangedElement,
    setDatasetCleanup,
} from './dataset.mjs';

const SETTINGS_SCHEMA = "settingsSchema";
const SETTINGS_SCHEMA_DEL = `${SETTINGS_SCHEMA}Del`;

 /** @typedef {{del: string[], schema: string[]}} GroupSettingsSchema */

/**
 * @param {HTMLElement} elem
 * @returns {GroupSettingsSchema}
 */
export function groupSettingsSchema(elem) {
    const [del, schema] =
        [SETTINGS_SCHEMA_DEL, SETTINGS_SCHEMA]
        .map((name) => elem.dataset[name] ?? "")
        .map((data) => data.split(" "));
    return { del, schema };
}

/**
 * @param {HTMLElement} elem
 * @returns {string[]}
 */
function pickGroupSettingsSchema(elem) {
    const { del, schema } = groupSettingsSchema(elem);
    return ((del.length > 0) ? del : schema)
}

// Right now, group additions happen from:
// - WebSocket, likely to happen exactly once per connection through processData handler(s). Specific keys trigger functions that append into the container element.
// - User input. Same functions are triggered, but with an additional event for the container element that causes most recent element to be marked as changed.
// Removal only happens from user input by triggering 'settings-group-del' from the target element.

/**
 * @param {HTMLElement} group
 * @param {HTMLElement} target
 */
export function groupSettingsAdd(group, target) {
    /**
     * @type {(function(InputOrSelect): boolean) | null}
     * @param {InputOrSelect} elem
     */
    let validity = function(elem) {
        elem.focus();
        return elem.reportValidity();
    };

    for (let elem of findInputOrSelect(target)) {
        if (elem.required && validity && !validity(elem)) {
            validity = null;
        }

        checkAndSetElementChanged(elem);
    }

    const form = group.closest("form");
    const formId = maybeGetFormId(form);

    const index = group.children.length - 1;

    pickGroupSettingsSchema(group)
        .map((key) => `${key}${index}`)
        .forEach((key) => {
            resetDatasetCleanup(formId, key);
        });
}

// 'settings-group' contain elements that represent kv list that is suffixed with an index in raw kvs
// 'button-add-settings-group' will trigger update on the specified 'data-settings-group' element id, which
// needs to have 'settings-group-add' event handler attached to it.

/**
 * to 'instantiate' a new element, we must explicitly set 'target' keys in kvs
 * notice that the 'row' creation *should* be handled by the group-specific
 * event listener, we already expect the dom element to exist at this point
 * @param {Event} event
 */
function onGroupSettingsEventAdd(event) {
    const group = event.target;
    if (!(group instanceof HTMLElement)) {
        return;
    }

    const target = group.lastElementChild;
    if (!(target instanceof HTMLElement)) {
        return;
    }

    groupSettingsAdd(group, target);
}

/**
 * @param {HTMLElement} group
 * @param {HTMLElement} target
 */
export function groupSettingsDel(group, target) {
    const form = group.closest("form");
    const formId = maybeGetFormId(form);

    const delIndex = group.children.length - 1;

    /** @type {function(string): string} */
    const makeDelKey =
        (key) => `${key}${delIndex}`;

    /** @type {function(string): boolean} */
    const originalExists =
        (key) => getOriginalForName(formId, key) !== undefined;

    // note that if 'schema' is not the same as 'del' attr,
    // dataset keeps previous values until page reload
    const delKeys = pickGroupSettingsSchema(group)
        .map(makeDelKey)
        .filter(originalExists);

    if (delKeys.length > 0) {
        setDatasetCleanup(formId, delKeys);
    }

    findInputOrSelect(target)
        .map(resetChangedElement);
    target.remove();

    findInputOrSelect(group)
        .forEach((elem) => {
            checkAndSetElementChanged(elem);
        })
}

/**
 * @param {Event} event
 */
export function onGroupSettingsDel(event) {
    const target = event.target;
    if (!(target instanceof HTMLElement)) {
        return;
    }

    const elements = parentGroupElements(target);
    if (!(elements instanceof HTMLElement)) {
        return;
    }

    elements.dispatchEvent(
        new CustomEvent("settings-group-del", {bubbles: true}));
}

/**
 * removing the element means we need to notify the kvs about the updated keys
 * in case it's the last row, just remove those keys from the store
 * in case we are in the middle, make sure to handle difference update
 * in case change was 'ephemeral' (i.e. from the previous add that was not saved), do nothing
 * @param {Event} event
 */
export function onGroupSettingsEventDel(event) {
    event.preventDefault();
    event.stopImmediatePropagation();

    const target = event.target;
    if (!(target instanceof HTMLElement)) {
        return;
    }

    const group = event.currentTarget;
    if (!(group instanceof HTMLElement)) {
        return;
    }

    groupSettingsDel(group, target);
}

const SETTINGS_GROUP_EVENT_ADD = "settings-group-add";

/**
 * @param {HTMLElement} elem
 * @param {EventListener} listener
 */
export function groupSettingsOnAddElem(elem, listener) {
    elem.addEventListener(SETTINGS_GROUP_EVENT_ADD,
        (event) => {
            event.stopPropagation();
            listener(event);
            onGroupSettingsEventAdd(event);
        });
}

/**
 * @param {string} id
 * @param {EventListener} listener
 */
export function groupSettingsOnAdd(id, listener) {
    const elem = document.getElementById(id);
    if (elem) {
        groupSettingsOnAddElem(elem, listener);
    }
}

const SETTINGS_GROUP = "settingsGroup";

/**
 * handle addition to the group using the 'add' button
 * @param {Event} event
 */
export function onGroupSettingsAddClick(event) {
    const elem = event.target;
    if (!(elem instanceof HTMLElement)) {
        return;
    }

    const id = elem.dataset[SETTINGS_GROUP];
    if (!id) {
        return;
    }

    const group = document.getElementById(id);
    if (!group) {
        throw `Unable to find group w/ [id='${id}']`;
    }

    const max = getGroupMax(group);
    if ((max > 0) && (1 + group.children.length) > max) {
        alert(`Can't add more than ${max} elements to ${id}`);
        return;
    }

    group.dispatchEvent(new CustomEvent(SETTINGS_GROUP_EVENT_ADD));
}
