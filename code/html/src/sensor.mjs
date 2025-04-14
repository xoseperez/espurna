import { sendAction } from './connection.mjs';

import {
    showPanelByName,
} from './core.mjs';

import {
    addEnumerables,
    initElementOptions,
    listenEnumerableName,
    listenEnumerableTarget,
    prepareEnumerableTarget,
    setOriginalFromValue,
    setSelectValue,
    variableListeners,
} from './settings.mjs';

import {
    resetGroupElement,
    setChangedElement,
    setIgnoredElement,
} from './settings/utils.mjs';

import {
    fromSchema,
    NumberInput,
} from './template.mjs';

import {
    loadTemplate,
    mergeTemplate,
} from './settings/template.mjs';

/**
 * @typedef Magnitude
 * @property {string} description
 * @property {number} index_global
 * @property {string} name
 * @property {number} type
 * @property {number} units
*/

/** @typedef {[number, string]} SupportedUnits */

const Magnitudes = {
    /** @type {Map<number, Magnitude>} */
    properties: new Map(),

    /** @type {Map<number, string>} */
    errors: new Map(),

    /** @type {Map<number, string>} */
    types: new Map(),

    /** @type {Map<number, string>} */
    units: new Map(),

    /** @type {Map<number, number[]>} */
    supportedUnits: new Map(),

    /** @type {Map<number, string>} */
    typePrefix: new Map(),

    /** @type {Map<string, number>} */
    prefixType: new Map(),

    /** @type {boolean} */
    pending: false,
};

/**
 * @param {number} type
 * @param {number} index
 * @param {string} name
 */
function magnitudeTypedKey(type, index, name) {
    const prefix = Magnitudes.typePrefix.get(type);
    return `${prefix}${name}${index}`;
}

/**
 * @param {number} type
 */
function magnitudeTypeName(type) {
    return Magnitudes.types.get(type)
        ?? type.toString();
}

/**
 * @param {number} type
 * @param {number} index
 */
function magnitudeName(type, index) {
    return `${magnitudeTypeName(type)} #${index}`;
}

/**
 * @param {number} type
 * @param {number} index
 * @returns {NodeListOf<HTMLInputElement>}
 */
function magnitudeValueSelector(type, index) {
    return document.querySelectorAll(`input[name='magnitude:${magnitudeTypedKey(type, index, "")}`)
}

/**
 * @param {HTMLElement} elem
 * @param {number} id
 */
function listenEnumerableMagnitudeDescription(elem, id) {
    prepareEnumerableTarget(elem, id, "magnitude");
    listenEnumerableName(elem, "magnitude",
        (elem) => {
            elem.textContent =
                Magnitudes.properties.get(id)?.description ?? "";
        });
}

/**
 * @param {string} prefix
 * @param {any[][]} values
 * @param {string[]} schema
 */
function initModuleMagnitudes(prefix, values, schema) {
    const container = document.getElementById(`${prefix}-magnitudes`);
    if (!container || container.childElementCount > 0) {
        return;
    }

    const template = new NumberInput();

    values.forEach((value, id) => {
        const magnitude = fromSchema(value, schema);

        mergeTemplate(container, template.with(
            (label, input, span) => {
                listenEnumerableTarget(label, id, "magnitude");

                input.min = "0";
                input.name = `${prefix}Magnitude`;
                input.required = true;
                input.value = /** @type {!number} */
                    (magnitude.index_module).toString();
                setOriginalFromValue(input);

                listenEnumerableMagnitudeDescription(span, id);
            }));
    });
}

// Poll remote for magnitudes initial setup
function pendingMagnitudes() {
    if (Magnitudes.pending) {
        sendAction("magnitudes-pending");
        window.setTimeout(pendingMagnitudes, 5000);
    }
}

function waitPendingMagnitudes() {
    Magnitudes.pending = true;
    pendingMagnitudes();
}

/**
 * @param {any} types
 * @param {any} errors
 * @param {any} units
 */
