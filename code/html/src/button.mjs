import { prepareFromSchema } from './template.mjs';
import { variableListeners } from './settings.mjs';
import { stringToBoolean } from './core.mjs';

import { setOriginalsFromValuesForNode } from './settings/dataset.mjs';
import { setGroupCleanup, setGroupElement } from './settings/group.elem.mjs';
import { setInputValue } from './settings/input.mjs';
import { setSelectValue } from './settings/select.mjs';
import { loadTemplate } from './settings/template.mjs';
import { findInputOrSelect } from './settings/utils.mjs';

/**
 * Button inputs, currently only AC mains switch (frequency detection) options.
 * Device reports them on connection as `btnConfig` (schema + values, one entry per button).
 * Options of the button controlling a relay (`btnRelay`) are shown in that relay card,
 * the rest is shown in the separate 'Other buttons' block.
 * Every field is saved with the *button* index suffix (`btnFreq#`, `btnMinPulses#`, ...),
 * which is set explicitly as `data-key` and therefore does not depend on the element position.
 */

/** @import { TemplateConfig } from './template.mjs' */

/**
 * @typedef {{
 *   buttons: TemplateConfig[] | null,
 *   relays: TemplateConfig[] | null,
 *   nodes: Map<number, HTMLElement>,
 *   fallbacks: Map<number, HTMLElement>,
 * }} ButtonState
 */

/** @type {ButtonState} */
const state = {
    buttons: null,
    relays: null,
    nodes: new Map(),
    fallbacks: new Map(),
};

/**
 * @param {HTMLElement} node
 * @returns {HTMLSelectElement | null}
 */
function typeSelect(node) {
    return node.querySelector("select[name='btnFreq']");
}

/** @param {HTMLElement} node */
function updateFilterVisibility(node) {
    const filter = node.querySelector(".button-ac-filter");
    if (filter instanceof HTMLElement) {
        filter.hidden = typeSelect(node)?.value !== "1";
    }
}

function updateGlobalVisibility() {
    const global = document.getElementById("buttonAcGlobal");
    if (!global) {
        return;
    }

    const any = Array.from(state.nodes.values())
        .some((node) => node.isConnected && (typeSelect(node)?.value === "1"));
    global.hidden = !any;
}

/**
 * @param {number} index
 * @param {TemplateConfig} cfg
 * @returns {HTMLElement}
 */
function createNode(index, cfg) {
    const fragment = loadTemplate("button-config");
    const node = /** @type {HTMLElement} */(fragment.firstElementChild);
    node.dataset["button"] = index.toString();

    for (const elem of findInputOrSelect(node)) {
        const name = elem.name;
        // explicit name, so that neither relay card position nor fallback block affects the suffix
        elem.dataset["key"] = `${name}${index}`;
        // plain kv updates (updateKeyValue) must not touch these
        setGroupElement(elem);

        const value = cfg[name];
        if ((value === undefined) || (value === null)) {
            continue;
        }

        if (elem instanceof HTMLSelectElement) {
            const ac = (typeof value === "boolean") ? value
                : (typeof value === "number") ? (value !== 0)
                : stringToBoolean(String(value));
            setSelectValue(elem, ac ? "1" : "0");
        } else if (elem instanceof HTMLInputElement) {
            setInputValue(elem, value);
        }
    }

    typeSelect(node)?.addEventListener("change", () => {
        updateFilterVisibility(node);
        updateGlobalVisibility();
    });

    updateFilterVisibility(node);
    return node;
}

/**
 * relay index -> button index; when several buttons control the same relay,
 * prefer the one with the GPIO configured as the relay 'Button GPIO'
 * @param {TemplateConfig[]} buttons
 * @param {TemplateConfig[]} relays
 * @param {HTMLElement | null} container
 * @returns {Map<number, number>}
 */
function assignToRelays(buttons, relays, container) {
    /** @type {Map<number, number>} */
    const out = new Map();
    if (!container) {
        return out;
    }

    relays.forEach((relay, relayIndex) => {
        if (!container.children[relayIndex]) {
            return;
        }

        const candidates = buttons
            .map((cfg, index) => ({cfg, index}))
            .filter(({cfg}) => Number(cfg.btnRelay) === relayIndex);
        if (!candidates.length) {
            return;
        }

        const gpio = String(relay.relayBtnGpio ?? "");
        const match = candidates.find(({cfg}) => String(cfg.btnGpio) === gpio)
            ?? candidates[0];
        out.set(relayIndex, match.index);
    });

    return out;
}

