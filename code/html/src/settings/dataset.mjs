/** @import { InputOrSelect } from '../settings.mjs' */
/** @import { ElementValue, NamedElementValue } from './value.mjs' */

/** @typedef {{[name: string]: ElementValue}} DatasetFormValue */
/** @typedef {{[formId: string]: DatasetFormValue}} DatasetFormValues */

import { getElementName } from './name.mjs';
import { findInputOrSelect } from './utils.mjs';
import { getNamedElementValue } from './value.mjs';

import { preparedInputElementValue } from './input.mjs';
import { preparedSelectElementValue } from './select.mjs';

/**
 * aka 'original' values, as received and set on the form element(s)
 * @type {DatasetFormValues} */
const __dataset = {};

/**
 * @param {(string | string[])?} formId_or_formIds
 * @returns {DatasetFormValue}
 */
export function getOriginals(formId_or_formIds = null) {
    /** @type {DatasetFormValue} */
    let out = {};

    if (typeof formId_or_formIds === "string") {
        out = Object.assign({}, __dataset[formId_or_formIds] ?? {});
    } else if (formId_or_formIds === null) {
        out = Object.values(__dataset)
            .reduce((prev, curr) =>
                Object.assign(prev, curr), {});
    } else {
        for (const formId of formId_or_formIds) {
            out = Object.assign(out, __dataset[formId] ?? {});
        }
    }

    return out;
}

/**
 * @param {string} formId
 * @param {(string | string[])?} name_or_names
 */
export function resetOriginals(formId, name_or_names = null) {
    if (__dataset[formId] === undefined) {
        return;
    }

    if (typeof name_or_names === "string") {
        delete __dataset[formId][name_or_names];
    } else if (name_or_names === null) {
        delete __dataset[formId];
    } else {
        for (const name of name_or_names) {
            delete __dataset[formId][name];
        }
    }

    if ((__dataset[formId] !== undefined)
     && (Object.keys(__dataset[formId]).length === 0))
    {
        delete __dataset[formId];
    }
}

/**
 * @param {string} formId
 * @param {string} name
 */
export function getOriginalForName(formId, name) {
    return __dataset?.[formId]?.[name];
}

/**
 * @param {InputOrSelect} elem
 * @param {NamedElementValue} value
 */
function setElementOriginalImpl(elem, value) {
    const formId = maybeGetFormId(elem?.form);
    if (__dataset[formId] === undefined) {
        __dataset[formId] = {};
    }

    __dataset[formId][value.name] = value.value;
}

/**
 * @param {InputOrSelect} elem
 */
export function setOriginalFromValue(elem) {
    setElementOriginalImpl(elem, getNamedElementValue(elem));
    resetChangedElement(elem);
}

/**
 * @param {InputOrSelect[]} elems
 */
export function setOriginalsFromValues(elems) {
    for (const elem of elems) {
        setOriginalFromValue(elem);
    }
}

/**
 * @param {HTMLElement | DocumentFragment} node
 */
export function setOriginalsFromValuesForNode(node) {
    setOriginalsFromValues(findInputOrSelect(node));
}

/**
 * @param {HTMLFormElement | null} form
 * @returns {string}
 */
export function maybeGetFormId(form) {
    return form?.id ?? "default";
}

/**
 * @param {InputOrSelect} elem
 * @returns {string}
 */
export function getElementFormId(elem) {
    return maybeGetFormId(elem.form);
}

/**
 * @param {InputOrSelect} elem
 * @returns {NamedElementValue}
 */
function getNamedElementOriginalImpl(elem) {
    const formId = maybeGetFormId(elem?.form);

    const name = getElementName(elem);
    const value = __dataset?.[formId]?.[name] ?? null;

    return { name, value };
}

/**
 * @param {InputOrSelect} elem
 * @returns {NamedElementValue}
 */
export function getNamedElementOriginal(elem) {
    const { name, value } = getNamedElementOriginalImpl(elem);

    /** @type {NamedElementValue} */
    const out = {
        name,
        value: null,
    };

    if (elem instanceof HTMLInputElement) {
        out.value = preparedInputElementValue(elem, value);
    } else if (elem instanceof HTMLSelectElement) {
        out.value = preparedSelectElementValue(elem, value);
    }

    return out;
}

/**
 * @param {InputOrSelect} elem
 * @returns {ElementValue}
 */
export function getElementOriginal(elem) {
    const { value } = getNamedElementOriginal(elem);
    return value;
}

/**
 * tracking inputs / selects in the DOM that triggered change event
 * @type {Set<InputOrSelect>} */
const __dataset_changed = new Set();

/** @param {InputOrSelect} elem */
export function isChangedElement(elem) {
    return __dataset_changed.has(elem);
}

/** @param {InputOrSelect} elem */
export function setChangedElement(elem) {
    __dataset_changed.add(elem);
    emitPendingChanges();
}

/** @param {InputOrSelect} elem */
export function resetChangedElement(elem) {
    __dataset_changed.delete(elem);
    emitPendingChanges();
}

/** @param {HTMLFormElement[]} forms */
export function resetChangedElements(forms) {
    for (const changed of __dataset_changed) {
        if ((changed.form !== null) && forms.includes(changed.form)) {
            __dataset_changed.delete(changed);
        }
    }
    emitPendingChanges();
}

