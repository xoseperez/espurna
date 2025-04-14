import { sendAction } from './connection.mjs';

import {
    addFromTemplate,
    fromSchema,
    NumberInput,
} from './template.mjs';

import {
    addEnumerables,
    setOriginalsFromValues,
    variableListeners,
    listenEnumerableTarget,
} from './settings.mjs';

import {
    loadTemplate,
    mergeTemplate,
} from './settings/template.mjs';

/** @param {Event} event */
function onToggle(event) {
    event.preventDefault();

    const target = /** @type {!HTMLInputElement} */(event.target);
    const id = target.dataset["id"];
    if (!id) {
        return;
    }

    sendAction("relay", {
        id: parseInt(id, 10),
        status: target.checked ? "1" : "0"});
}

/**
 * @param {number} id
 */
function initToggle(id) {
    const container = document.getElementById("relays");
    if (!container) {
        return;
    }

    const line = loadTemplate("relay-control");

    const root = /** @type {!HTMLDivElement} */
        (line.querySelector("div"));
    root.classList.add(`relay-control-${id}`);

    const realId = `relay${id}`;

    const [label] = line.querySelectorAll("label");
    label.setAttribute("for", realId);

    listenEnumerableTarget(label, id, "relay");

    const toggle = /** @type {!HTMLInputElement} */
        (line.querySelector("input[type='checkbox']"));
    toggle.checked = false;
    toggle.disabled = true;
    toggle.dataset["id"] = id.toString();
    toggle.addEventListener("change", onToggle);

    toggle.setAttribute("id", realId);
    toggle.previousElementSibling?.setAttribute("for", realId);

    mergeTemplate(container, line);
}

/**
 * @param {any[]} states
 * @param {string[]} schema
 */
function updateFromState(states, schema) {
    states.forEach((state, id) => {
        const elem = /** @type {!HTMLInputElement} */
            (document.querySelector(`input[name='relay'][data-id='${id}']`));

        const relay = fromSchema(state, schema);

        const status = /** @type {boolean} */(relay.status);
        elem.checked = status;

        // TODO: publish possible enum values in ws init
        const as_lock = new Map();
        as_lock.set(0, false);
        as_lock.set(1, !status);
        as_lock.set(2, status);

        const lock = /** @type {number} */(relay.lock);
        if (as_lock.has(lock)) {
            elem.disabled = as_lock.get(lock);
        }
    });
}

/** @param {any} cfg */
function addConfigNode(cfg) {
    const container = /** @type {!HTMLElement} */
        (document.getElementById("relayConfig"));
    addFromTemplate(container, "relay-config", cfg);
}

/**
 * @param {any[]} configs
 * @param {string[]} schema
 */
function updateFromConfig(configs, schema) {
    const container = document.getElementById("relays");
    if (!container || container.childElementCount > 0) {
        return;
    }

    /** @import { EnumerableNames } from './settings.mjs' */

    /** @type {EnumerableNames} */
    const names = {};

    configs.forEach((config, id) => {
        const relay = fromSchema(config, schema);
        if (!relay.relayName) {
            relay.relayName = `Switch #${id}`;
        }

        names[id.toString()] =
            `${relay.relayName} (${relay.relayProv})`;

        initToggle(id);
        addConfigNode(relay);
    });

    addEnumerables("relay", names);
}

/**
 * @param {string} id
 * @param {any[]} values
 * @param {string} keyPrefix
 */
export function createNodeList(id, values, keyPrefix) {
    const container = document.getElementById(id);
    if (!container || container.childElementCount > 0) {
        return;
    }

    // TODO generic template to automatically match elems to (limited) schema?
    const template = new NumberInput();
    values.forEach((value, index) => {
        mergeTemplate(container, template.with(
            (label, input) => {
                listenEnumerableTarget(label, index, "relay");

                input.name = keyPrefix;
                input.value = value;
                setOriginalsFromValues([input]);
            }));
        });
}

/** @returns {import('./settings.mjs').KeyValueListeners} */
function listeners() {
    return {
        "relayConfig": (_, value) => {
            updateFromConfig(value.values, value.schema);
        },
        "relayState": (_, value) => {
            updateFromState(value.values, value.schema);
        },
    };
}

export function init() {
    variableListeners(listeners());
}
