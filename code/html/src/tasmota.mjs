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
    // Light channels (bulbs, LED strips), ESPurna dimmer channels in PWM order
    // Standard PWM: 416 to 447 (PWM1 to PWM32)
    // Inverted PWM: 448 to 479 (PWM_i1 to PWM_i32)
    for (let i = 0; i < 32; i++) {
        map.set(416 + i, { type: 'light', index: i, inverted: false });
        map.set(448 + i, { type: 'light', index: i, inverted: true });
    }

    // -------------------------------------------------------------
    // LedLink / Status LEDs (standard status/Wi-Fi connection LED)
    // -------------------------------------------------------------
    // LedLink = 544
    map.set(544, { type: 'led', index: 0, inverted: false, link: true });
    // LedLinki = 576
    map.set(576, { type: 'led', index: 0, inverted: true, link: true });

    return map;
}

// Template GPIO array position -> physical GPIO, ref. Tasmota include/tasmota_template.h
// - ESP8266, 13 (legacy 8-bit codes) or 14 (+ ADC0, not used here) entries, flash pins 6, 7, 8 and 11 are skipped
// - ESP32, 36 entries. Kept backwards compatible with the older 0-5 9-10 12-39 layout, flash pins are at the end
// Any other length (e.g. full 18 / 40 entry lists) is taken as position == GPIO
const ESP8266_TEMPLATE_PINS = [0, 1, 2, 3, 4, 5, 9, 10, 12, 13, 14, 15, 16];
const ESP32_TEMPLATE_PINS = [
    0, 1, 2, 3, 4, 5, 9, 10,
    12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27,
    6, 7, 8, 11,
    32, 33, 34, 35, 36, 37, 38, 39];

/**
 * @param {number} length of the template GPIO array
 * @returns {'esp8266' | 'esp32' | null} chip the template was made for, null when the layout is not known
 */
export function templateChip(length) {
    if ((length === 13) || (length === 14)) {
        return 'esp8266';
    }

    if (length === 36) {
        return 'esp32';
    }

    return null;
}

/**
 * @param {number} length
 * @param {number} position
 * @returns {number | null} physical GPIO, null for ADC0
 */
function templatePin(length, position) {
    switch (templateChip(length)) {
    case 'esp8266':
        return ESP8266_TEMPLATE_PINS[position] ?? null;
    case 'esp32':
        return ESP32_TEMPLATE_PINS[position] ?? null;
    }

    return position;
}

// Tasmota before 9.1 used 8-bit codes, still found in 13 entry templates.
// Indexed by the legacy code, ref. kGpioConvert in Tasmota include/tasmota_template_legacy.h
const LEGACY_GPIO_CODES = [
    0, 1184, 1216, 1248, 1312, 608, 640, 1376, 1056, 160, 161, 162, 163, 164, 165, 166, 167, 32, 33, 34, 35,
    224, 225, 226, 227, 228, 229, 230, 231, 256, 257, 258, 259, 260, 261, 262, 263, 416, 417, 418, 419, 420,
    352, 353, 354, 355, 448, 449, 450, 451, 452, 1088, 288, 289, 290, 291, 320, 321, 322, 323, 1408, 1440, 1472,
    1504, 1600, 1632, 768, 800, 992, 1696, 1760, 1792, 1824, 1856, 1888, 1920, 1952, 1984, 2016, 2048, 2080,
    2112, 192, 193, 194, 195, 196, 197, 198, 199, 64, 65, 66, 67, 384, 385, 386, 387, 1536, 1568, 2144, 1728,
    2176, 2208, 2240, 1120, 1152, 2272, 2304, 2336, 2368, 832, 864, 896, 928, 960, 2400, 2432, 2464, 2496, 2528,
    2560, 96, 97, 98, 99, 128, 129, 130, 131, 2592, 2624, 2656, 2688, 2720, 2752, 2784, 2816, 2848, 2880, 2912,
    2944, 2976, 3008, 3040, 3072, 3104, 3136, 3200, 3232, 3264, 3296, 3265, 3297, 3392, 3424, 3456, 544, 576,
    3168, 480, 512, 1024, 3488, 3520, 3552, 3584, 3616, 3648, 3680, 3712, 3744, 3776, 3808, 3809, 3810, 3904,
    3936, 3968, 4000, 4032, 4064, 4096, 4128, 4160, 4192, 4224, 4256, 4288, 4320, 4352, 4384, 1344, 1280, 4416,
    4448, 4480, 4512, 4544, 4576, 4608, 4640, 4672, 1664, 4928, 4960, 4992, 5056, 5088, 5120, 5152, 5184, 5216,
    5248, 5280, 5312, 5344
];

/**
 * @param {number} length of the template GPIO array
 * @param {number} code
 * @returns {number} current Tasmota GPIO function code
 */
function templateCode(length, code) {
    if (length === 13) {
        return LEGACY_GPIO_CODES[code] ?? 0;
    }

    return code;
}

/**
 * @typedef {{
 *   NAME: string,
 *   GPIO: number[],
 *   FLAG?: number,
 *   BASE?: number,
 *   CMND?: string
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
 * The template itself cannot express AC mains (optocoupler) wall switches. Tasmota enables them
 * with `SwitchDebounce` ending with 9, when that is in the template commands (`CMND`) Switch inputs
 * become AC mains switches here too. Otherwise they are marked afterwards on the RELAYS panel
 * @param {TasmotaTemplate} tmpl
 * @returns {{[key: string]: string}}
 */
