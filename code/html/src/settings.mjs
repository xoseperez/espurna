import {
    pageReloadIn,
    showPanelByName,
} from './core.mjs';

import { askAndCall } from './question.mjs';

import {
    listenAppConnected,
    send,
    sendAction,
} from './connection.mjs';

import {
    validateFormsPasswords,
    validateFormsReportValidity,
} from './validate.mjs';

import { notifyError } from './notify.mjs';

import {
    cleanupDataset,
    isChangedElement,
    listenPendingChanges,
    makeDataRequest,
    setOriginalFromValue,
    setOriginalsFromValues,
} from './settings/dataset.mjs';

import { setInputValue } from './settings/input.mjs';
import { setSelectValue } from './settings/select.mjs';
import { setSpanValue } from './settings/span.mjs';

import { onElementChange } from './settings/change.mjs';
import { listenEnumerable } from './settings/enumerable.mjs';

import { findInputOrSelect } from './settings/utils.mjs';

import { isGroupElement, SETTINGS_GROUP } from './settings/group.elem.mjs';
import {
    onGroupSettingsAddClick,
    onGroupSettingsEventDel,
} from './settings/group.mjs';

import { MODULE_DEV } from '@build-preset/constants.mjs';

/**
 * @typedef { HTMLInputElement | HTMLSelectElement } InputOrSelect
 */

/** @import { DisplayValue } from './settings/span.mjs' */
/** @import { ElementValue } from './settings/value.mjs' */

/** @param {boolean} changed */
function stylizeSave(changed) {
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

/** @type {{saved: boolean, forms: HTMLFormElement[]}} */
const __pending_request = {
    saved: true,
    forms: [],
};

const __after_save = {
    reboot: {
        message: "You have to reboot the board for the changes to take effect, do you want to do it now?",
        action: () => sendAction("reboot"),
    },
    reload: {
        message: "You have to reload the page to see the latest changes, do you want to do it now?",
        action: () => pageReloadIn(0),
    },
    reconnect: {
        message: "You have to reconnect to the WiFi for the changes to take effect, do you want to do it now?",
        action: () => sendAction("reconnect"),

    },
};

/** @param {HTMLFormElement[]} forms */
function afterSavedAction(forms) {
    /** @type {(function(): void)?} */ 
    let once = null;

    for (const form of forms) {
        for (const elem of form.elements) {
            if (!(elem instanceof HTMLInputElement)
             && !(elem instanceof HTMLSelectElement))
            {
                continue;
            }

            if (!isChangedElement(elem)) {
                continue;
            }

            if (!once) {
                const action = elem.dataset["action"] ?? "";
                switch (action) {
                case "reboot":
                case "reload":
                case "reconnect":
                    const after = __after_save[action];
                    if (after !== undefined) {
                        const { message, action } = after;
                        once = () => { askAndCall([(ask) => ask(message)], action) };
                    }

                    break;
                }
            }

            setOriginalFromValue(elem);
        }
    }

    if (once) {
        once();
    }
}

/** @param {HTMLFormElement[]} forms */
function afterSavedCleanup(forms) {
    for (const form of forms) {
        cleanupDataset(form.id);
    }
}

/** @param {HTMLFormElement[]?} forms */
function afterSaved(forms) {
    if (forms !== null) { 
        afterSavedAction(forms);
        afterSavedCleanup(forms);
    }
}

function waitForSaved(){
    if (!__pending_request.saved) {
        setTimeout(waitForSaved, 1000);
    } else {
        afterSaved(__pending_request.forms);
        __pending_request.forms = [];
    }
}

/**
 * @param {HTMLFormElement[]} forms
 */
export function applySettingsFromForms(forms) {
    if (__pending_request.saved && __pending_request.forms.length === 0) {
        const settings = makeDataRequest(forms);

        send(JSON.stringify({settings}));

        __pending_request.saved = MODULE_DEV;
        __pending_request.forms = forms;

        waitForSaved();
    }
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
function applySettingsFromDataset(event) {
    event.preventDefault();

    const elems = /** @type {NodeListOf<HTMLFormElement>} */
        (document.querySelectorAll("form.form-settings"));

    const forms = Array.from(elems);
    if (!validateForms(forms)) {
        return;
    }

    applySettingsFromForms(forms);
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

let __pending_changes = false;

/** @type {import("./question.mjs").QuestionWrapper} */
export function askSaveSettings(ask) {
    if (__pending_changes) {
        return ask("There are pending changes to the settings, continue the operation without saving?");
    }

    return true;
}

/** @returns {KeyValueListeners} */
function listeners() {
    return {
        "saved": (_, value) => {
            if (typeof value === "boolean") {
                __pending_request.saved = value;
            } else {
                __pending_request.saved = true;
            }
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
        ?.addEventListener("click", applySettingsFromDataset);

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
    for (const group of document.querySelectorAll(`.${SETTINGS_GROUP}`)) {
        group.addEventListener("settings-group-del", onGroupSettingsEventDel);
        group.addEventListener("change", onElementChange);
    }

    for (const elem of findInputOrSelect(document.documentElement)) {
        elem.addEventListener("change", onElementChange);
    }

    listenPendingChanges(stylizeSave);
    listenPendingChanges((value) => {
        __pending_changes = value;
    });

    stylizeSave(false);
}
