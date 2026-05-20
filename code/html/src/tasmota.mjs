import { send, sendAction } from './connection.mjs';
import { notifyError } from './notify.mjs';

/**
 * Tasmota GPIO function codes → ESPurna mapping
 * Source: https://tasmota.github.io/docs/Components/
 * Only the most common function codes are listed here.
 * @type {Map<number, {type: string, index: number}>}
 */
const TASMOTA_GPIO_MAP = buildGpioMap();

function buildGpioMap() {
    /** @type {Map<number, {type: string, index: number, inverted: boolean}>} */
    const map = new Map();

    // -------------------------------------------------------------
    // Relays (up to 32)
    // -------------------------------------------------------------
    // Standard Relays: 224 to 255 (Relay1 to Relay32)
    // Inverted Relays: 256 to 287 (Relay1i to Relay32i)
    for (let i = 0; i < 32; i++) {
        map.set(224 + i, { type: 'relay', index: i, inverted: false });
        map.set(256 + i, { type: 'relay', index: i, inverted: true });
    }

    // Support custom templates where Relay1 uses active-low/inverted code 223
    map.set(223, { type: 'relay', index: 0, inverted: true });

    // -------------------------------------------------------------
    // Buttons (up to 32)
    // -------------------------------------------------------------
    // Standard Buttons (active low, pull-up): 32 to 63 (Button1 to Button32)
    // Buttons no pull-up (active low): 64 to 95 (Button_n1 to Button_n32)
    // Buttons inverted (active high, pull-up): 96 to 127 (Button_i1 to Button_i32)
    // Buttons inverted no pull-up (active high): 128 to 159 (Button_in1 to Button_in32)
    for (let i = 0; i < 32; i++) {
        map.set(32 + i, { type: 'button', index: i, inverted: false });
        map.set(64 + i, { type: 'button', index: i, inverted: false });
        map.set(96 + i, { type: 'button', index: i, inverted: true });
        map.set(128 + i, { type: 'button', index: i, inverted: true });
    }

    // -------------------------------------------------------------
    // Switches (up to 32)
    // -------------------------------------------------------------
    // Switches are also mapped to ESPurna buttons for input actions.
    // Standard Switches (pull-up): 160 to 191 (Switch1 to Switch32)
    // Switches no pull-up: 192 to 223 (Switch_n1 to Switch_n32)
    // Note: 223 is overridden above for Relay1i to support user's custom template.
    for (let i = 0; i < 32; i++) {
        const switchCode = 160 + i;
        const switchNoPullUpCode = 192 + i;
        
        if (!map.has(switchCode)) {
            map.set(switchCode, { type: 'button', index: i, inverted: false });
        }
        if (!map.has(switchNoPullUpCode)) {
            map.set(switchNoPullUpCode, { type: 'button', index: i, inverted: false });
        }
    }

    // -------------------------------------------------------------
    // LEDs (up to 32)
    // -------------------------------------------------------------
    // Standard LEDs: 288 to 319 (Led1 to Led32)
    // Inverted LEDs: 320 to 351 (Led1i to Led32i)
    for (let i = 0; i < 32; i++) {
        map.set(288 + i, { type: 'led', index: i, inverted: false });
        map.set(320 + i, { type: 'led', index: i, inverted: true });
    }

    // -------------------------------------------------------------
    // PWM Channels (up to 32)
    // -------------------------------------------------------------
    // Often used as LEDs or lighting outputs, can be mapped as LEDs.
    // Standard PWM: 416 to 447 (PWM1 to PWM32)
    // Inverted PWM: 448 to 479 (PWM_i1 to PWM_i32)
    for (let i = 0; i < 32; i++) {
        map.set(416 + i, { type: 'led', index: i, inverted: false });
        map.set(448 + i, { type: 'led', index: i, inverted: true });
    }

    // -------------------------------------------------------------
    // LedLink / Status LEDs (standard status/Wi-Fi connection LED)
    // -------------------------------------------------------------
    // LedLink = 544
    map.set(544, { type: 'led', index: 0, inverted: false });
    // LedLinki = 576
    map.set(576, { type: 'led', index: 0, inverted: true });

    return map;
}

/**
 * @typedef {{
 *   NAME: string,
 *   GPIO: number[],
 *   FLAG?: number,
 *   BASE?: number
 * }} TasmotaTemplate
 */

/**
 * Parse raw Tasmota JSON template text.
 * @param {string} text
 * @returns {TasmotaTemplate}
 */
