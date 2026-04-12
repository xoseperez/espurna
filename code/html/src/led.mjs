import { addEnumerables, addSimpleEnumerables } from './settings/enumerable.mjs';
import { groupSettingsOnAddElem } from './settings/group.mjs';

import { addFromTemplate, addOriginalsFromTemplate } from './template.mjs';
import { variableListeners } from './settings.mjs';

/** @param {function(HTMLElement): void} callback */
function withLeds(callback) {
    callback(/** @type {!HTMLElement} */
        (document.getElementById("leds")));
}

const TEMPLATE_NAME = "led-config";

/**
 * @param {HTMLElement} elem
 */
function addLed(elem) {
    addFromTemplate(elem, TEMPLATE_NAME, {});
}

/**
 * @param {any} value
 */
function onConfig(value) {
    withLeds((elem) => {
        addOriginalsFromTemplate(
            elem, TEMPLATE_NAME,
            {
                entries: value.leds,
                schema: value.schema,
                max: value.max ?? 0
            });
    });
    addSimpleEnumerables("led", "LED", value.leds.length);
}

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "ledConfig": (_, value) => {
            onConfig(value);
        },
        "ledModes": (_, value) => {
            addEnumerables("ledMode", value);
        },
    };
};

export function init() {
    withLeds((elem) => {
        variableListeners(listeners());
        groupSettingsOnAddElem(elem, () => {
            addLed(elem);
        });
    });
}
