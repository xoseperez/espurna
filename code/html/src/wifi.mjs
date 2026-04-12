import { groupSettingsOnAddElem } from './settings/group.mjs';
import { variableListeners } from './settings.mjs';

import { sendAction } from './connection.mjs';
import { lastMoreElem } from './core.mjs';
import { addFromTemplate, addOriginalsFromTemplate } from './template.mjs';

/** @param {function(HTMLElement): void} callback */
function withNetworks(callback) {
    callback(/** @type {!HTMLElement} */
        (document.getElementById("networks")));
}

const TEMPLATE_NAME = "network-config";

/**
 * @param {any} value
 */
function onConfig(value) {
    withNetworks((elem) => {
        addOriginalsFromTemplate(
            elem, TEMPLATE_NAME,
            {
                entries: value.networks,
                schema: value.schema,
                max: value.max ?? 0
            });
    });
}

/**
 * @param {HTMLElement} elem
 */
function networkAdd(elem) {
    addFromTemplate(elem, TEMPLATE_NAME, {});
    lastMoreElem(elem);
}

/** @param {function(HTMLTableElement): void} callback */
function withScanTable(callback) {
    callback(/** @type {!HTMLTableElement} */
        (document.getElementById("scanResult")));
}

/** @param {function(HTMLButtonElement): void} callback */
function withScanButton(callback) {
    /** @type {NodeListOf<HTMLButtonElement>} */
    (document.querySelectorAll("button.button-wifi-scan"))
        .forEach(callback);
}

/** @param {boolean} value */
function scanButtonDisabled(value) {
    withScanButton((button) => {
        button.disabled = value;
    });
}

/** @param {boolean} value */
function loadingDisplay(value) {
    const loading = (/** @type {!HTMLDivElement} */
        (document.querySelector("div.scan.loading")));
    loading.style.display = value ? "table" : "none";
}

/** @param {string[]} values */
function scanResult(values) {
    withScanTable((table) => {
        scanButtonDisabled(false);
        loadingDisplay(false);
        table.style.display = "table";

        const [body] = table.tBodies;
        const row = body.insertRow();
        for (let value of values) {
            const cell = row.insertCell();
            cell.appendChild(document.createTextNode(value));
        }

        const footer = table.nextElementSibling;
        if (footer instanceof HTMLSpanElement) {
            footer.textContent = (new Date()).toLocaleString();
        }
    });
}

/** @param {Event} event */
function scanStart(event) {
    event.preventDefault();

    withScanTable((table) => {
        const [body] = table.tBodies;
        while (body.rows.length) {
            body.deleteRow(0);
        }

        loadingDisplay(true);
        scanButtonDisabled(true);

        sendAction("scan");
    });
}

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "wifiConfig": (_, value) => {
            onConfig(value);
        },
        "scanResult": (_, value) => {
            scanResult(value);
        },
    };
}

export function init() {
    withNetworks((elem) => {
        variableListeners(listeners());
        // TODO: as event arg?
        groupSettingsOnAddElem(elem, () => {
            networkAdd(elem);
        });
        withScanButton((button) => {
            button.addEventListener("click", scanStart);
        });
    });
}