function parseTasmotaTemplate(text) {
    const obj = JSON.parse(text);
    if (!Array.isArray(obj.GPIO)) {
        throw new Error('Invalid Tasmota template: missing GPIO array');
    }
    return obj;
}

/**
 * Convert Tasmota template to ESPurna settings key-value pairs.
 * @param {TasmotaTemplate} tmpl
 * @returns {{[key: string]: string}}
 */
function tasmotaToEspurna(tmpl) {
    /** @type {{[key: string]: string}} */
    const settings = {};

    if (tmpl.NAME) {
        settings['desc'] = tmpl.NAME;
    }

    // Counters per type to track assigned indices
    const counters = { relay: 0, button: 0, led: 0 };

    /**
     * Pin index (gpio number) → assigned espurna index for each type.
     * @type {Map<string, number>}
     */
    const assigned = new Map();

    tmpl.GPIO.forEach((code, gpioPin) => {
        if (code === 0) return;

        const info = TASMOTA_GPIO_MAP.get(code);
        if (!info) return; // unknown/unsupported function

        const { type, inverted } = info;
        const espurnaIndex = counters[type]++;
        const key = `${type}:${gpioPin}`;
        assigned.set(key, espurnaIndex);

        switch (type) {
            case 'relay':
                settings[`relayGpio${espurnaIndex}`] = gpioPin.toString();
                settings[`relayType${espurnaIndex}`] = inverted ? 'inverse' : 'normal';
                settings[`relayProv${espurnaIndex}`] = 'gpio';
                break;

            case 'button':
                settings[`btnGpio${espurnaIndex}`] = gpioPin.toString();
                settings[`btnConfig${espurnaIndex}`] = inverted
                    ? '2'  // BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
                    : '6'; // BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
                break;

            case 'led':
                settings[`ledGpio${espurnaIndex}`] = gpioPin.toString();
                settings[`ledInverse${espurnaIndex}`] = inverted ? '1' : '0';
                break;
        }
    });

    return settings;
}

/**
 * @param {string} json
 * @returns {string} Human readable summary of what was imported
 */
function importSummary(json) {
    try {
        const tmpl = parseTasmotaTemplate(json);
        const settings = tasmotaToEspurna(tmpl);

        const relays = Object.keys(settings).filter(k => k.startsWith('relayGpio')).length;
        const buttons = Object.keys(settings).filter(k => k.startsWith('btnGpio')).length;
        const leds = Object.keys(settings).filter(k => k.startsWith('ledGpio')).length;

        return `"${tmpl.NAME}": ${relays} relay(s), ${buttons} button(s), ${leds} LED(s)`;
    } catch {
        return '';
    }
}

/**
 * Import Tasmota template and send settings to the device.
 * @param {string} json
 */
function doImport(json) {
    const tmpl = parseTasmotaTemplate(json);
    const settings = tasmotaToEspurna(tmpl);

    if (Object.keys(settings).length === 0) {
        window.alert('No supported GPIO functions found in this Tasmota template.');
        return;
    }

    const summary = importSummary(json);
    const msg = `Import Tasmota template?\n\n${summary}\n\nNote: existing relay/button/LED settings will be overwritten.`;
    if (!window.confirm(msg)) return;

    send(JSON.stringify({
        settings: {
            set: settings,
            del: []
        }
    }));

    setTimeout(() => {
        if (window.confirm(`Tasmota settings have been sent to the device!\n\nReboot now to apply?`)) {
            sendAction('reboot');
        }
    }, 500);
}

export function init() {
    const textarea = /** @type {HTMLTextAreaElement | null} */
        (document.getElementById('tasmota-template'));
    const button = document.getElementById('button-tasmota-import');
    const preview = document.getElementById('tasmota-preview');

    if (!textarea || !button || !preview) return;

    textarea.addEventListener('input', () => {
        const val = textarea.value.trim();
        if (!val) {
            preview.textContent = '';
            return;
        }
        try {
            const summary = importSummary(val);
            preview.textContent = summary ? `✓ ${summary}` : '⚠ No recognized functions';
            preview.style.color = summary ? '#2a7' : '#c80';
        } catch {
            preview.textContent = '✗ Invalid JSON';
            preview.style.color = '#c22';
        }
    });

    button.addEventListener('click', () => {
        const val = textarea.value.trim();
        if (!val) {
            window.alert('Please paste a Tasmota template JSON first.');
            return;
        }
        try {
            doImport(val);
        } catch (e) {
            notifyError(/** @type {Error} */(e));
        }
    });
}
