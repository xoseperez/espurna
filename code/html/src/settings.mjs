import {
    capitalize,
    pageReloadIn,
    showPanelByName,
    stringToBoolean,
} from './core.mjs';

import {
    send,
    sendAction,
    listenAppConnected,
} from './connection.mjs';

import { validateFormsReportValidity, validateFormsPasswords, resetCustomValidity } from './validate.mjs';
import { notifyError } from './notify.mjs';

import {
    countChangedElements,
    isChangedElement,
    isGroupElement,
    isIgnoredElement,
    setChangedElement,
    resetChangedElement,
    resetSettingsGroup,
} from './settings/utils.mjs';

/**
 * generic value type to set to or get from an element. usually an editable type, like input or select
 * @typedef { string | number | boolean | null } ElementValue
 */

/**
 * generic value to be set to an element. usually cannot be edited after setting, expected to be updated from the device side
 * @typedef { ElementValue | ElementValue[] } DisplayValue
 */

/**
 * @typedef { HTMLInputElement | HTMLSelectElement } InputOrSelect
 */

/**
 * @typedef {{element: InputOrSelect, key: string, value: ElementValue}} GroupElementInfo
 */

// Right now, group additions happen from:
// - WebSocket, likely to happen exactly once per connection through processData handler(s). Specific keys trigger functions that append into the container element.
// - User input. Same functions are triggered, but with an additional event for the container element that causes most recent element to be marked as changed.
// Removal only happens from user input by triggering 'settings-group-del' from the target element.
//
// TODO: distinguish 'current' state to avoid sending keys when adding and immediately removing the latest node?
// TODO: previous implementation relied on defaultValue and / or jquery $(...).val(), but this does not really work where 'line' only has <select>

/**
 * @param {Element} target
 * @returns {GroupElementInfo[]}
 */
function groupElementInfo(target) {
    /** @type {GroupElementInfo[]} */
    const out = [];

    findInputOrSelect(target).forEach((elem) => {
        const name = elem.dataset.settingsRealName || elem.name;
        if (name === undefined) {
            return;
        }

        const value = getOriginalForElement(elem) ?? getDataForElement(elem);
        if (null === value) {
            return;
        }

        out.push({
            element: elem,
            key: name,
            value: value,
        });
    });

    return out;
}

/**
 * @param {HTMLElement} elem
 * @param {string[]} pending
 */
function setGroupPending(elem, pending) {
    elem.dataset["settingsGroupPending"] =
        ([...new Set(pending)]).join(" ");
}

/**
 * @param {HTMLElement} elem
 * @returns {string[]}
 */
function getGroupPending(elem) {
    const raw = elem.dataset["settingsGroupPending"] || "";
    if (!raw.length) {
        return [];
    }

    return raw.split(" ");
}

/**
 * @param {HTMLElement} group
 */
export function groupSettingsAdd(group) {
    const index = group.children.length - 1;
    const last = group.children[index];

    const before = countChangedElements(group);

    const pending = getGroupPending(group);
    pending.push(`set:${index}`);
    setGroupPending(group, pending);

    let once = true;

    for (let elem of findInputOrSelect(last)) {
        if (once && elem.required) {
            elem.focus();
            elem.reportValidity();
            once = false;
        }

        if (elem.required
            && elem.checkValidity()
            && (getDataForElement(elem) !== null))
        {
                setChangedElement(elem);
        }
    }

    Settings.countFor(
        countChangedElements(group) - before);
    Settings.stylizeSave();
}

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

    groupSettingsAdd(group);
}

/**
 * @param {HTMLElement} group
 */
function delGroupPending(group) {
    const top = group.childElementCount - 1;
    if (top < 0) {
        return;
    }

    let pending = getGroupPending(group);

    const set = pending.indexOf(`set:${top}`);
    if (set >= 0) {
        pending.splice(set, 1);
    } else {
        pending.push(`del:${top}`);
    }

    setGroupPending(group, pending);
}

/**
 * @param {HTMLElement} group
 * @param {HTMLElement} target
 */
export function groupSettingsDel(group, target) {
    const elems = Array.from(group.children);
    const shiftFrom = elems.indexOf(target);

    const before = countChangedElements(group);

    const info = elems.map(groupElementInfo);
    for (let index = -1; index < info.length; ++index) {
        const prev = (index > 0)
            ? info[index - 1]
            : null;
        const current = info[index];

        if ((index > shiftFrom) && prev && (prev.length === current.length)) {
            for (let inner = 0; inner < prev.length; ++inner) {
                const [lhs, rhs] = [prev[inner], current[inner]];
                if (lhs.value !== rhs.value) {
                    setChangedElement(rhs.element);
                }
            }
        }
    }

    delGroupPending(group);
    target.remove();

    Settings.countFor(
        countChangedElements(group) - before);
    Settings.stylizeSave();
}

