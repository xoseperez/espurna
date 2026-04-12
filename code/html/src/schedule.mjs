import { addEnumerables } from './settings/enumerable.mjs';
import { groupSettingsOnAddElem } from './settings/group.mjs';
import { variableListeners } from './settings.mjs';

import { capitalize } from './core.mjs';
import { addFromTemplate, addOriginalsFromTemplate } from './template.mjs';
import { reportValidityForInputOrSelect } from './validate/utils.mjs';

/** @param {function(HTMLElement): void} callback */
function withSchedules(callback) {
    callback(/** @type {!HTMLElement} */
        (document.getElementById("schedules")));
}

const TEMPLATE_NAME = "schedule-config";

/**
 * @param {HTMLElement} elem
 */
function scheduleAdd(elem) {
    addFromTemplate(elem, TEMPLATE_NAME, {});
}

/**
 * @param {any} value
 */
function onConfig(value) {
    withSchedules((elem) => {
        addOriginalsFromTemplate(
            elem, TEMPLATE_NAME,
            {
                entries: value.schedules,
                schema: value.schema,
                max: value.max ?? 0
            });
    });
}

/**
 * @param {[number, string, string]} value
 */
function onValidate(value) {
    withSchedules((elem) => {
        const [id, key, message] = value;
        const elems = /** @type {NodeListOf<HTMLInputElement>} */
            (elem.querySelectorAll(`input[name=${key}]`));

        if (id < elems.length) {
            reportValidityForInputOrSelect(elems[id], message);
        }
    });
}

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "schConfig": (_, value) => {
            onConfig(value);
        },
        "schValidate": (_, value) => {
            onValidate(value);
        },
        "schTypes": (_, value) => {
            const tuples =
                /** @type {import('./settings/enumerable.mjs').EnumerableTuple[]} */(value);
            addEnumerables("schType",
                tuples.map((x) => [x[0], capitalize(x[1])]));
        },
    };
}

export function init() {
    withSchedules((elem) => {
        variableListeners(listeners());
        groupSettingsOnAddElem(elem, () => {
            scheduleAdd(elem);
        });
    });
}