export function tasmotaToEspurna(tmpl) {
    /** @type {{[key: string]: string}} */
    const settings = {};

    /** @type {Map<number, number>} Tasmota RelayN number to the ESPurna relay index */
    const relays = new Map();

    const ac = /SwitchDebounce\s+\d*9\b/i.test(tmpl.CMND ?? '');

    // shown as the Device on the status page, `desc` is left to the user
    if (tmpl.NAME) {
        settings['sysDevice'] = tmpl.NAME;
    }

    // Template positions to physical GPIOs, legacy codes to the current ones.
    // ESPurna indices are assigned in GPIO order
    const length = tmpl.GPIO.length;
    const pins = tmpl.GPIO
        .map((code, position) => ({
            gpio: templatePin(length, position),
            info: TASMOTA_GPIO_MAP.get(templateCode(length, code)),
        }))
        .filter((pin) => (pin.gpio !== null) && (pin.info !== undefined))
        .sort((lhs, rhs) => (lhs.gpio ?? 0) - (rhs.gpio ?? 0));

    for (const pin of pins) {
        const info = /** @type {NonNullable<typeof pin.info>} */(pin.info);
        if (info.type === 'relay') {
            relays.set(info.index, relays.size);
        }
    }

    // Counters per type to track assigned indices
    const counters = { relay: 0, button: 0, led: 0, light: 0 };

    /** @type {Map<number, number>} Tasmota LedN number to the ESPurna led index */
    const leds = new Map();
    let link = false;

    for (const pin of pins) {
        const info = /** @type {NonNullable<typeof pin.info>} */(pin.info);
        const { type, inverted } = info;
        const espurnaIndex = counters[type]++;
        const gpioPin = /** @type {number} */(pin.gpio);

        switch (type) {
            case 'relay':
                settings[`relayGpio${espurnaIndex}`] = gpioPin.toString();
                settings[`relayType${espurnaIndex}`] = inverted ? 'inverse' : 'normal';
                settings[`relayProv${espurnaIndex}`] = 'gpio';
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

                // Tasmota default is SwitchN / ButtonN controlling RelayN
                const relay = relays.get(info.index);
                if (relay !== undefined) {
                    settings[`btnRelay${espurnaIndex}`] = relay.toString();
                }

                // Wall switch, every change of position toggles the relay (Tasmota SwitchMode 0).
                // No click actions, holding the switch must not reset the device
                if (info.switch) {
                    settings[`btnMode${espurnaIndex}`] = 'switch';
                    settings[`btnPress${espurnaIndex}`] = 'relay-toggle';
                    settings[`btnRlse${espurnaIndex}`] = 'relay-toggle';
                    for (const key of ['btnClick', 'btnDclk', 'btnTclk', 'btnLclk', 'btnLLclk']) {
                        settings[`${key}${espurnaIndex}`] = 'none';
                    }

                    // mains frequency pulses while ON, counted within the debounce window
                    if (ac) {
                        settings[`btnFreq${espurnaIndex}`] = '1';
                        settings[`btnDefVal${espurnaIndex}`] = 'high';
                        settings[`btnPinMode${espurnaIndex}`] = 'default';
                        settings[`btnDebDel${espurnaIndex}`] = '100';
                    }
                }

                break;
            }

            case 'led':
                settings[`ledGpio${espurnaIndex}`] = gpioPin.toString();
                settings[`ledInv${espurnaIndex}`] = inverted ? '1' : '0';
                if (info.link) {
                    settings[`ledMode${espurnaIndex}`] = 'wifi';
                    link = true;
                } else {
                    leds.set(espurnaIndex, info.index);
                }
                break;

            case 'light':
                settings[`ltDimmerGPIO${espurnaIndex}`] = gpioPin.toString();
                settings[`ltInv${espurnaIndex}`] = inverted ? '1' : '0';
                break;
        }
    }

    // Tasmota LedN shows the state of RelayN (LedState 1), the first one is also
    // the Wi-Fi status LED when there is no LedLink
    for (const [index, tasmota] of leds) {
        const relay = relays.get(tasmota);
        if (!link && (index === 0)) {
            settings[`ledMode${index}`] = (relay !== undefined) ? 'relays-wifi' : 'wifi';
            link = true;
        } else if (relay !== undefined) {
            settings[`ledMode${index}`] = 'relay';
            settings[`ledRelay${index}`] = relay.toString();
        } else {
            settings[`ledMode${index}`] = 'manual';
        }
    }

    // Bulbs and strips without a relay pin, light is switched by its own (virtual) relay
    if (counters.light && !counters.relay) {
        settings['relayProv0'] = 'light-state';
    }

    return settings;
}

/**
 * @param {{[key: string]: string}} settings
 * @returns {string} relays, buttons, LEDs and light channels found
 */
export function importSummary(settings) {
    const count = (/** @type {string} */prefix) =>
        Object.keys(settings).filter((key) => key.startsWith(prefix)).length;

    let out = `${count('relayGpio')} relay(s), ${count('btnGpio')} button(s), ${count('ledGpio')} LED(s)`;
    const lights = count('ltDimmerGPIO');
    if (lights) {
        out += `, ${lights} light channel(s)`;
    }

    return out;
}

/**
 * @param {string} json
 * @returns {{[key: string]: string}}
 */
function settingsFromJson(json) {
    return tasmotaToEspurna(parseTasmotaTemplate(json));
}

/**
 * Import Tasmota template and send settings to the device.
 * @param {string} json
 */
function doImport(json) {
    const tmpl = parseTasmotaTemplate(json);
    const settings = tasmotaToEspurna(tmpl);

    if (Object.keys(settings).every((key) => key === 'sysDevice')) {
        window.alert('No supported GPIO functions found in this Tasmota template.');
        return;
    }

    const msg = `Import Tasmota template "${tmpl.NAME}"?\n\n${importSummary(settings)}\n\nNote: existing relay/button/LED settings will be overwritten.`;
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
            preview.textContent = `✓ ${importSummary(settingsFromJson(val))}`;
            preview.style.color = '#2a7';
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