/**
 * removing the element means we need to notify the kvs about the updated keys
 * in case it's the last row, just remove those keys from the store
 * in case we are in the middle, make sure to handle difference update
 * in case change was 'ephemeral' (i.e. from the previous add that was not saved), do nothing
 * @param {Event} event
 */
function onGroupSettingsEventDel(event) {
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

// 'settings-group' contain elements that represent kv list that is suffixed with an index in raw kvs
// 'button-add-settings-group' will trigger update on the specified 'data-settings-group' element id, which
// needs to have 'settings-group-add' event handler attached to it.

/**
 * @param {Event} event
 * @returns {boolean}
 */
function groupSettingsCheckMax(event) {
    const target = event.target;
    if (!(target instanceof HTMLElement)) {
        return false;
    }

    const max = target.dataset["settingsMax"];
    const val = 1 + target.children.length;

    if ((max !== undefined) && (max !== "0") && (parseInt(max, 10) < val)) {
        alert(`Max number of ${target.id} has been reached (${val} out of ${max})`);
        return false;
    }

    return true;
}

/**
 * @param {HTMLElement} elem
 * @param {EventListener} listener
 */
export function groupSettingsOnAddElem(elem, listener) {
    elem.addEventListener("settings-group-add",
        (event) => {
            event.stopPropagation();
            if (!groupSettingsCheckMax(event)) {
                return;
            }

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

/**
 * @param {Event} event
 */
export function onGroupSettingsDel(event) {
    let target = /** @type {HTMLElement | null} */(event.target);
    if (!(target instanceof HTMLElement)) {
        return;
    }

    let parent = target.parentElement;
    if (!(parent instanceof HTMLElement)) {
        return;
    }

    while (parent && !parent.classList.contains("settings-group")) {
        target = parent;
        parent = target.parentElement;
    }

    target.dispatchEvent(
        new CustomEvent("settings-group-del", {bubbles: true}));
}

/**
 * handle addition to the group using the 'add' button
 * @param {Event} event
 */
function onGroupSettingsAddClick(event) {
    const elem = event.target;
    if (!(elem instanceof HTMLElement)) {
        return;
    }

    const name = elem.dataset["settingsGroup"];
    if (!name) {
        return;
    }

    document.getElementById(name)
        ?.dispatchEvent(new CustomEvent("settings-group-add"));
}

/**
 * @param {HTMLElement} container
 * @param {string[]} keys
 * @returns {string[]} - key# for each node that needs removal
 */
function groupSettingsCleanup(container, keys) {
    /** @type {string[]} */
    const out = [];

    for (let elem of container.getElementsByClassName("settings-group")) {
        if (!(elem instanceof HTMLElement)) {
            continue;
        }

        const schema = elem.dataset["settingsSchemaDel"]
            ?? elem.dataset["settingsSchema"]
            ?? "";
        if (!schema) {
            continue;
        }

        const prefix = "del:";

        getGroupPending(elem)
            .filter((x) => x.startsWith(prefix))
            .map((x) => x.slice(prefix.length))
            .forEach((index) => {
                const elem_keys = schema
                    .split(" ")
                    .map((x) => `${x}${index}`);
                if (!elem_keys.length) {
                    return;
                }

                elem_keys.forEach((key) => {
                    if (!keys.includes(key)) {
                        out.push(key);
                    }
                });
            });
    }

    return [...new Set(out)];
}

/**
 * @param {string | number |boolean} value
 * @returns {DataValue}
 */
function maybeAdjustDataValue(value) {
    if (typeof value === "boolean") {
        return value ? 1 : 0;
    }

    if (typeof value === "string" && (value.length > 0)) {
        const number = Number(value);
        if (!Number.isNaN(number)) {
            return number;
        }
    }

    if (typeof value === "number" && isNaN(value)) {
        return "nan";
    }

    return value;
}

/**
 * besides gathering the data, func is expected to also provide
 * - del: 'cleanup' keys, usually from setting groups that marked certain keys for deletion
 * - set: kvs only for 'changed' keys, or everything available
 * @typedef {{cleanup?: boolean, assumeChanged?: boolean}} GetDataOptions
 */

/**
 * kvs for the device settings storage
 * @typedef {string | number} DataValue
 * @typedef {{[k: string]: DataValue}} SetRequest
 */

/**
 * usually, settings request is sent as a single object
 * @typedef {{set: SetRequest, del: string[]}} DataRequest
 */

/**
 * populates two sets of data, ones that had been changed and ones that stayed the same
 * @param {HTMLFormElement[]} forms
 * @param {GetDataOptions} options
 */
export function getData(forms, {cleanup = true, assumeChanged = false} = {}) {
    /** @type {{[k: string]: DataValue}} */
    const data = {};

    /** @type {string[]} */
    const changed_data = [];

    /** @type {{[k: string]: number}} */
    const group_counter = {};

    /** @type DataRequest */
    const out = {
        set: {
        },
        del: [
        ]
    };

    // TODO: <input type="radio"> can be found as both individual elements and as a `RadioNodeList` view.
    // matching will extract the specific radio element, but will ignore the list b/c it has no tagName
    // TODO: actually use type="radio" in the WebUI to check whether this works
    for (let form of forms) {
        for (let elem of form.elements) {
            if (!(elem instanceof HTMLInputElement) && !(elem instanceof HTMLSelectElement)) {
                continue;
            }

            if (isIgnoredElement(elem)) {
                continue;
            }

            if (elem instanceof HTMLInputElement && elem.readOnly) {
                continue;
            }

            const name = elem.dataset["settingsRealName"] || elem.name;
            if (!name) {
                continue;
            }

            // explicitly attributed, but *could* be determined implicitly by `form.elements[elem.name]` or `FormData::getAll(elem.name)` returing a list
            // - https://developer.mozilla.org/en-US/docs/Web/API/HTMLFormElement/elements
            // - https://developer.mozilla.org/en-US/docs/Web/API/FormData/getAll
            // ts-lint would trigger false positive, though (at least with current version of lib-dom)
            // - https://github.com/microsoft/TypeScript-DOM-lib-generator/issues/1009
            const group_element = isGroupElement(elem);
            const group_index = group_counter[name] || 0;
            const group_name = `${name}${group_index}`;
            if (group_element) {
                group_counter[name] = group_index + 1;
            }

            const value = getDataForElement(elem);
            if (null !== value) {
                const elem_indexed = changed_data.indexOf(name) >= 0;
                if ((isChangedElement(elem) || assumeChanged) && !elem_indexed) {
                    changed_data.push(group_element ? group_name : name);
                }

                // whenever group is involved, make sure its name is used instead
                const data_name = group_element
                    ? group_name : name;

                // fixing outgoing data, when it is necessary
                data[data_name] = maybeAdjustDataValue(value);
            }
        }
    }

    // Finally, filter out only fields that *must* be assigned.
    for (const name of Object.keys(data)) {
        if (assumeChanged || (changed_data.indexOf(name) >= 0)) {
            out.set[name] = data[name];
        }
    }

    // Make sure to remove dynamic group entries from the kvs
    // Only group keys can be removed atm, so only process .settings-group
    if (cleanup) {
        for (let form of forms) {
            out.del.push(...groupSettingsCleanup(form, Object.keys(out.set)));
        }
    }

    return out;
}

// TODO: <input type="radio"> is a special beast, since the actual value is one of 'checked' elements with the same name=... attribute.
// Right now, WebUI does not use this kind of input, but in case it does this needs a once-over that the actual input value is picked up correctly through all of changed / original comparisons.

// Not all of available forms are used for settings:
// - terminal input, which is implemented with an input field. it is attributed with `action="none"`, so settings handler never treats it as 'changed'
// - initial setup. it is shown programatically, but is still available from the global list of forms

/**
 * @param {InputOrSelect} elem
 * @returns {ElementValue}
 */
export function getDataForElement(elem) {
    if (elem instanceof HTMLInputElement) {
        switch (elem.type) {
        case "checkbox":
            return elem.checked;

        case "radio":
            if (elem.checked) {
                return elem.value;
            }

            return null;

        case "text":
        case "password":
        case "hidden":
            return elem.value;

        case "number":
        case "range":
            return elem.valueAsNumber;

        }
    } else if (elem instanceof HTMLSelectElement) {
        if (elem.multiple) {
            return bitsetForSelectElement(elem);
        } else if (elem.selectedIndex >= 0) {
            const option = elem.options[elem.selectedIndex];
            if (option.disabled) {
                return null;
            }

            return option.value;
        }
    }

    return null;
}

/**
 * @param {InputOrSelect} elem
 * @returns {ElementValue}
 */
export function getOriginalForElement(elem) {
    const original = elem.dataset["original"];

    if (elem instanceof HTMLInputElement) {
        switch (elem.type) {
        case "radio":
        case "text":
        case "password":
        case "hidden":
            return original ?? "";

        case "checkbox":
            return (original !== undefined)
                ? stringToBoolean(original)
                : false;

        case "number":
        case "range":
            return (original !== undefined)
                ? parseInt(original)
                : NaN;
        }
    } else if (elem instanceof HTMLSelectElement) {
        if (original === undefined) {
            return "";
        } else if (elem.multiple) {
            return bitsetFromSelectedValues(original.split(","));
        } else {
            return original;
        }
    }

    return null;
}

/**
 * @param {HTMLSelectElement} select
 * @returns {string[]}
 */
function selectedValues(select) {
    if (select.multiple) {
        return Array.from(select.selectedOptions)
            .map(option => option.value);
    } else if (select.selectedIndex >= 0) {
        return [select.options[select.selectedIndex].value];
    }

    return [];
}

// When receiving / returning data, <select multiple=true> <option> values are treated as bitset (u32) indexes (i.e. individual bits that are set)
// For example 0b101 is translated to ["0", "2"], or 0b1111 is translated to ["0", "1", "2", "3"]
// Right now only `hbReport` uses such format, but it is not yet clear how such select element should behave when value is not an integer

/**
 * @param {number} bitset
 * @returns {string[]}
 */
function bitsetToSelectedValues(bitset) {
    let values = [];
    for (let index = 0; index < 31; ++index) {
        if (bitset & (1 << index)) {
            values.push(index.toString());
        }
    }

    return values;
}

/**
 * @param {string[]} values
 * @returns {number}
 */
function bitsetFromSelectedValues(values) {
    let result = 0;
    for (let value of values) {
        result |= 1 << parseInt(value);
    }

    return result;
}

/**
 * @param {HTMLSelectElement} select
 * @returns {number}
 */
function bitsetForSelectElement(select) {
    return bitsetFromSelectedValues(selectedValues(select))
}

/**
 * @param {HTMLInputElement} input
 * @param {ElementValue} value
 */
export function setInputValue(input, value) {
    switch (input.type) {
    case "radio":
        input.checked = (value === input.value);
        break;

    case "checkbox":
        input.checked =
            (typeof value === "boolean") ? value :
            (typeof value === "string") ? stringToBoolean(value) :
            (typeof value === "number") ? (value !== 0) : false;
        break;

    case "number":
    case "range":
        input.valueAsNumber =
            (typeof value === "string") ? parseInt(value) :
            (typeof value === "number") ? value : NaN;
        break;

    case "password":
    case "text":
        input.value =
            (value ?? "").toString();
        break;
    }
}

/**
 * @param {HTMLSpanElement} span
 * @param {ElementValue} value
 * @returns {string}
 */
function prepareSpanValue(span, value) {
    value = value ?? "";
    value = value.toString();

    if (value) {
        value = span.dataset[`value${capitalize(value)}`] ?? value;
    }

    const out = [
        `${span.dataset["pre"] || ""}`,
        `${value}`,
        `${span.dataset["post"] || ""}`,
    ];

    return out.join("");
}

/**
 * @param {HTMLSpanElement} span
 * @param {DisplayValue} value
 */
export function setSpanValue(span, value) {
    if (Array.isArray(value)) {
        /** @type {Node[]} */
        const nodes = [];

        value.forEach((entry) => {
            nodes.push(new Text(prepareSpanValue(span, entry)));
            nodes.push(document.createElement("br"));
        });
        span.replaceChildren(...nodes);
    } else {
        span.textContent = prepareSpanValue(span, value);
    }
}

/**
 * @param {HTMLSelectElement} select
 * @param {ElementValue} value
 */
export function setSelectValue(select, value) {
    /** @type string[] */
    const values = [];

    switch (typeof value) {
    case "boolean":
    case "string":
        values.push(value.toString());
        break;

    case "number":
        if (select.multiple) {
            values.push(...bitsetToSelectedValues(value));
        } else {
            values.push(value.toString());
        }
        break;
    }

    Array.from(select.options)
        .forEach((option) => {
            option.selected = values.includes(option.value);
        });
}

/**
 * @param {InputOrSelect} elem
 */
export function setOriginalFromValue(elem) {
    if (elem instanceof HTMLInputElement) {
        if (elem.readOnly) {
            return;
        }

        if (elem.type === "checkbox") {
            elem.dataset["original"] = elem.checked.toString();
        } else {
            elem.dataset["original"] = elem.value;
        }
    } else if (elem instanceof HTMLSelectElement) {
        elem.dataset["original"] = selectedValues(elem).join(",");
    }

    resetChangedElement(elem);
}

/**
 * @param {InputOrSelect[]} elems
 */
export function setOriginalsFromValues(elems) {
    for (let elem of elems) {
        setOriginalFromValue(elem);
    }
}

/**
 * @param {Element | HTMLElement | DocumentFragment} node
 * @returns {Array<InputOrSelect>}
 */
function findInputOrSelect(node) {
    return Array.from(node.querySelectorAll("input,select"));
}

/**
 * @param {HTMLElement | DocumentFragment} node
 */
export function setOriginalsFromValuesForNode(node) {
    setOriginalsFromValues(findInputOrSelect(node));
}

/**
 * Automatically updates element contents using named entries.
 *
 * Consumer is expected to
 * - set 'data-<NAME>' attribute on the element, either statically or dynamically
 * - install default or custom listener for the respective <NAME>d event
 *
 * Element behaviour varies
 * - <select> recreates <options> with value=$key and labeled with the string contents
 * - <span> inner text is updated with the all of the string contents joined together
 *
 * @typedef {[string, string]} EnumerableTuple
 * @typedef {{[k: string]: string}} EnumerableNames
 */


/** @type {{[k: string]: EnumerableNames}} */
const Enumerable = {};

/** PREFIX + NAME propogated to every "data-enumerable='NAME'" */
const ENUMERABLE_EVENT_PREFIX = 'enumerable-update-';

// <select> initialization from simple {id: ..., name: ...} that map as <option> value=... and textContent
// To avoid depending on order of incoming messages, always store real value inside of dataset["original"] and provide a way to re-initialize every 'enumerable' <select> element on the page
//
// Notice that <select multiple> input and output format is u32 number, but the 'original' string is comma-separated <option> value=... attributes

/**
 * @typedef {{value: string, text: string}} ElementOption
 */

/**
 * @param {HTMLSelectElement | HTMLDataListElement} elem
 * @param {ElementOption[]} options
 */
export function initElementOptions(elem, options) {
    const initial = document.createElement("option");
    initial.disabled = true;
    initial.value = "";

    elem.appendChild(initial);
    if (elem instanceof HTMLSelectElement) {
        elem.selectedIndex = 0;
    }

    for (const option of options) {
        const child = document.createElement("option");
        child.value = option.value;
        child.textContent = option.text;
        elem.appendChild(child);
    }
}

/**
 * @param {EnumerableNames} names
 * @returns {ElementOption[]}
 */
function elementOptionsFromEnumerable(names) {
    /** @type {ElementOption[]} */
    const out = [];

    Object.entries(names)
        .forEach(([id, name]) => {
            out.push({
                "value": id,
                "text": name,
            });
        });

    return out;
}

/**
 * @param {HTMLElement} elem
 */
function cleanupChildElements(elem) {
    while (elem.childElementCount && elem.firstElementChild) {
        elem.removeChild(elem.firstElementChild);
    }
}

/**
 * @param {HTMLSelectElement} select
 * @param {EnumerableNames} names
 */
function onEnumerableUpdateSelect(select, names) {
    cleanupChildElements(select);
    initElementOptions(select, elementOptionsFromEnumerable(names));

    const original = getOriginalForElement(select);
    if (original !== null) {
        setSelectValue(select, original);
    }
}

/**
 * @param {HTMLSpanElement} span
 * @param {EnumerableNames} names
 */
function onEnumerableUpdateSpan(span, names) {
    const id = span.dataset["enumerableId"] ?? "";
    if (id.length === 0) {
        return;
    }

    const name = names[id];
    if (!name) {
        return;
    }

    setSpanValue(span, name);
}

/**
 * @param {HTMLDataListElement} datalist
 * @param {EnumerableNames} names
 */
function onEnumerableUpdateDataList(datalist, names) {
    cleanupChildElements(datalist);
    initElementOptions(datalist, elementOptionsFromEnumerable(names));
}

/**
 * @callback EnumerableElemCallback
 * @param {HTMLElement} elem
 * @param {EnumerableNames} names
 * @returns {void}
 */

/**
 * @type {EnumerableElemCallback}
 */
function onEnumerableUpdateElem(elem, names) {
    if (elem instanceof HTMLSelectElement) {
        onEnumerableUpdateSelect(elem, names);
    } else if (elem instanceof HTMLSpanElement) {
        onEnumerableUpdateSpan(elem, names);
    } else if (elem instanceof HTMLDataListElement) {
        onEnumerableUpdateDataList(elem, names);
    }
}

/**
 * @param {Event} event
 * @param {EnumerableElemCallback} callback
 */
function onEnumerableUpdate(event, callback) {
    const elem = /** @type {!HTMLElement} */(event.target);
    const enumerables = /** @type {CustomEvent<{enumerables: EnumerableNames}>} */
        (event).detail.enumerables;
    callback(elem, enumerables);
}

/**
 * @param {string} name
 * @param {EnumerableNames} enumerables
 */
function notifyEnumerables(name, enumerables) {
    document.querySelectorAll(`[data-enumerable=${name}]`)
        .forEach((elem) => {
            if (!(elem instanceof HTMLElement)) {
                return;
            }

            elem.dispatchEvent(
                new CustomEvent(
                    `${ENUMERABLE_EVENT_PREFIX}${name}`,
                    {detail: {enumerables}}));
        });
}

/**
 * @param {HTMLElement} elem
 * @param {string} name
 * @param {EnumerableElemCallback?} callback
 */
export function listenEnumerableName(elem, name, callback = null) {
    callback = callback ?? onEnumerableUpdateElem;
    elem.addEventListener(
        `${ENUMERABLE_EVENT_PREFIX}${name}`,
        (event) => onEnumerableUpdate(event, callback));

    const current = Enumerable[name];
    if (!current) {
        return;
    }

    callback(elem, current);
}

/**
 * @param {HTMLElement} elem
 * @param {number} id
 * @param {string} name
 */
export function prepareEnumerableTarget(elem, id, name) {
    elem.dataset["enumerableId"] = id.toString();
    elem.dataset["enumerable"] = name;
}

/**
 * @param {HTMLElement} elem
 * @param {number} id
 * @param {string} name
 * @param {EnumerableElemCallback?} callback
 */
export function listenEnumerableTarget(elem, id, name, callback = null) {
    const span = document.createElement("span");
    prepareEnumerableTarget(span, id, name);
    listenEnumerableName(span, name, callback);
    elem.appendChild(span);
}

/**
 * @param {HTMLElement} elem
 * @param {EnumerableElemCallback?} callback
 */
export function listenEnumerable(elem, callback = null) {
    const name = elem.dataset["enumerable"];
    if (!name) {
        return;
    }

    listenEnumerableName(elem, name, callback);
}

/**
 * @param {string} name
 * @returns {EnumerableNames}
 */
export function getEnumerables(name) {
    return Enumerable[name] ?? {};
}

/**
 * @param {string} name
 * @param {EnumerableNames | EnumerableTuple[]} enumerables
 */
export function addEnumerables(name, enumerables) {
    /** @type {EnumerableNames} */
    let names = {};

    if (Array.isArray(enumerables)) {
        enumerables.forEach(([id, name]) => {
            names[id] = name;
        });
    } else {
        names = enumerables;
    }

    Enumerable[name] = names;
    notifyEnumerables(name, names);
}

/**
 * @param {string} name
 * @param {string} prettyName
 * @param {number} count
 */
export function addSimpleEnumerables(name, prettyName, count) {
    if (count <= 0) {
        return;
    }

    /** @type {EnumerableNames} */
    const names = {};
    for (let id = 0; id < count; ++id) {
        names[id.toString()] = `${prettyName} #${id}`;
    }

    addEnumerables(name, names);
}

// track <input> values, count total number of changes and their side-effects / needed actions
class SettingsBase {
    constructor() {
        this.counters = {
            changed: 0,
            reboot: 0,
            reconnect: 0,
            reload: 0,
        };
        this.saved = false;
    }

    /**
     * @param {number} count
     * @param {string?} action
     */
    countFor(count, action = null) {
        this.counters.changed += count;
        if (typeof action === "string") {
            switch (action) {
            case "reboot":
            case "reload":
            case "reconnect":
                this.counters[action] += count;
                break;
            }
        }
    }

    /**
     * @param {string?} action
     */
    increment(action = null) {
        this.countFor(1, action);
    }

    /**
     * @param {string?} action
     */
    decrement(action = null) {
        this.countFor(-1, action);
    }

    resetChanged() {
        this.counters.changed = 0;
    }

    resetCounters() {
        this.counters.changed = 0;
        this.counters.reboot = 0;
        this.counters.reconnect = 0;
        this.counters.reload = 0;
    }

    stylizeSave() {
        const changed = this.counters.changed !== 0;
        document.querySelectorAll(".button-save")
            .forEach((elem) => {
                if (!(elem instanceof HTMLElement)) {
                    return;
                }

                if (changed) {
                    elem.style.setProperty("--save-background", "rgb(0, 192, 0)");
                } else {
                    elem.style.removeProperty("--save-background");
                }
            });
    }
}

/**
 * read-only kv pairs. currently, this is span with a data-key=$key
 * @param {Document | Element} node
 * @param {string} key
 * @param {DisplayValue} value
 */
export function setSpanValueByKey(node, key, value) {
    for (const span of node.querySelectorAll(`span[data-key='${key}']`)) {
        if (!(span instanceof HTMLSpanElement)) {
            continue;
        }

        setSpanValue(span, value);
    }
}

/**
 * @param {InputOrSelect} elem
 * @param {ElementValue} value
 */
function setInputOrSelect(elem, value) {
    if (elem instanceof HTMLInputElement) {
        setInputValue(elem, value);
    } else if (elem instanceof HTMLSelectElement) {
        setSelectValue(elem, value);
    }
}

/**
 * handle plain kv pairs when they are already on the page, and don't need special template handlers
 * @param {Document | Element} node
 * @param {string} key
 * @param {ElementValue} value
 */
export function setInputOrSelectValueByKey(node, key, value) {
    const inputs = [];

    for (const elem of node.querySelectorAll(`[name='${key}'`)) {
        if ((elem instanceof HTMLInputElement)
         || (elem instanceof HTMLSelectElement))
        {
            if (isGroupElement(elem)) {
                continue;
            }

            setInputOrSelect(elem, value);
            inputs.push(elem);
        }
    }

    setOriginalsFromValues(inputs);
}

const Settings = new SettingsBase();

/**
 * @param {InputOrSelect} elem
 * @returns {boolean}
 */
function checkElementChanged(elem) {
    const lhs = getOriginalForElement(elem);
    const rhs = getDataForElement(elem);

    if (typeof lhs === "number"
     && typeof rhs === "number"
     && isNaN(lhs)
     && isNaN(rhs))
    {
        return false;
    }

    return lhs !== rhs;
}

/**
 * @param {InputOrSelect} elem
 * @returns {boolean}
 */
export function checkAndSetElementChanged(elem) {
    const changed = isChangedElement(elem);

    if (checkElementChanged(elem)) {
        setChangedElement(elem);
    } else {
        resetChangedElement(elem);
    }

    return changed !== isChangedElement(elem);
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

    if (isChangedElement(target)) {
        Settings.increment(action);
    } else {
        Settings.decrement(action);
    }

    if (target.required) {
        target.reportValidity();
    }

    Settings.stylizeSave();
}

/**
 * @typedef {function(string, any): void} KeyValueListener
 */

/**
 * @typedef {{[k: string]: KeyValueListener}} KeyValueListeners
 */

/**
 * @type {{[k: string]: KeyValueListener[]}}
 */
const __variable_listeners = {};

/**
 * @param {string} key
 * @param {KeyValueListener} func
 */
export function listenVariables(key, func) {
    if (__variable_listeners[key] === undefined) {
        __variable_listeners[key] = [];
    }

    __variable_listeners[key].push(func);
}

/**
 * @param {KeyValueListeners} listeners
 */
export function variableListeners(listeners) {
    for (const [key, listener] of Object.entries(listeners)) {
        listenVariables(key, listener);
    }
}

/**
 * @param {string} key
 * @param {any} value
 */
export function updateKeyValue(key, value) {
    const listeners = __variable_listeners[key];
    if (listeners !== undefined) {
        for (let listener of listeners) {
            listener(key, value);
        }
    }

    if (typeof value === "object") {
        return;
    }

    setSpanValueByKey(document, key, value);
    setInputOrSelectValueByKey(document, key, value);
}

function resetOriginals() {
    setOriginalsFromValuesForNode(document.documentElement);
    resetSettingsGroup();

    Settings.resetCounters();
    Settings.saved = false;
}

function afterSaved() {
    let response = false;

    if (Settings.counters.reboot > 0) {
        response = window.confirm("You have to reboot the board for the changes to take effect, do you want to do it now?");
        if (response) {
            sendAction("reboot");
        }
    } else if (Settings.counters.reconnect > 0) {
        response = window.confirm("You have to reconnect to the WiFi for the changes to take effect, do you want to do it now?");
        if (response) {
            sendAction("reconnect");
        }
    } else if (Settings.counters.reload > 0) {
        response = window.confirm("You have to reload the page to see the latest changes, do you want to do it now?");
        if (response) {
            pageReloadIn(0);
        }
    }

    resetOriginals();
    Settings.stylizeSave();
}

function waitForSaved(){
    if (!Settings.saved) {
        setTimeout(waitForSaved, 1000);
    } else {
        afterSaved();
    }
}

/** @param {any} settings */
export function applySettings(settings) {
    send(JSON.stringify({settings}));
}

/** @param {HTMLFormElement[]} forms */
export function applySettingsFromForms(forms) {
    applySettings(getData(forms));
    Settings.resetChanged();
    waitForSaved();
}

/**
 * @param {HTMLFormElement[]} forms
 * @returns {boolean}
 */
function validateForms(forms) {
    return validateFormsReportValidity(forms)
        && validateFormsPasswords(forms, {strict: false});
}

/** @param {Event} event */
function applySettingsFromAllForms(event) {
    event.preventDefault();

    const elems = /** @type {NodeListOf<HTMLFormElement>} */
        (document.querySelectorAll("form.form-settings"));

    const forms = Array.from(elems);
    if (validateForms(forms)) {
        applySettingsFromForms(forms);
    }
}

/** @param {Event} event */
function resetToFactoryDefaults(event) {
    event.preventDefault();

    if (window.confirm("Are you sure you want to erase all settings from the device?")) {
        sendAction("factory_reset");
    }
}

/** @param {Event} event */
function handleSettingsFile(event) {
    event.preventDefault();

    const target = event.target;
    if (!(target instanceof HTMLInputElement)) {
        return;
    }

    const inputFiles = target.files;
    if (!inputFiles || inputFiles.length === 0) {
        return false;
    }

    const inputFile = inputFiles[0];
    target.value = "";

    if (!window.confirm("Previous settings will be overwritten. Are you sure you want to restore from this file?")) {
        return false;
    }

    const reader = new FileReader();
    reader.onload = function(event) {
        try {
            const data = event.target?.result;
            if (!data) {
                throw new Error(`${event.target} is missing data payload`);
            }

            if (data instanceof ArrayBuffer) {
                throw new Error("invalid payload type - ArrayBuffer");
            }

            sendAction("restore", JSON.parse(data));
        } catch (e) {
            notifyError(/** @type {Error} */(e));
        }
    };
    reader.readAsText(inputFile);
}

/** @returns {boolean} */
export function pendingChanges() {
    return Settings.counters.changed > 0;
}

/** @type {import("./question.mjs").QuestionWrapper} */
export function askSaveSettings(ask) {
    if (pendingChanges()) {
        return ask("There are pending changes to the settings, continue the operation without saving?");
    }

    return true;
}

/** @returns {KeyValueListeners} */
function listeners() {
    return {
        "saved": (_, value) => {
            Settings.saved = value;
        },
    };
}

/** @param {{[k: string]: any}} kvs */
export function updateVariables(kvs) {
    Object.entries(kvs)
        .forEach(([key, value]) => {
            updateKeyValue(key, value);
        });
}

export function init() {
    variableListeners(listeners());

    document.getElementById("uploader")
        ?.addEventListener("change", handleSettingsFile);

    document.querySelector(".button-save")
        ?.addEventListener("click", applySettingsFromAllForms);

    const backup = document.querySelector(".button-settings-backup");
    if (backup instanceof HTMLButtonElement) {
        listenAppConnected((urls) => {
            backup.dataset["url"] = urls.config.href;
        });

        backup.addEventListener("click", (event) => {
            event.preventDefault();

            const url = backup.dataset["url"];
            if (!url) {
                alert("Not connected");
                return;
            }

            const elem = document.getElementById("downloader");
            if (elem instanceof HTMLAnchorElement) {
                elem.href = url;
                elem.click();
            }
        });
    }

    document.querySelector(".button-settings-restore")
        ?.addEventListener("click", () => {
            document.getElementById("uploader")?.click();
        });
    document.querySelector(".button-settings-factory")
        ?.addEventListener("click", resetToFactoryDefaults);

    document.querySelector(".button-settings-password")
        ?.addEventListener("click", () => {
            showPanelByName("password");
        });

    document.querySelectorAll(".button-add-settings-group")
        .forEach((elem) => {
            elem.addEventListener("click", onGroupSettingsAddClick);
        });

    // aka elements that already have "dataset['enumerable']" set
    // most likely, merged static .html contains this reference
    document.querySelectorAll("[data-enumerable]")
        .forEach((elem) => {
            if (!(elem instanceof HTMLElement)) {
                return;
            }

            listenEnumerable(elem);
        });

    // No group handler should be registered after this point, since we depend on the order
    // of registration to trigger 'after-add' handler and update group attributes *after*
    // module function finishes modifying the container
    for (const group of document.querySelectorAll(".settings-group")) {
        group.addEventListener("settings-group-del", onGroupSettingsEventDel);
        group.addEventListener("change", onElementChange);
    }

    for (let elem of document.querySelectorAll("input,select")) {
        elem.addEventListener("change", onElementChange);
    }

    resetOriginals();
}
