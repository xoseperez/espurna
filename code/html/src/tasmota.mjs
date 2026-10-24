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
    /**
     * @type {Map<number, {
     *   type: string,
     *   index: number,
     *   inverted: boolean,
     *   activeHigh?: boolean,
     *   internalPull?: boolean,
     *   switch?: boolean
     * }>}
     *
     * For buttons we carry polarity (activeHigh) and internal-pull preference
     * as two independent flags — Tasmota encodes both, and conflating them
     * causes inverted buttons to register "release" as "press".
     */
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
    // 32–63   Button{N}    — active LOW,  internal pull-up
    // 64–95   Button{N}n   — active LOW,  no internal pull (external pull-up wired)
    // 96–127  Button{N}i   — active HIGH, internal pull-down
    // 128–159 Button{N}in  — active HIGH, no internal pull (external pull-down wired)
    for (let i = 0; i < 32; i++) {
        map.set(32 + i,  { type: 'button', index: i, inverted: false, activeHigh: false, internalPull: true  });
        map.set(64 + i,  { type: 'button', index: i, inverted: false, activeHigh: false, internalPull: false });
        map.set(96 + i,  { type: 'button', index: i, inverted: true,  activeHigh: true,  internalPull: true  });
        map.set(128 + i, { type: 'button', index: i, inverted: true,  activeHigh: true,  internalPull: false });
    }

    // -------------------------------------------------------------
    // Switches (up to 32)
    // -------------------------------------------------------------
    // Mapped to ESPurna buttons for input actions.
    // 160–191 Switch{N}   — internal pull-up,  active LOW
    // 192–223 Switch{N}n  — no internal pull, active LOW (external pull-up expected)
    // Note: 223 is overridden above for Relay1i to support user's custom template.
    for (let i = 0; i < 32; i++) {
        const switchCode = 160 + i;
        const switchNoPullUpCode = 192 + i;

        if (!map.has(switchCode)) {
            map.set(switchCode, { type: 'button', index: i, inverted: false, activeHigh: false, internalPull: true, switch: true });
        }
        if (!map.has(switchNoPullUpCode)) {
            map.set(switchNoPullUpCode, { type: 'button', index: i, inverted: false, activeHigh: false, internalPull: false, switch: true });
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
 * Tasmota templates cannot express AC mains (optocoupler) switch inputs, Tasmota itself
 * enables AC detection with `SwitchDebounce` ending with 9 (e.g. `SwitchDebounce 69`).
 * - `ac` marks inputs imported from Switch codes (160..223) as AC mains switches
 * - `acButtons` also marks inputs imported from Button codes (32..159)
 * - `freq` is the mains frequency, 50 or 60 Hz (global `btnAcFreq`)
 * @typedef {{ac: boolean, acButtons: boolean, freq: number}} ImportOptions
 */

/** @type {ImportOptions} */
const DEFAULT_OPTIONS = { ac: false, acButtons: false, freq: 50 };

/**
 * Detect AC switch mode from pasted Tasmota commands, e.g. `SwitchDebounce 69`
 * @param {string} text
 * @returns {boolean}
 */
export function acFromCommands(text) {
    const match = text.match(/SwitchDebounce\s+(\d+)/i);
    if (!match) {
        return false;
    }

    return (parseInt(match[1], 10) % 10) === 9;
}

/**
 * Convert Tasmota template to ESPurna settings key-value pairs.
 * @param {TasmotaTemplate} tmpl
 * @param {ImportOptions} options
 * @returns {{[key: string]: string}}
 */
export function tasmotaToEspurna(tmpl, options = DEFAULT_OPTIONS) {
    /** @type {{[key: string]: string}} */
    const settings = {};

    /** @type {{button: number, tasmota: number}[]} */
    const acInputs = [];

    /** @type {Map<number, number>} Tasmota RelayN number to the ESPurna relay index */
    const relays = new Map();

    if (tmpl.NAME) {
        settings['desc'] = tmpl.NAME;
    }

    // Counters per type to track assigned indices
    const counters = { relay: 0, button: 0, led: 0 };

    tmpl.GPIO.forEach((code, gpioPin) => {
        if (code === 0) return;

        const info = TASMOTA_GPIO_MAP.get(code);
        if (!info) return; // unknown/unsupported function

        const { type, inverted } = info;
        const espurnaIndex = counters[type]++;

        switch (type) {
            case 'relay':
                settings[`relayGpio${espurnaIndex}`] = gpioPin.toString();
                settings[`relayType${espurnaIndex}`] = inverted ? 'inverse' : 'normal';
                settings[`relayProv${espurnaIndex}`] = 'gpio';
                relays.set(info.index, espurnaIndex);
                break;

            case 'button': {
                settings[`btnGpio${espurnaIndex}`] = gpioPin.toString();
                settings[`btnMode${espurnaIndex}`] = 'pushbutton';
                // Polarity and internal-pull are independent in Tasmota.
                // `btnDefVal` = idle level: active-low → 'high' (idle high, pressed pulls low);
                //                          active-high → 'low'  (idle low,  pressed pulls high).
                // `btnPinMode`: internal resistor used by the MCU.
                const activeHigh = !!info.activeHigh;
                const internalPull = info.internalPull !== false;
                settings[`btnDefVal${espurnaIndex}`] = activeHigh ? 'low' : 'high';
                settings[`btnPinMode${espurnaIndex}`] =
                    !internalPull ? 'default'
                    : activeHigh  ? 'pull-down'
                                  : 'pull-up';

                // AC mains wall switch. Input sees mains-frequency pulses while ON,
                // which are counted within the debounce window (100ms).
                // Maintained switch, every change of position toggles the relay.
                if (options.ac && (info.switch || options.acButtons)) {
                    settings[`btnFreq${espurnaIndex}`] = '1';
                    settings[`btnMode${espurnaIndex}`] = 'switch';
                    settings[`btnDefVal${espurnaIndex}`] = 'high';
                    settings[`btnPress${espurnaIndex}`] = 'relay-toggle';
                    settings[`btnRlse${espurnaIndex}`] = 'relay-toggle';
                    settings[`btnDebDel${espurnaIndex}`] = '100';
                    acInputs.push({button: espurnaIndex, tasmota: info.index});
                }
                break;
            }

            case 'led':
                settings[`ledGpio${espurnaIndex}`] = gpioPin.toString();
                settings[`ledInv${espurnaIndex}`] = inverted ? '1' : '0';
                break;
        }
    });

    // Tasmota default is SwitchN / ButtonN controlling RelayN.
    // nb. ESPurna indices are assigned in GPIO order, not by the Tasmota number
    if (acInputs.length) {
        settings['btnAcFreq'] = (options.freq === 60) ? '60' : '50';
        for (const input of acInputs) {
            const relay = relays.get(input.tasmota);
            if (relay !== undefined) {
                settings[`btnRelay${input.button}`] = relay.toString();
            }
        }
    }

    return settings;
}

/**
 * @param {string} json
 * @param {ImportOptions} options
 * @returns {string} Human readable summary of what was imported
 */
export function importSummary(json, options = DEFAULT_OPTIONS) {
    try {
        const tmpl = parseTasmotaTemplate(json);
        const settings = tasmotaToEspurna(tmpl, options);

        const relays = Object.keys(settings).filter(k => k.startsWith('relayGpio')).length;
        const buttons = Object.keys(settings).filter(k => k.startsWith('btnGpio')).length;
        const leds = Object.keys(settings).filter(k => k.startsWith('ledGpio')).length;
        const ac = Object.keys(settings).filter(k => k.startsWith('btnFreq')).length;

        let out = `"${tmpl.NAME}": ${relays} relay(s), ${buttons} button(s), ${leds} LED(s)`;
        if (ac) {
            out += `, ${ac} of them AC mains switch(es) @ ${settings['btnAcFreq']} Hz`;
        }

        return out;
    } catch {
        return '';
    }
}

/**
 * Import Tasmota template and send settings to the device.
 * @param {string} json
 * @param {ImportOptions} options
 */
function doImport(json, options) {
    const tmpl = parseTasmotaTemplate(json);
    const settings = tasmotaToEspurna(tmpl, options);

    if (Object.keys(settings).length === 0) {
        window.alert('No supported GPIO functions found in this Tasmota template.');
        return;
    }

    const summary = importSummary(json, options);
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

    const ac = /** @type {HTMLInputElement | null} */
        (document.getElementById('tasmota-ac'));
    const acButtons = /** @type {HTMLInputElement | null} */
        (document.getElementById('tasmota-ac-buttons'));
    const acFreq = /** @type {HTMLSelectElement | null} */
        (document.getElementById('tasmota-ac-freq'));
    const commands = /** @type {HTMLInputElement | null} */
        (document.getElementById('tasmota-commands'));

    /** @returns {ImportOptions} */
    const options = () => ({
        ac: !!ac?.checked,
        acButtons: !!acButtons?.checked,
        freq: parseInt(acFreq?.value ?? '50', 10),
    });

    const updatePreview = () => {
        const val = textarea.value.trim();
        if (!val) {
            preview.textContent = '';
            return;
        }
        try {
            const summary = importSummary(val, options());
            preview.textContent = summary ? `✓ ${summary}` : '⚠ No recognized functions';
            preview.style.color = summary ? '#2a7' : '#c80';
        } catch {
            preview.textContent = '✗ Invalid JSON';
            preview.style.color = '#c22';
        }
    };

    textarea.addEventListener('input', updatePreview);
    for (const elem of [ac, acButtons, acFreq]) {
        elem?.addEventListener('change', updatePreview);
    }

    commands?.addEventListener('input', () => {
        if (ac && acFromCommands(commands.value)) {
            ac.checked = true;
        }
        updatePreview();
    });

    button.addEventListener('click', () => {
        const val = textarea.value.trim();
        if (!val) {
            window.alert('Please paste a Tasmota template JSON first.');
            return;
        }
        try {
            doImport(val, options());
        } catch (e) {
            notifyError(/** @type {Error} */(e));
        }
    });
}
