import { sendAction } from './connection.mjs';
import { variableListeners } from './settings.mjs';
import { groupSettingsOnAddElem } from './settings/group.mjs';
import { addFromTemplate, addOriginalsFromTemplate } from './template.mjs';

/**
 * @typedef {Map<number, string>} FiltersMap
 */

/** @type {FiltersMap} */
const Filters = new Map();

/** @param {function(HTMLTableElement): void} callback */
function withTable(callback) {
    callback(/** @type {!HTMLTableElement} */
        (document.getElementById("rfm69-messages")));
}

/** @param {function(HTMLTableSectionElement): void} callback */
function withMessages(callback) {
    withTable((elem) => {
        callback(elem.tBodies[0]);
    });
}

/** @param {[number, number, number, string, string, number, number, number]} message */
function addMessage(message) {
    withTable((elem) => {
        const timestamp = (new Date())
            .toLocaleTimeString("en-US", {hour12: false});

        const row = elem.tBodies[0].insertRow();
        for (let value of [timestamp, ...message]) {
            const cell = row.insertCell();
            cell.appendChild(
                document.createTextNode(value.toString()));
            filterRow(Filters, row);
        }
    });
}

/** @param {function(HTMLElement): void} callback */
function withMapping(callback) {
    callback(/** @type {!HTMLElement} */
        (document.getElementById("rfm69-mapping")));
}

const TEMPLATE_NAME = "rfm69-node";

/** @param {HTMLElement} elem */
function addMappingNode(elem) {
    addFromTemplate(elem, TEMPLATE_NAME, {});
}

/** @param {any} value */
function onMapping(value) {
    withMapping((elem) => {
        addOriginalsFromTemplate(
            elem, TEMPLATE_NAME,
            {
                entries: value.mapping,
                schema: value.schema,
                max: value.max ?? 0
            });
    });
}

function clearCounters() {
    sendAction("rfm69Clear");
    return false;
}

function clearMessages() {
    withMessages((elem) => {
        while (elem.rows.length) {
            elem.deleteRow(0);
        }
    });

    return false;
}

/**
 * @param {FiltersMap} filters
 * @param {HTMLTableRowElement} row
 */
function filterRow(filters, row) {
    row.style.display = "table-row";
    for (const [cell, filter] of filters) {
        if (row.cells[cell].textContent !== filter) {
            row.style.display = "none";
        }
    }
}

/**
 * @param {FiltersMap} filters
 * @param {HTMLTableRowElement[]} rows
 */
function filterRows(filters, rows) {
    for (let row of rows) {
        filterRow(filters, row);
    }
}

/** @param {Event} event */
function filterEvent(event) {
    if (!(event.target instanceof HTMLTableCellElement)) {
        return;
    }

    if (!event.target.textContent) {
        return;
    }

    const index = event.target.cellIndex;
    if (event.target.classList.contains("filtered")) {
        Filters.delete(index);
    } else {
        Filters.set(index, event.target.textContent);
    }

    event.target.classList.toggle("filtered");
    withMessages((elem) => {
        filterRows(Filters, Array.from(elem.rows));
    });
}

function clearFilters() {
    withMessages((elem) => {
        for (let filtered of elem.querySelectorAll("filtered")) {
            filtered.classList.remove("filtered");
        }

        Filters.clear();
        filterRows(Filters, Array.from(elem.rows));
    });
}

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "rfm69": (_, value) => {
            if (value.message !== undefined) {
                addMessage(value.message);
            }

            if (value.mapping !== undefined) {
                onMapping(value);
            }
        },
    };
}

export function init() {
    variableListeners(listeners());

    document.querySelector(".button-clear-counts")
        ?.addEventListener("click", clearCounters);
    document.querySelector(".button-clear-messages")
        ?.addEventListener("click", clearMessages);

    document.querySelector(".button-clear-filters")
        ?.addEventListener("click", clearFilters);
    document.querySelector("#rfm69-messages tbody")
        ?.addEventListener("click", filterEvent);

    withMapping((elem) => {
        groupSettingsOnAddElem(elem, () => {
            addMappingNode(elem);
        });
    });
}
