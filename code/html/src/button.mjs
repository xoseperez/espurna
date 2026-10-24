import { addOriginalsFromTemplateWithPreparedSchema, prepareFromSchema } from './template.mjs';
import { variableListeners } from './settings.mjs';

/**
 * Button inputs, currently only AC mains switch (frequency detection) options.
 * Values are reported by the device on connection as `btnConfig`,
 * `btnFreq#`, `btnMinPulses#`, `btnMinDuty#`, `btnAcConfirm#` and `btnDebDel#` are saved with the button index suffix.
 * Filter options are only shown for the inputs marked as AC mains switch,
 * global AC options (`btnAcFreq`, `btnAcDbg`) only when there is at least one such input.
 */

/**
 * @param {Element} node
 * @returns {boolean}
 */
function isAcSwitch(node) {
    const input = node.querySelector("input[name='btnFreq']");
    return (input instanceof HTMLInputElement) && input.checked;
}

/** @param {Element} node */
function updateFilterVisibility(node) {
    const filter = node.querySelector(".button-ac-filter");
    if (filter instanceof HTMLElement) {
        filter.hidden = !isAcSwitch(node);
    }
}

/** @param {HTMLElement} container */
function updateGlobalVisibility(container) {
    const global = document.getElementById("buttonAcGlobal");
    if (global) {
        global.hidden = !Array.from(container.children).some(isAcSwitch);
    }
}

/** @param {HTMLElement} container */
function updateVisibility(container) {
    for (const node of container.children) {
        updateFilterVisibility(node);
    }

    updateGlobalVisibility(container);
}

/**
 * @param {any[]} values
 * @param {string[]} schema
 */
function updateFromConfig(values, schema) {
    const container = document.getElementById("buttonConfig");
    if (!container || container.childElementCount > 0) {
        return;
    }

    const prepared = prepareFromSchema(values, schema);
    if (!prepared.length) {
        return;
    }

    addOriginalsFromTemplateWithPreparedSchema(
        container, "button-config", prepared);

    container.addEventListener("change", (event) => {
        const target = event.target;
        if ((target instanceof HTMLInputElement) && (target.name === "btnFreq")) {
            updateVisibility(container);
        }
    });

    updateVisibility(container);
    container.closest("fieldset")?.removeAttribute("hidden");
}

/** @returns {import('./settings.mjs').KeyValueListeners} */
function listeners() {
    return {
        "btnConfig": (_, value) => {
            updateFromConfig(value.values, value.schema);
        },
    };
}

export function init() {
    variableListeners(listeners());
}
