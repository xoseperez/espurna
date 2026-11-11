import { sendAction } from './connection.mjs';
import { installMatterCode } from './fwinstall.mjs';
import { styleInject } from './core.mjs';
import { qrDraw } from './qrcode.mjs';
import { variableListeners } from './settings.mjs';

// <link> stylesheets are not stripped per-module by the build, keep the rules here
const STYLE = [
    ".matter-qr { display: block; image-rendering: pixelated; border: 1px solid #eee; }",
    ".matter-manual { font-family: 'Courier New', monospace; font-size: 150%; letter-spacing: 1px; }",
    ".matter-payload { font-family: 'Courier New', monospace; word-break: break-all; }",
    ".matter-controller { display: flex; align-items: center; flex-wrap: wrap; gap: 0.5em 1em; margin: 0.6em 0; }",
    ".matter-controller-node { color: #888; font-size: 85%; }",
    ".button-matter-remove { background: rgb(192, 0, 0); margin-left: auto; }",
];

/**
 * @param {string} selector
 * @returns {HTMLElement | null}
 */
function element(selector) {
    return document.querySelector(`#panel-matter ${selector}`);
}

/**
 * @param {string} payload
 */
function drawQr(payload) {
    const canvas = element(".matter-qr");
    if (!(canvas instanceof HTMLCanvasElement)) {
        return;
    }

    if (!payload) {
        canvas.width = 0;
        canvas.height = 0;
        return;
    }

    qrDraw(canvas, payload, 6);
}

// Vendor ids of the controllers (fabrics) people meet most; others show the number
const VENDORS = new Map([
    [0x132F, "Smart speaker"],
    [0x1349, "Apple Home"],
    [0x6006, "Google Home"],
    [0x1217, "Amazon Alexa"],
    [0x110A, "Samsung SmartThings"],
]);

/**
 * @param {number} vendor
 * @returns {string}
 */
export function vendorName(vendor) {
    const name = VENDORS.get(vendor);
    if (name) {
        return name;
    }

    // test vendor ids: python-matter-server, Home Assistant, chip-tool and other self-made controllers use them
    if ((vendor >= 0xFFF1) && (vendor <= 0xFFF4)) {
        return `Test vendor 0x${vendor.toString(16).toUpperCase()} (matter-server, Home Assistant, chip-tool...)`;
    }

    return `Vendor 0x${vendor.toString(16).toUpperCase().padStart(4, "0")}`;
}

/**
 * [[#, vendor id, "node id (hex)", "label"], ...]
 * @param {any} list
 */
function updateControllers(list) {
    const container = element(".matter-controllers");
    if (!container) {
        return;
    }

    container.replaceChildren();

    const entries = Array.isArray(list) ? list : [];
    if (!entries.length) {
        const empty = document.createElement("div");
        empty.classList.add("matter-controller");
        empty.textContent = "None: the device is not added anywhere yet.";
        container.append(empty);
        return;
    }

    for (const [index, vendor, node, label] of entries) {
        const name = vendorName(Number(vendor));

        const row = document.createElement("div");
        row.classList.add("matter-controller");

        const title = document.createElement("span");
        title.textContent = `${name}${label ? ` "${label}"` : ""}`;

        const details = document.createElement("span");
        details.classList.add("matter-controller-node");
        const node_id = String(node).replace(/^0+(?=.)/, "");
        details.textContent = `#${index}, device node 0x${node_id}`;

        const button = document.createElement("button");
        button.type = "button";
        button.classList.add("pure-button", "button-matter-remove");
        button.textContent = "Remove";
        button.addEventListener("click", (event) => {
            event.preventDefault();
            if (!window.confirm(`Remove ${name} from this device?\n\nIt stops controlling the device. To add the device to it again, open the pairing window below and add it in the app.`)) {
                return;
            }
            button.disabled = true;
            sendAction("matterRemove", {index: Number(index)});
        });

        row.append(title, details, button);
        container.append(row);
    }
}

/**
 * @param {number} seconds
 * @returns {string}
 */
export function windowText(seconds) {
    if (seconds <= 0) {
        return "Open pairing window";
    }

    const minutes = Math.floor(seconds / 60);
    const rest = String(seconds % 60).padStart(2, "0");
    return `Pairing window open, ${minutes}:${rest} left`;
}

/** @type {{left: number, timer: ReturnType<typeof setInterval> | null}} */
const WINDOW = {
    left: 0,
    timer: null,
};

function showWindow() {
    const button = element(".button-matter-window");
    if (!(button instanceof HTMLButtonElement)) {
        return;
    }

    button.disabled = WINDOW.left > 0;
    button.textContent = windowText(WINDOW.left);
}

/**
 * seconds left of the open pairing window (0: closed), counted down here between the updates of the device
 * @param {number} seconds
 */
function updateWindow(seconds) {
    WINDOW.left = Math.max(0, Math.floor(seconds));
    if (WINDOW.timer !== null) {
        clearInterval(WINDOW.timer);
        WINDOW.timer = null;
    }

    if (WINDOW.left > 0) {
        WINDOW.timer = setInterval(() => {
            WINDOW.left = Math.max(0, WINDOW.left - 1);
            if (!WINDOW.left && (WINDOW.timer !== null)) {
                clearInterval(WINDOW.timer);
                WINDOW.timer = null;
            }
            showWindow();
        }, 1000);
    }

    showWindow();
}

/**
 * @param {boolean} started
 */
function updateStarted(started) {
    const button = element(".button-matter-start");
    if (button instanceof HTMLButtonElement) {
        button.disabled = started;
    }

    const pairing = element(".matter-commissioning");
    if (pairing) {
        pairing.style.display = started ? "" : "none";
    }
}

export function init() {
    styleInject(STYLE);

    variableListeners({
        matterQr: (_, value) => {
            drawQr(String(value ?? ""));
        },
        matterStarted: (_, value) => {
            updateStarted(value === true || value === "yes");
        },
        matterControllers: (_, value) => {
            updateControllers(value);
        },
        matterWindow: (_, value) => {
            updateWindow(Number(value) || 0);
        },
        // Beken, Tuya layout: empty when the Matter code is in place, otherwise what is wrong with it
        matterCode: (_, value) => {
            const problem = String(value ?? "");
            for (const row of document.querySelectorAll("#panel-matter .matter-code")) {
                /** @type {HTMLElement} */(row).style.display = problem ? "" : "none";
            }
            const text = element(".matter-code-state");
            if (text) {
                text.textContent = problem ? `${problem}, Matter cannot start` : "";
            }
        },
    });

    element(".button-matter-code")
        ?.addEventListener("click", (event) => {
            event.preventDefault();
            installMatterCode();
        });

    element(".button-matter-start")
        ?.addEventListener("click", (event) => {
            event.preventDefault();
            sendAction("matterStart");
        });

    element(".button-matter-window")
        ?.addEventListener("click", (event) => {
            event.preventDefault();
            // grey at once, the countdown comes with the open event of the device
            if (event.currentTarget instanceof HTMLButtonElement) {
                event.currentTarget.disabled = true;
            }
            sendAction("matterWindow");
        });

    updateStarted(false);
}