/**
 * @param {HTMLElement} card
 * @param {HTMLElement} node
 */
function placeIntoCard(card, node) {
    if (node.parentElement && card.contains(node)) {
        return;
    }

    // deleting the last relay card must only remove relay keys, not the button ones
    for (const elem of findInputOrSelect(card)) {
        setGroupCleanup(elem);
    }

    const anchor = card.querySelector("[name='relayBtnGpio']")
        ?.closest(".pure-control-group");
    if (anchor) {
        anchor.after(node);
    } else {
        (card.querySelector("fieldset") ?? card).appendChild(node);
    }
}

/**
 * @param {number} index
 * @param {TemplateConfig} cfg
 * @param {HTMLElement} container
 * @param {HTMLElement} node
 */
function placeIntoFallback(index, cfg, container, node) {
    let wrapper = state.fallbacks.get(index);
    if (!wrapper) {
        const fragment = loadTemplate("button-fallback");
        wrapper = /** @type {HTMLElement} */(fragment.firstElementChild);

        const id = wrapper.querySelector(".button-id");
        if (id) {
            id.textContent = index.toString();
        }

        const gpio = wrapper.querySelector(".button-gpio");
        if (gpio) {
            gpio.textContent = String(cfg.btnGpio ?? "");
        }

        container.appendChild(wrapper);
        state.fallbacks.set(index, wrapper);
    }

    if (!wrapper.contains(node)) {
        (wrapper.querySelector("fieldset") ?? wrapper).appendChild(node);
    }
}

/** @param {number} index */
function removeFallback(index) {
    state.fallbacks.get(index)?.remove();
    state.fallbacks.delete(index);
}

function render() {
    const buttons = state.buttons;
    if (!buttons) {
        return;
    }

    const relayContainer = document.getElementById("relayConfig");
    const assigned = assignToRelays(buttons, state.relays ?? [], relayContainer);

    /** @type {Map<number, number>} */
    const buttonToRelay = new Map();
    for (const [relay, button] of assigned) {
        buttonToRelay.set(button, relay);
    }

    const fallbackContainer = document.getElementById("buttonConfig");

    buttons.forEach((cfg, index) => {
        let node = state.nodes.get(index);
        if (!node) {
            node = createNode(index, cfg);
            state.nodes.set(index, node);
        }

        const relay = buttonToRelay.get(index);
        const card = (relay !== undefined)
            ? /** @type {HTMLElement | undefined} */(relayContainer?.children[relay])
            : undefined;

        if (card) {
            placeIntoCard(card, node);
            removeFallback(index);
        } else if (fallbackContainer) {
            placeIntoFallback(index, cfg, fallbackContainer, node);
        } else {
            return;
        }

        // originals need the form, so only once the node is in the DOM
        if (!node.dataset["originals"]) {
            setOriginalsFromValuesForNode(node);
            node.dataset["originals"] = "1";
        }
    });

    const other = document.getElementById("buttonOther");
    if (other) {
        other.hidden = state.fallbacks.size === 0;
    }

    updateGlobalVisibility();
}

/**
 * @param {any[]} values
 * @param {string[]} schema
 */
function updateFromConfig(values, schema) {
    if (state.buttons) {
        return;
    }

    const prepared = prepareFromSchema(values, schema);
    if (!prepared.length) {
        return;
    }

    state.buttons = prepared;
    render();
}

/**
 * @param {any[]} values
 * @param {string[]} schema
 */
function updateFromRelayConfig(values, schema) {
    if (state.relays) {
        return;
    }

    state.relays = prepareFromSchema(values, schema);
    render();
}

/** @returns {import('./settings.mjs').KeyValueListeners} */
function listeners() {
    return {
        "btnConfig": (_, value) => {
            updateFromConfig(value.values, value.schema);
        },
        // relay.mjs listener is registered first (see index.mjs), so relay cards already exist here
        "relayConfig": (_, value) => {
            updateFromRelayConfig(value.values, value.schema);
        },
    };
}

export function init() {
    variableListeners(listeners());
}