function initMagnitudes(types, errors, units) {
    /** @type {[number, string, string][]} */
    (types.values).forEach((value) => {
        const info = fromSchema(value, types.schema);

        const type = /** @type {number} */(info.type);
        Magnitudes.types.set(type,
            /** @type {string} */(info.name));

        const prefix = /** @type {string} */(info.prefix);
        Magnitudes.typePrefix.set(type, prefix);
        Magnitudes.prefixType.set(prefix, type);

        const supported_units = /** @type {number[]} */(info.units);
        Magnitudes.supportedUnits.set(type, supported_units);
    });

    /** @type {[number, string][]} */
    (errors.values).forEach((value) => {
        const error = fromSchema(value, errors.schema);
        Magnitudes.errors.set(
            /** @type {number} */(error.type),
            /** @type {string} */(error.name));
    });

    /** @type {SupportedUnits[]} */
    (units.values).forEach((value) => {
        const unit = fromSchema(value, units.schema);
        Magnitudes.units.set(
            /** @type {number} */(unit.type),
            /** @type {string} */(unit.name));
    });
}

/**
 * @typedef {function(number, Magnitude): void} MagnitudeCallback
 * @param {any[]} values
 * @param {string[]} schema
 * @param {MagnitudeCallback[]} callbacks
 */
function initMagnitudesList(values, schema, callbacks) {
    /** @import { EnumerableNames } from './settings.mjs' */

    /** @type {EnumerableNames} */
    const names = {};

    values.forEach((value, id) => {
        const magnitude = fromSchema(value, schema);

        const description = /** @type {string} */
            (magnitude.description);
        const index_global = /** @type {number} */
            (magnitude.index_global);
        const type = /** @type {number} */
            (magnitude.type);
        const units = /** @type {number} */
            (magnitude.units);

        const name = magnitudeName(type, index_global);

        /** @type {Magnitude} */
        const result = {
            description,
            index_global,
            name,
            type,
            units,
        };

        names[id.toString()] = name;

        Magnitudes.properties.set(id, result);
        callbacks.forEach((callback) => {
            callback(id, result);
        });
    });

    addEnumerables("magnitude", names);
    Magnitudes.pending = false;
}

/**
 * @param {number} _id
 * @param {Magnitude} magnitude
 */
function createMagnitudeInfo(_id, magnitude) {
    const container = document.getElementById("magnitudes");
    if (!container) {
        return;
    }

    const line = loadTemplate("magnitude-status");

    const label = /** @type {!HTMLLabelElement} */
        (line.querySelector("label"));
    label.textContent = magnitude.name;

    const input = /** @type {!HTMLInputElement} */
        (line.querySelector("input[name='magnitude:']"));
    input.name +=
        magnitudeTypedKey(magnitude.type, magnitude.index_global, "");

    const info = /** @type {!HTMLSpanElement} */
        (line.querySelector(".magnitude-info"));
    info.style.display = "none";

    const description = /** @type {!HTMLSpanElement} */
        (line.querySelector(".magnitude-description"));
    description.textContent = magnitude.description;

    mergeTemplate(container, line);
}

/**
 * @param {number} _id
 * @param {Magnitude} magnitude
 */
function createMagnitudeUnitSelector(_id, magnitude) {
    const container = document.getElementById("magnitude-units");
    if (!container) {
        return;
    }

    const line = loadTemplate("magnitude-units");

    const label = /** @type {!HTMLLabelElement} */
        (line.querySelector("label"));
    label.textContent =
        magnitudeName(magnitude.type, magnitude.index_global);

    const select = /** @type {!HTMLSelectElement} */
        (line.querySelector("select"));
    select.setAttribute("name",
        magnitudeTypedKey(magnitude.type, magnitude.index_global, "Units"));

    const options = (Magnitudes.supportedUnits.get(magnitude.type) ?? [])
        .map((type) => ({
            'value': type.toString(),
            'text': Magnitudes.units.get(type) ?? type.toString()
        }));

    initElementOptions(select, options);
    setSelectValue(select, magnitude.units);
    setOriginalFromValue(select);

    container?.parentElement?.classList?.remove("maybe-hidden");
    mergeTemplate(container, line);
}

/**
 * @typedef SettingInfo
 * @property {number} id
 * @property {number} index_global
 * @property {string} key
 * @property {string} name
 * @property {string} prefix
 * @property {string} description
 */

/**
 * @param {number} id
 * @param {string} suffix
 * @returns {SettingInfo | null}
 */