export function anyChangedElements() {
    return __dataset_changed.size > 0;
}

export function* changedElements() {
    for (const value of __dataset_changed) {
        yield value;
    }
}

const DATASET_PENDING_CHANGES_EVENT = "dataset-pending-changes";

function emitPendingChanges() {
    window.dispatchEvent(
        new CustomEvent(DATASET_PENDING_CHANGES_EVENT, {
            detail: {
                pendingChanges: pendingChanges(document.forms),
            },
        }));
}

/** @typedef {CustomEvent<{pendingChanges: boolean}>} InputOrSelectEvent */

/** @param {function(boolean): void} callback */
export function listenPendingChanges(callback) {
    window.addEventListener(DATASET_PENDING_CHANGES_EVENT,
        (event) => {
            const pending = /** @type {InputOrSelectEvent} */
                (event).detail.pendingChanges;
            callback(pending);
        });
}

/**
 * form element(s) modified by the user, currently only happens in groups
 * note that dataset *may* retain some keys for the row / entry whenever -del schema is set for the group
 * @typedef {{[formId: string]: Set<string>}} DatasetCleanup */

/** @type {DatasetCleanup} */
const __dataset_cleanup = {};

/**
 * @param {HTMLFormElement | Iterable<HTMLFormElement>} form_or_forms
 */
export function pendingChanges(form_or_forms) {
    const forms = (form_or_forms instanceof HTMLFormElement)
        ? [form_or_forms]
        : form_or_forms;

    const formIds = [];
    for (const form of forms) {
        formIds.push(form.id);
    }

    if (formIds.some((x) => __dataset_cleanup[x] !== undefined)) {
        return true;
    }

    for (const value of __dataset_changed.values()) {
        const formId = value?.form?.id;
        if (formId !== undefined && formIds.includes(formId)) {
            return true;
        }
    }

    return false;
}

/** @param {string} formId */
export function cleanupDataset(formId) {
    for (const name of __dataset_cleanup[formId]?.values() ?? []) {
        delete (__dataset[formId] ?? {})[name];
    }
    delete __dataset_cleanup[formId];

    emitPendingChanges();
}

export function resetDatasetCleanupAll() {
    const keys = Object.keys(__dataset_cleanup);
    for (const key of keys) {
        delete __dataset_cleanup[key];
    }

    emitPendingChanges();
}

/**
 * @param {string} formId
 * @param {(string | string[])?} name_or_names
 */
export function resetDatasetCleanup(formId, name_or_names = null) {
    if (typeof name_or_names === "string") {
        __dataset_cleanup[formId]?.delete(name_or_names);
    } else if (name_or_names === null) {
        delete __dataset_cleanup[formId];
    } else {
        for (const name of name_or_names) {
            resetDatasetCleanup(formId, name);
        }
    }

    if ((__dataset_cleanup[formId] !== undefined)
     && (__dataset_cleanup[formId].size === 0)) {
        delete __dataset_cleanup[formId];
    }

    emitPendingChanges();
}

/**
 * @param {string} formId
 * @param {string | string[]} name_or_names
 */
export function setDatasetCleanup(formId, name_or_names) {
    if (__dataset_cleanup[formId] === undefined) {
        __dataset_cleanup[formId] = new Set();
    }

    if (Array.isArray(name_or_names)) {
        name_or_names.forEach((name) => {
            __dataset_cleanup[formId].add(name);
        });
    } else {
        __dataset_cleanup[formId].add(name_or_names);
    }

    emitPendingChanges();
}

/**
 * kvs for the device settings storage
 * @typedef {string | number} DataValue
 * @typedef {{[k: string]: DataValue}} DataRequestValue
 */

/**
 * @param {ElementValue} value
 * @returns {DataValue}
 */
export function elementToDataValue(value) {
    if (value === null) {
        return "";
    } else if (typeof value === "boolean") {
        return value ? 1 : 0;
    } else if (typeof value === "string" && (value.length > 0)) {
        const number = Number(value);
        if (!Number.isNaN(number)) {
            return number;
        }
    } else if (typeof value === "number" && isNaN(value)) {
        return "nan";
    }

    return value;
}

/**
 * usually, settings request is sent as a single object
 * @typedef {{set: DataRequestValue, del: string[]}} DataRequest
 */

/**
 * returns data storage modification request for the device, updating values for or resetting specific keys
 * @param {HTMLFormElement[]} forms
 * @param {{assumeChanged?: boolean}} opts?
 * @returns {DataRequest}
 */
export function makeDataRequest(forms, { assumeChanged = false } = {}) {
    /** @type DataRequest */
    const out = {
        set: {},
        del: [],
    };

    for (const form of forms) {
        for (const elem of form.elements) {
            if (!(elem instanceof HTMLInputElement)
             && !(elem instanceof HTMLSelectElement))
            {
                continue;
            }

            if (assumeChanged || isChangedElement(elem)) {
                const { name, value } = getNamedElementValue(elem);
                out.set[name] = elementToDataValue(value);
            }
        }

        const formId = maybeGetFormId(form);
        for (const delKey of __dataset_cleanup[formId] ?? []) {
            out.del.push(delKey);
        }
    }

    return out;
}

