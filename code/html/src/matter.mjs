import { sendAction } from './connection.mjs';
import { styleInject } from './core.mjs';
import { qrDraw } from './qrcode.mjs';
import { variableListeners } from './settings.mjs';

// <link> stylesheets are not stripped per-module by the build, keep the rules here
const STYLE = [
    ".matter-qr { display: block; image-rendering: pixelated; border: 1px solid #eee; }",
    ".matter-manual { font-family: 'Courier New', monospace; font-size: 150%; letter-spacing: 1px; }",
    ".matter-payload { font-family: 'Courier New', monospace; word-break: break-all; }",
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
    });

    element(".button-matter-start")
        ?.addEventListener("click", (event) => {
            event.preventDefault();
            sendAction("matterStart");
        });

    element(".button-matter-window")
        ?.addEventListener("click", (event) => {
            event.preventDefault();
            sendAction("matterWindow");
        });

    updateStarted(false);
}