function magnitudeSettingInfo(id, suffix) {
    const props = Magnitudes.properties.get(id);
    if (!props) {
        return null;
    }

    const prefix = Magnitudes.typePrefix.get(props.type);
    if (!prefix) {
        return null;
    }

    return {
        id: id,
        index_global: props.index_global,
        key: `${prefix}${suffix}${props.index_global}`,
        name: props.name,
        prefix,
        description: props.description,
    };
}

/**
 * @param {number} id
 * @returns {SettingInfo | null}
 */
function emonRatioInfo(id) {
    return magnitudeSettingInfo(id, "Ratio");
}

/** @typedef {{required?: boolean, min?: string, max?: string}} MagnitudeNumberOptions */

/**
 * @param {string} containerId
 * @param {number} id
 * @param {string} keySuffix
 * @param {number} value
 * @param {MagnitudeNumberOptions} options
 */
function initMagnitudeNumberSetting(containerId, id, keySuffix, value, {required = true, min = "", max = ""} = {}) {
    const container = document.getElementById(containerId);
    if (!container) {
        return;
    }

    const info = magnitudeSettingInfo(id, keySuffix);
    if (!info) {
        return;
    }

    container?.parentElement?.classList?.remove("maybe-hidden");

    const template = new NumberInput();
    mergeTemplate(container, template.with(
        (label, input, span) => {
            label.textContent = info.name;
            label.htmlFor = info.key;

            input.id = info.key;
            input.name = info.key;
            input.required = required;
            input.min = min;
            input.max = max;
            input.value = value.toString();

            resetGroupElement(input);
            setOriginalFromValue(input);
            listenEnumerableMagnitudeDescription(span, id);
        }));
}

/**
 * @param {number} id
 */
function initMagnitudesExpected(id) {
    const container = document.getElementById("emon-expected");
    if (!container) {
        return;
    }

    const info = emonRatioInfo(id);
    if (!info) {
        return;
    }

    const template = loadTemplate("emon-expected");

    const [magnitude, result, expected] =
        template.querySelectorAll("input");

    /** @type {!HTMLLabelElement} */
        (magnitude.previousElementSibling).textContent = info.name;
    /** @type {!HTMLSpanElement} */
        (magnitude.nextElementSibling).textContent = info.description;
    magnitude.name +=
        `${info.prefix}${info.index_global}`;

    result.name += info.key;
    result.id = result.name;

    expected.name += info.key;
    expected.id = expected.name;
    expected.dataset["id"] = info.id.toString();
    setIgnoredElement(expected);

    const [message] = /** @type {NodeListOf<HTMLSpanElement>} */
        (template.querySelectorAll(`span.emon-expected-${info.prefix}`));
    if (message) {
        message.classList.replace("hidden", "visible");
    }

    mergeTemplate(container, template);
}

function emonCalculateRatios() {
    const expected = document.getElementById("emon-expected")
        ?.querySelectorAll("input.emon-expected-input");
    if (!expected) {
        return;
    }

    /** @type {NodeListOf<HTMLInputElement>} */
    (expected).forEach((input) => {
        if (!input.value || !input.dataset["id"]) {
            return;
        }

        sendAction("emon-expected", {
            id: parseInt(input.dataset["id"], 10),
            expected: parseFloat(input.value) });
    });
}

function emonApplyRatios() {
    const results = document.getElementById("emon-expected")
        ?.querySelectorAll("input.emon-expected-result");

    /** @type {NodeListOf<HTMLInputElement>} */
    (results).forEach((result) => {
        if (!result.value) {
            return;
        }

        let next = result.name
            .replace("result:", "");

        const ratio = document.getElementById(next);
        if (!(ratio instanceof HTMLInputElement)) {
            return;
        }

        ratio.value = result.value;
        setChangedElement(ratio);

        result.value = "";

        next = result.name
            .replace("result:", "expected:");
        const expected = document.getElementById(next);
        if (!(expected instanceof HTMLInputElement)) {
            return;
        }

        expected.value = "";
    });

    showPanelByName("sns");
}

/**
 * @param {any[]} values
 * @param {string[]} schema
 */
