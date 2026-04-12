import { groupSettingsOnAddElem } from './settings/group.mjs';
import { variableListeners } from './settings.mjs';
import { addFromTemplate, addOriginalsFromTemplate } from './template.mjs';

/** @param {function(HTMLElement): void} callback */
function withRules(callback) {
    callback(/** @type {!HTMLElement} */
        (document.getElementById("rpn-rules")));
}

/**
 * @param {HTMLElement} elem
 * @param {string} rule
 */
function addRule(elem, rule = "") {
    addFromTemplate(elem, "rpn-rule", {rpnRule: rule});
}

/** @param {function(HTMLElement): void} callback */
function withTopics(callback) {
    callback(/** @type {!HTMLElement} */
        (document.getElementById("rpn-topics")));
}

const TEMPLATE_NAME = "rpn-topic";

/** @param {HTMLElement} elem */
function addTopic(elem) {
    addFromTemplate(elem, TEMPLATE_NAME, {});
}

/**
 * @param {HTMLElement} elem
 * @param {any} value
 */
function addOriginalTopic(elem, value) {
    addOriginalsFromTemplate(
        elem, TEMPLATE_NAME,
        {
            entries: value.topics,
            schema: value.schema,
            max: value.max ?? 0
        });
}

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "rpnRules": (_, value) => {
            withRules((elem) => {
                for (let rule of value) {
                    addRule(elem, rule);
                }
            });
        },
        "rpnTopics": (_, value) => {
            withTopics((elem) => {
                addOriginalTopic(elem, value);
            });
        },
    };
}

export function init() {
    variableListeners(listeners());
    withRules((elem) => {
        groupSettingsOnAddElem(elem, () => {
            addRule(elem);
        });
    });
    withTopics((elem) => {
        groupSettingsOnAddElem(elem, () => {
            addTopic(elem);
        });
    });
}