function initMagnitudesSettings(values, schema) {
    values.forEach((value, id) => {
        const settings = fromSchema(value, schema);

        if (typeof settings.Ratio === "number") {
            initMagnitudeNumberSetting(
                "emon-ratios", id,
                "Ratio", settings.Ratio);
            initMagnitudesExpected(id);
        }

        if (typeof settings.Correction === "number") {
            initMagnitudeNumberSetting(
                "magnitude-corrections", id,
                "Correction", settings.Correction);
        }

        if (typeof settings.MinDelta === "number") {
            initMagnitudeNumberSetting(
                "magnitude-min-deltas", id,
                "MinDelta", settings.MinDelta, {min: "0"});
        }

        if (typeof settings.MaxDelta === "number") {
            initMagnitudeNumberSetting(
                "magnitude-max-deltas", id,
                "MaxDelta", settings.MaxDelta, {min: "0"});
        }

        for (let type of ["Min", "Max", "Zero"]) {
            const key = `${type}Threshold`;
            const threshold =
                (typeof settings[key] === "number")
                    ? settings[key] :
                (typeof settings[key] === "string")
                    ? NaN : null;

            if (typeof threshold === "number") {
                initMagnitudeNumberSetting(
                    `magnitude-${type.toLowerCase()}-thresholds`, id,
                    key, threshold, {
                        required: false,
                        min: (type === "Zero") ? "0" : undefined
                    });
            }
        }
    });
}

/**
 * @param {any[]} values
 * @param {string[]} schema
 */
function updateMagnitudes(values, schema) {
    values.forEach((value, id) => {
        const props = Magnitudes.properties.get(id);
        if (!props) {
            return;
        }

        const magnitude = fromSchema(value, schema);
        if (typeof magnitude.units === "number") {
            props.units = magnitude.units;
        }

        let inputValue = "";
        if (typeof magnitude.error === "number" && 0 !== magnitude.error) {
            inputValue =
                Magnitudes.errors.get(magnitude.error) ?? "Unknown error";
        } else if (typeof magnitude.value === "string") {
            const units = Magnitudes.units.get(
                /** @type {number} */(magnitude.units)) ?? "";
            inputValue = `${magnitude.value}${units}`;
        } else {
            inputValue = magnitude.value?.toString() ?? "?";
        }

        magnitudeValueSelector(props.type, props.index_global)
            .forEach((input) => { input.value = inputValue; });
    });
}

/**
 * @param {HTMLInputElement} input
 * @param {string} saved
 */
function updateEnergyInput(input, saved) {
    const info = input.nextElementSibling;
    if (info instanceof HTMLElement && info.matches("span.magnitude-info")) {
        info.style.display = "inherit";
        info.textContent = saved;
    }
}

/**
 * @param {any[]} values
 * @param {string[]} schema
 */
function updateEnergy(values, schema) {
    values.forEach((value) => {
        const energy = fromSchema(value, schema);

        const props = Magnitudes.properties.get(
            /** @type {!number} */(energy.id));
        if (!props) {
            return;
        }

        const saved = /** @type {!string} */(energy.saved);
        magnitudeValueSelector(props.type, props.index_global)
            .forEach((input) => updateEnergyInput(input, saved));
    });
}

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "magnitudes-pending": () => {
            waitPendingMagnitudes();
        },
        "magnitudes-init": (_, value) => {
            initMagnitudes(
                value.types, value.errors, value.units);
        },
        "magnitudes-module": (_, value) => {
            initModuleMagnitudes(
                value.prefix, value.values, value.schema);
        },
        "magnitudes-list": (_, value) => {
            initMagnitudesList(value.values, value.schema, [
                createMagnitudeUnitSelector, createMagnitudeInfo]);
        },
        "magnitudes-settings": (_, value) => {
            initMagnitudesSettings(value.values, value.schema);
        },
        "magnitudes": (_, value) => {
            updateMagnitudes(value.values, value.schema);
        },
        "energy": (_, value) => {
            updateEnergy(value.values, value.schema);
        },
    };
}

export function init() {
    variableListeners(listeners());

    document.querySelector(".button-emon-expected-calculate")
        ?.addEventListener("click", emonCalculateRatios);
    document.querySelector(".button-emon-expected-apply")
        ?.addEventListener("click", emonApplyRatios);
}
