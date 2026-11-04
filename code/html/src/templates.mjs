import { listenAppConnected, send, sendAction } from './connection.mjs';
import { notifyError } from './notify.mjs';
import { variableListeners } from './settings.mjs';
import { templateChip, tasmotaToEspurna } from './tasmota.mjs';

// Device template search. Databases are fetched by the browser at the time of the search,
// straight from their authors (all of them allow CORS), nothing is copied or bundled:
// - Tasmota templates, https://templates.blakadder.com (ESP8266, ESP32)
// - OpenBeken devices, https://openbekeniot.github.io/webapp (BK72xx)
// - ESPurna devices, a devices.json on GitHub (every chip), boards that are not in either of them.
//   The default list can be replaced with the `webTplUrl` setting
// plus the boards of this firmware, which are not in either of them or need ESPurna-only settings

const TASMOTA_URL = 'https://templates.blakadder.com/templates.json';
const TASMOTA_LINK = 'https://templates.blakadder.com';
const OPENBEKEN_URL = 'https://openbekeniot.github.io/webapp/devices.json';
const OPENBEKEN_LINK = 'https://openbekeniot.github.io/webapp/devicesList.html';
export const GITHUB_URL = 'https://raw.githubusercontent.com/igorufox/espurna-devices/main/devices.json';

const RESULTS_MAX = 30;

/** @typedef {{[key: string]: string}} Settings */

/** @typedef {'esp8266' | 'esp32' | 'beken'} Arch */

/**
 * @typedef {{
 *   source: 'espurna' | 'tasmota' | 'openbeken' | 'github',
 *   name: string,
 *   model: string,
 *   category: string,
 *   link: string,
 *   settings: Settings,
 *   names: Settings,
 *   image?: string,
 *   notes?: string,
 * }} DeviceTemplate
 */

/**
 * Entry of the ESPurna devices.json, a JSON array of these. `mcu` limits the entry to one chip
 * (as reported by the device, e.g. BK7231N), `image` is the firmware image the board needs
 * @typedef {{
 *   name: string,
 *   model?: string,
 *   category?: string,
 *   arch: Arch,
 *   mcu?: string,
 *   image?: string,
 *   link?: string,
 *   notes?: string,
 *   settings: Settings,
 * }} DeviceEntry
 */

/**
 * Manufacturer and Device on the status page, MQTT `board` and Home Assistant
 * @param {string} manufacturer
 * @param {string} device
 * @returns {Settings}
 */
function names(manufacturer, device) {
    /** @type {Settings} */
    const out = {};
    if (manufacturer) {
        out['sysManufacturer'] = manufacturer;
    }
    if (device) {
        out['sysDevice'] = device;
    }

    return out;
}

/**
 * @param {number} index
 * @param {string} gpio
 * @param {boolean} inverse
 * @returns {Settings}
 */
function relay(index, gpio, inverse = false) {
    return {
        [`relayGpio${index}`]: gpio,
        [`relayType${index}`]: inverse ? 'inverse' : 'normal',
        [`relayProv${index}`]: 'gpio',
    };
}

/**
 * @param {number} index
 * @param {string} gpio
 * @param {number | null} relayIndex
 * @param {{pull?: string, defVal?: string}} options
 * @returns {Settings}
 */
function pushbutton(index, gpio, relayIndex, {pull = 'default', defVal = 'high'} = {}) {
    return {
        [`btnGpio${index}`]: gpio,
        [`btnMode${index}`]: 'pushbutton',
        [`btnPinMode${index}`]: pull,
        [`btnDefVal${index}`]: defVal,
        ...buttonRelay(index, relayIndex),
    };
}

/**
 * @param {number} index
 * @param {number | null} relayIndex
 * @returns {Settings}
 */
function buttonRelay(index, relayIndex) {
    return (relayIndex !== null) ? {[`btnRelay${index}`]: relayIndex.toString()} : {};
}

/**
 * Wall switch, toggles the relay on every change of position, no click actions
 * @param {number} index
 * @param {string} gpio
 * @param {number | null} relayIndex
 * @returns {Settings}
 */
function wallSwitch(index, gpio, relayIndex) {
    /** @type {Settings} */
    const out = {
        [`btnGpio${index}`]: gpio,
        [`btnMode${index}`]: 'switch',
        [`btnPinMode${index}`]: 'pull-up',
        [`btnDefVal${index}`]: 'initial',
        ...buttonRelay(index, relayIndex),
        [`btnPress${index}`]: 'relay-toggle',
        [`btnRlse${index}`]: 'relay-toggle',
    };

    for (const key of ['btnClick', 'btnDclk', 'btnTclk', 'btnLclk', 'btnLLclk']) {
        out[`${key}${index}`] = 'none';
    }

    return out;
}

/**
 * Optocoupled AC mains wall switch, ref. the AC mains switch button option
 * @param {number} index
 * @param {string} gpio
 * @param {number} relayIndex
 * @returns {Settings}
 */
function acSwitch(index, gpio, relayIndex) {
    return {
        ...wallSwitch(index, gpio, relayIndex),
        [`btnPinMode${index}`]: 'default',
        [`btnDefVal${index}`]: 'high',
        [`btnFreq${index}`]: '1',
        [`btnDebDel${index}`]: '100',
    };
}

/**
 * @param {number} index
 * @param {string} gpio
 * @param {boolean} inverse
 * @param {string} mode
 * @returns {Settings}
 */
function led(index, gpio, inverse, mode = 'wifi') {
    return {
        [`ledGpio${index}`]: gpio,
        [`ledInv${index}`]: inverse ? '1' : '0',
        [`ledMode${index}`]: mode,
    };
}

/**
 * Boards of this firmware, same pins as their hardware.h entries
 * @type {{arch: Arch, name: string, model: string, category: string, settings: Settings}[]}
 */
export const PROFILES = [
    {arch: 'esp8266', name: 'Sonoff Mini', model: 'ITEAD SONOFF_MINI', category: 'relay', settings: {
        ...relay(0, '12'), ...pushbutton(0, '0', 0), ...wallSwitch(1, '4', 0), ...led(0, '13', true)}},
    {arch: 'esp8266', name: 'Sonoff Basic', model: 'ITEAD SONOFF_BASIC', category: 'relay', settings: {
        ...relay(0, '12'), ...pushbutton(0, '0', 0), ...wallSwitch(1, '14', 0), ...led(0, '13', true)}},
    {arch: 'esp8266', name: 'Sonoff 4CH', model: 'ITEAD SONOFF_4CH', category: 'relay', settings: {
        ...relay(0, '12'), ...relay(1, '5'), ...relay(2, '4'), ...relay(3, '15'),
        ...pushbutton(0, '0', 0), ...pushbutton(1, '9', 1), ...pushbutton(2, '10', 2), ...pushbutton(3, '14', 3),
        ...led(0, '13', true)}},
    {arch: 'esp8266', name: 'Sonoff RF Bridge 433', model: 'ITEAD SONOFF_RFBRIDGE', category: 'misc', settings: {
        relayDummy: '8', ...pushbutton(0, '0', null), ...led(0, '13', true),
        uartTx0: '1', uartRx0: '3', uartBaud0: '19200'}},
    {arch: 'esp8266', name: 'Sonoff S20', model: 'ITEAD S20', category: 'plug', settings: {
        ...relay(0, '12'), ...pushbutton(0, '0', 0), ...led(0, '13', true)}},
    {arch: 'esp8266', name: 'Sonoff 1CH Inching', model: 'ITEAD 1CH_INCHING', category: 'relay', settings: {
        ...relay(0, '12'), ...pushbutton(0, '0', 0), ...led(0, '13', true)}},
    {arch: 'esp8266', name: 'LC-Tech ESP12F relay x2', model: 'LC_TECH ESP12F_RELAY_X2', category: 'relay', settings: {
        ...relay(0, '4'), ...relay(1, '5')}},
    {arch: 'esp8266', name: 'LC-Tech ESP12F relay x4', model: 'LC_TECH ESP12F_RELAY_X4', category: 'relay', settings: {
        ...relay(0, '16'), ...relay(1, '14'), ...relay(2, '12'), ...relay(3, '13'),
        ...pushbutton(0, '0', 0), ...led(0, '2', true), ...led(1, '5', false, 'manual')}},
    {arch: 'esp8266', name: 'HW-622 relay', model: 'GENERIC HW_622', category: 'relay', settings: {
        ...relay(0, '4'), ...pushbutton(0, '5', 0), ...led(0, '2', true)}},
    {arch: 'esp8266', name: 'Moes MS-104B', model: 'MOES MS_104B', category: 'switch', settings: {
        ...relay(0, '14'), ...relay(1, '15'),
        ...acSwitch(0, '13', 0), ...acSwitch(1, '12', 1), ...pushbutton(2, '2', 0),
        btnAcFreq: '0'}},
    {arch: 'esp32', name: 'LC-Tech ESP32 relay x2', model: 'LC_TECH ESP32_RELAY_X2', category: 'relay', settings: {
        ...relay(0, '16'), ...relay(1, '17'), ...pushbutton(0, '0', 0, {pull: 'pull-up'}), ...led(0, '23', true)}},
    {arch: 'esp32', name: 'Sonoff MINIR4 (Mini Extreme)', model: 'ITEAD SONOFF_MINIR4', category: 'relay', settings: {
        ...relay(0, '26'), ...pushbutton(0, '0', 0, {pull: 'pull-up'}), ...wallSwitch(1, '27', 0), ...led(0, '19', true)}},
    {arch: 'beken', name: 'Tuya TYWRA-RF relay 433 MHz (CB3S)', model: 'TUYA CB3S_RELAY_RF', category: 'relay', settings: {
        ...relay(0, '24'), ...pushbutton(0, '14', 0, {pull: 'pull-up'}), ...pushbutton(1, '6', 0, {pull: 'pull-up'}),
        ...led(0, '8', true), rfbRX: '26'}},
];

/**
 * templates.json is generated without escaping, some entries are not valid JSON
 * (e.g. `"template": Module 1` or an extra brace). Every entry is read on its own,
 * field by field, broken fields are dropped
 * @param {string} text
 * @returns {{[key: string]: any}[]}
 */
export function parseTasmotaTemplates(text) {
    /** @type {{[key: string]: any}[]} */
    const out = [];

    for (const chunk of text.split(/\n\s*\{\s*\n(?=\s*"name")/).slice(1)) {
        /** @type {{[key: string]: any}} */
        const entry = {};
        for (const match of chunk.matchAll(/^\s*"(\w+)":\s*(.*?),?\s*$/gm)) {
            const [, key, value] = match;
            for (const attempt of [value, value.replace(/\}+$/, '}')]) {
                try {
                    entry[key] = JSON.parse(attempt);
                    break;
                } catch {
                    continue;
                }
            }
        }

        if (typeof entry.name === 'string') {
            out.push(entry);
        }
    }

    return out;
}

/**
 * @param {{[key: string]: any}[]} entries
 * @param {Arch} arch
 * @returns {DeviceTemplate[]}
 */
export function tasmotaTemplates(entries, arch) {
    return entries
        .filter((entry) => (typeof entry.template === 'object')
            && Array.isArray(entry.template?.GPIO)
            && (templateChip(entry.template.GPIO.length) === arch)
            && hasPins(tasmotaToEspurna(entry.template)))
        .map((entry) => ({
            source: 'tasmota',
            name: entry.name,
            model: entry.model ?? '',
            category: entry.category ?? '',
            link: TASMOTA_LINK + (entry.link ?? ''),
            settings: tasmotaToEspurna(entry.template),
            // no vendor field, names start with it (Sonoff Basic R2, Shelly 1PM, Moes ...)
            names: names(String(entry.name).split(' ')[0],
                entry.model || String(entry.name).split(' ').slice(1).join(' ') || entry.name),
        }));
}

/**
 * OpenBeken pin roles, `{"<pin>": "<role>;<channel>"}`. Channels with a relay are the relays,
 * other inputs and LEDs follow the relay of their channel
 * @param {{[pin: string]: string}} pins
 * @returns {Settings}
 */
export function openbekenToEspurna(pins) {
    /** @type {Settings} */
    const settings = {};

    const roles = Object.entries(pins ?? {})
        .map(([pin, value]) => {
            const [role, channel] = String(value).split(';');
            return {pin, role, channel: parseInt(channel ?? '0', 10)};
        })
        .sort((lhs, rhs) => parseInt(lhs.pin, 10) - parseInt(rhs.pin, 10));

    /** @type {Map<number, number>} */
    const relays = new Map();
    for (const {pin, role, channel} of roles) {
        if ((role === 'Rel') || (role === 'Rel_n')) {
            const index = relays.size;
            relays.set(channel, index);
            Object.assign(settings, relay(index, pin, role === 'Rel_n'));
        }
    }

    const relayOf = (/** @type {number} */channel) =>
        relays.get(channel) ?? ((relays.size === 1) ? 0 : null);

    let buttons = 0;
    let leds = 0;
    let lights = 0;

    for (const {pin, role, channel} of roles) {
        switch (role) {
        case 'Btn':
        case 'Btn_n':
        case 'Btn_Tgl_All':
        case 'Btn_Tgl_All_n': {
            const inverse = role.endsWith('_n');
            const index = buttons++;
            Object.assign(settings, pushbutton(index, pin,
                (role.startsWith('Btn_Tgl_All') && relays.size) ? 0 : relayOf(channel),
                {pull: inverse ? 'default' : 'pull-up', defVal: inverse ? 'low' : 'high'}));
            break;
        }

        case 'TglChanOnTgl':
            Object.assign(settings, wallSwitch(buttons++, pin, relayOf(channel)));
            break;

        case 'WifiLED':
        case 'WifiLED_n':
            Object.assign(settings, led(leds++, pin, role.endsWith('_n')));
            break;

        case 'LED':
        case 'LED_n': {
            const index = leds++;
            const relayIndex = relayOf(channel);
            Object.assign(settings, led(index, pin, role.endsWith('_n'), (relayIndex !== null) ? 'relay' : 'manual'));
            if (relayIndex !== null) {
                settings[`ledRelay${index}`] = relayIndex.toString();
            }
            break;
        }

        case 'PWM':
        case 'PWM_n': {
            const index = lights++;
            settings[`ltDimmerGPIO${index}`] = pin;
            settings[`ltInv${index}`] = role.endsWith('_n') ? '1' : '0';
            break;
        }

        case 'RCRecv':
            settings['rfbRX'] = pin;
            break;
        }
    }

    if (lights && !relays.size) {
        settings['relayProv0'] = 'light-state';
    }

    return settings;
}

/**
 * @param {{devices: {[key: string]: any}[]}} data
 * @param {string} mcu e.g. BK7231N, only devices with the same chip are listed
 * @returns {DeviceTemplate[]}
 */
export function openbekenTemplates(data, mcu) {
    return (data?.devices ?? [])
        .filter((device) => device.pins
            && (String(device.chip ?? '').toUpperCase() === mcu.toUpperCase()))
        .map((device) => ({
            source: 'openbeken',
            name: [device.vendor, device.name].filter((x) => x).join(' '),
            model: [device.model, device.board].filter((x) => x).join(' '),
            category: (device.keywords ?? []).slice(0, 3).join(', '),
            link: device.wiki || OPENBEKEN_LINK,
            settings: openbekenToEspurna(device.pins),
            names: names(device.vendor ?? '', device.model || device.name),
        }))
        .filter((template) => hasPins(template.settings));
}

/**
 * Settings describing the board: pins and their modes, button actions and the board names.
 * Indexed keys are listed without the index. Network, passwords, MQTT, topics, sync and
 * the rest are never exported and never applied from a downloaded list
 */
const HARDWARE_KEYS = new Set([
    'relayGpio', 'relayGpioType', 'relayType', 'relayProv', 'relayResetGpio', 'relayDummy',
    'btnGpio', 'btnGpioType', 'btnProv', 'btnMode', 'btnPinMode', 'btnDefVal', 'btnRelay',
    'btnPress', 'btnRlse', 'btnClick', 'btnDclk', 'btnTclk', 'btnLclk', 'btnLLclk',
    'btnLclkDel', 'btnLLclkDel', 'btnDebDel', 'btnRepDel',
    'btnFreq', 'btnMinPulses', 'btnMinDuty', 'btnAcConfirm', 'btnAcFreq',
    'ledGpio', 'ledInv', 'ledMode', 'ledRelay',
    'ltDimmerGPIO', 'ltEnableGPIO', 'ltInv',
    'uartTx', 'uartRx', 'uartBaud', 'uartDataBits', 'uartParity', 'uartStopBits', 'uartInv',
    'rfbRX', 'rfbTX',
    'sysManufacturer', 'sysDevice',
]);

// in case the list above ever gets a key it should not have
const NEVER_KEYS = /pass|key|secret|token|ssid|wifi|mqtt|api|host|topic/i;

/**
 * @param {{[key: string]: any}} settings
 * @returns {Settings} only the keys of HARDWARE_KEYS, values as strings
 */
export function hardwareSettings(settings) {
    /** @type {Settings} */
    const out = {};
    for (const [key, value] of Object.entries(settings ?? {})) {
        if (HARDWARE_KEYS.has(key.replace(/\d+$/, ''))
            && !NEVER_KEYS.test(key)
            && ((typeof value === 'string') || (typeof value === 'number')))
        {
            out[key] = String(value);
        }
    }

    return out;
}

/**
 * ESPurna devices.json, entries for this chip. Only hardware keys are taken from the entry
 * @param {any} entries
 * @param {Arch} arch
 * @param {string} mcu
 * @returns {DeviceTemplate[]}
 */
export function githubTemplates(entries, arch, mcu) {
    return (Array.isArray(entries) ? entries : [])
        .filter((entry) => (typeof entry?.name === 'string')
            && (entry.arch === arch)
            && (!entry.mcu || (String(entry.mcu).toUpperCase() === mcu.toUpperCase()))
            && (typeof entry.settings === 'object'))
        .map((/** @type {DeviceEntry} */entry) => {
            const {sysManufacturer, sysDevice, ...settings} = hardwareSettings(entry.settings);
            return {
                source: /** @type {const} */('github'),
                name: entry.name,
                model: entry.model ?? '',
                category: entry.category ?? '',
                link: entry.link ?? '',
                settings,
                names: names(sysManufacturer ?? '', sysDevice ?? ''),
                image: entry.image ?? '',
                notes: entry.notes ?? '',
            };
        })
        .filter((template) => hasPins(template.settings));
}

/**
 * Settings backup of the device (/config). Values are written as they are stored, without
 * escaping, so it is read line by line, every setting is a `"key": "value"` line
 * @param {string} text
 * @returns {Settings}
 */
export function parseSettingsBackup(text) {
    /** @type {Settings} */
    const out = {};
    for (const [, key, value] of text.matchAll(/^"(\w+)": "(.*)",?$/gm)) {
        out[key] = value;
    }

    return out;
}

/**
 * devices.json entry made of the hardware settings of this device. Names fall back to
 * the ones the device reported (build values), image and notes are left for the user
 * @param {Settings} backup
 * @param {{arch: Arch, mcu: string, manufacturer: string, device: string}} device
 * @returns {DeviceEntry}
 */
export function exportTemplate(backup, {arch, mcu, manufacturer, device}) {
    const settings = hardwareSettings(backup);
    for (const [key, value] of Object.entries(names(manufacturer, device))) {
        settings[key] ??= value;
    }

    const name = [settings['sysManufacturer'], settings['sysDevice']]
        .filter((x) => x).join(' ');

    return {
        name: name || 'My device',
        model: settings['sysDevice'] ?? '',
        arch,
        // ESP8266 and ESP32 entries are not limited to the chip revision
        ...((arch === 'beken') ? {mcu} : {}),
        image: '',
        notes: '',
        settings,
    };
}

/**
 * @param {Settings} settings
 * @returns {boolean} at least one relay, button, LED or light channel
 */
function hasPins(settings) {
    return Object.keys(settings).some((key) => /^(relayGpio|btnGpio|ledGpio|ltDimmerGPIO)/.test(key));
}

/**
 * @param {Arch} arch
 * @returns {DeviceTemplate[]}
 */
export function espurnaTemplates(arch) {
    return PROFILES
        .filter((profile) => profile.arch === arch)
        .map((profile) => ({
            source: 'espurna',
            name: profile.name,
            model: profile.model,
            category: profile.category,
            link: '',
            settings: profile.settings,
            // same as the board build, e.g. ITEAD / SONOFF_MINI
            names: names(profile.model.split(' ')[0], profile.model.split(' ').slice(1).join(' ')),
        }));
}

/**
 * Every word of the query has to be found in the name, model or category
 * @param {DeviceTemplate[]} templates
 * @param {string} query
 * @returns {DeviceTemplate[]}
 */
export function searchTemplates(templates, query) {
    const words = query.toLowerCase().split(/\s+/).filter((x) => x);
    if (!words.length) {
        return [];
    }

    return templates.filter((template) => {
        const text = `${template.name} ${template.model} ${template.category}`.toLowerCase();
        return words.every((word) => text.includes(word));
    });
}

/**
 * @param {Settings} settings
 * @returns {string}
 */
export function settingsSummary(settings) {
    const count = (/** @type {string} */prefix) =>
        Object.keys(settings).filter((key) => key.startsWith(prefix)).length;

    const parts = [
        `${count('relayGpio')} relay(s)`,
        `${count('btnGpio')} button(s)`,
        `${count('ledGpio')} LED(s)`,
    ];

    const lights = count('ltDimmerGPIO');
    if (lights) {
        parts.push(`${lights} light channel(s)`);
    }

    const ac = count('btnFreq');
    if (ac) {
        parts.push(`${ac} AC mains switch(es)`);
    }

    if (settings['relayDummy']) {
        parts.push(`${settings['relayDummy']} dummy relay(s)`);
    }

    if (settings['rfbRX'] || settings['uartBaud0']) {
        parts.push('RF 433 MHz');
    }

    return parts.join(', ');
}

/**
 * Downloaded lists are kept by their URL, failed ones are tried again with the next search
 * @type {{
 *   arch: Arch | null,
 *   mcu: string,
 *   manufacturer: string,
 *   device: string,
 *   url: string,
 *   config: URL | null,
 *   cache: Map<string, DeviceTemplate[]>,
 *   failed: Map<string, string>,
 * }}
 */
const state = {
    arch: null,
    mcu: '',
    manufacturer: '',
    device: '',
    url: '',
    config: null,
    cache: new Map(),
    failed: new Map(),
};

function resetLoaded() {
    state.cache.clear();
    state.failed.clear();
}

/** @returns {string} */
function githubUrl() {
    return state.url.trim() || GITHUB_URL;
}

/**
 * @param {string} name
 * @returns {Arch | null}
 */
function archFromName(name) {
    switch (name) {
    case 'esp8266':
    case 'esp32':
    case 'beken':
        return name;
    }

    return null;
}

/**
 * @typedef {{
 *   url: string,
 *   load: (response: Response) => Promise<DeviceTemplate[]>,
 * }} RemoteSource
 */

/**
 * @param {Arch} arch
 * @returns {RemoteSource[]}
 */
function remoteSources(arch) {
    /** @type {RemoteSource} */
    const github = {
        url: githubUrl(),
        load: async (response) => githubTemplates(await response.json(), arch, state.mcu),
    };

    if (arch === 'beken') {
        return [{
            url: OPENBEKEN_URL,
            load: async (response) => openbekenTemplates(await response.json(), state.mcu),
        }, github];
    }

    return [{
        url: TASMOTA_URL,
        load: async (response) => tasmotaTemplates(parseTasmotaTemplates(await response.text()), arch),
    }, github];
}

/**
 * One list that could not be downloaded does not stop the others
 * @param {RemoteSource} remote
 * @returns {Promise<DeviceTemplate[]>}
 */
async function loadRemote({url, load}) {
    const cached = state.cache.get(url);
    if (cached) {
        return cached;
    }

    try {
        const response = await fetch(url);
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }

        const templates = await load(response);
        state.cache.set(url, templates);
        state.failed.delete(url);
        return templates;
    } catch (e) {
        state.failed.set(url, /** @type {Error} */(e).message);
        return [];
    }
}

/** @returns {Promise<DeviceTemplate[]>} */
async function loadTemplates() {
    const arch = state.arch;
    if (!arch) {
        throw new Error('Device architecture is not known yet');
    }

    const remote = await Promise.all(remoteSources(arch).map(loadRemote));
    return [...espurnaTemplates(arch), ...remote.flat()];
}

/**
 * @param {DeviceTemplate} template
 * @returns {string} firmware image the board needs, when the list says so
 */
function imageText(template) {
    return template.image ? `needs the ${template.image} image` : '';
}

/** @param {DeviceTemplate} template */
function applySettings(template) {
    const summary = settingsSummary(template.settings);
    // the device does not know which image it runs, only the list does
    const image = template.image
        ? `\n\nThis board ${imageText(template)}, make sure the device runs it.`
        : '';
    const notes = template.notes ? `\n\n${template.notes}` : '';
    if (!window.confirm(`Apply "${template.name}"?\n\n${summary}${image}${notes}\n\nNote: existing relay/button/LED settings will be overwritten.`)) {
        return;
    }

    send(JSON.stringify({
        settings: {
            set: {...template.settings, ...template.names},
            del: [],
        }
    }));

    setTimeout(() => {
        if (window.confirm('Settings have been sent to the device!\n\nReboot now to apply?')) {
            sendAction('reboot');
        }
    }, 500);
}

/**
 * @param {DeviceTemplate} template
 * @returns {HTMLElement}
 */
function resultRow(template) {
    const row = document.createElement('li');
    row.classList.add('template-result');

    const button = document.createElement('button');
    button.type = 'button';
    button.classList.add('pure-button', 'button-template-apply');
    button.textContent = 'Apply';
    button.addEventListener('click', () => {
        try {
            applySettings(template);
        } catch (e) {
            notifyError(/** @type {Error} */(e));
        }
    });
    row.appendChild(button);

    const text = document.createElement('span');
    text.textContent = ` ${template.name}`
        + (template.model ? ` (${template.model})` : '')
        + ` - ${settingsSummary(template.settings)}`
        + (template.image ? `, ${imageText(template)}` : '');
    if (template.notes) {
        text.title = template.notes;
    }
    row.appendChild(text);

    if (template.link) {
        const link = document.createElement('a');
        link.href = template.link;
        link.target = '_blank';
        link.rel = 'noopener';
        link.textContent = ` [${template.source}]`;
        row.appendChild(link);
    } else {
        const tag = document.createElement('span');
        tag.textContent = ` [${template.source}]`;
        row.appendChild(tag);
    }

    return row;
}

/**
 * Which lists are searched for this chip and, once downloaded, how many devices they have
 * @param {DeviceTemplate[] | null} loaded
 * @returns {string}
 */
function sourcesText(loaded) {
    const count = (/** @type {DeviceTemplate['source']} */source, url = '') => {
        const failed = state.failed.get(url);
        if (failed) {
            return ` (unavailable, ${failed})`;
        }

        return loaded
            ? ` (${loaded.filter((template) => template.source === source).length})`
            : '';
    };

    const github = `ESPurna devices (GitHub)${count('github', githubUrl())}`;

    switch (state.arch) {
    case 'beken':
        return `boards of this firmware${count('espurna')}, OpenBeken ${state.mcu} devices${count('openbeken', OPENBEKEN_URL)} and ${github}`;
    case 'esp8266':
    case 'esp32':
        return `boards of this firmware${count('espurna')}, Tasmota ${state.arch.toUpperCase()} templates${count('tasmota', TASMOTA_URL)} and ${github}`;
    }

    return 'nothing yet, the device did not tell its chip';
}

/**
 * Hardware settings of this device, read from its settings backup (/config), which has every
 * stored key. The WebSocket only sends what the panels show, e.g. no button actions
 * @returns {Promise<string>}
 */
async function exportText() {
    if (!state.config || !state.arch) {
        throw new Error('not connected');
    }

    const response = await fetch(state.config.href, {
        credentials: 'same-origin',
        cache: 'no-store',
    });
    if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
    }

    const entry = exportTemplate(parseSettingsBackup(await response.text()), {
        arch: state.arch,
        mcu: state.mcu,
        manufacturer: state.manufacturer,
        device: state.device,
    });

    return JSON.stringify(entry, null, 4);
}

/**
 * Clipboard API only works on https:// and localhost, the device is plain http://
 * @param {HTMLTextAreaElement} textarea
 */
async function copyText(textarea) {
    if (navigator.clipboard && window.isSecureContext) {
        await navigator.clipboard.writeText(textarea.value);
        return;
    }

    textarea.select();
    if (!document.execCommand('copy')) {
        throw new Error('not supported by the browser');
    }
}

function initExport() {
    const textarea = /** @type {HTMLTextAreaElement | null} */
        (document.getElementById('template-export'));
    const button = document.getElementById('button-template-export');
    const copy = document.getElementById('button-template-copy');
    const status = document.getElementById('template-export-status');

    if (!textarea || !button || !copy || !status) {
        return;
    }

    listenAppConnected((urls) => {
        state.config = urls.config;
    });

    button.addEventListener('click', async () => {
        status.textContent = 'Loading...';
        try {
            textarea.value = await exportText();
            status.textContent = 'Fill in name, image and notes, then add it to devices.json';
        } catch (e) {
            status.textContent = `Could not read the settings (${/** @type {Error} */(e).message})`;
        }
    });

    copy.addEventListener('click', async () => {
        if (!textarea.value) {
            return;
        }

        try {
            await copyText(textarea);
            status.textContent = 'Copied';
        } catch (e) {
            status.textContent = `Could not copy (${/** @type {Error} */(e).message}), select the text and copy it by hand`;
        }
    });
}

export function init() {
    const input = /** @type {HTMLInputElement | null} */
        (document.getElementById('template-search'));
    const button = document.getElementById('button-template-search');
    const results = document.getElementById('template-results');
    const status = document.getElementById('template-status');
    const sources = document.getElementById('template-sources');
    const url = /** @type {HTMLInputElement | null} */
        (document.querySelector('input[name="webTplUrl"]'));

    if (!input || !button || !results || !status || !sources || !url) {
        return;
    }

    variableListeners({
        'arch': (_, value) => {
            state.arch = archFromName(String(value));
            resetLoaded();
            sources.textContent = sourcesText(null);
        },
        'mcu': (_, value) => {
            state.mcu = String(value);
            resetLoaded();
            sources.textContent = sourcesText(null);
        },
        'manufacturer': (_, value) => {
            state.manufacturer = String(value);
        },
        'device': (_, value) => {
            state.device = String(value);
        },
        'webTplUrl': (_, value) => {
            state.url = String(value);
            sources.textContent = sourcesText(null);
        },
    });

    // used as typed, Save keeps it on the device for the next time
    url.addEventListener('change', () => {
        state.url = url.value;
        sources.textContent = sourcesText(null);
    });

    const search = async () => {
        results.replaceChildren();
        if (!input.value.trim()) {
            return;
        }

        status.textContent = 'Loading...';
        try {
            const loaded = await loadTemplates();
            sources.textContent = sourcesText(loaded);

            const found = searchTemplates(loaded, input.value);
            status.textContent = found.length
                ? `${found.length} found` + ((found.length > RESULTS_MAX) ? `, first ${RESULTS_MAX} shown` : '')
                : 'Nothing found';
            if (state.failed.size && !state.cache.size) {
                status.textContent += '. The device lists could not be downloaded,'
                    + ' the browser needs internet access, the device itself does not.';
            }
            results.replaceChildren(...found.slice(0, RESULTS_MAX).map(resultRow));
        } catch (e) {
            status.textContent = `Could not load the device list (${/** @type {Error} */(e).message}).`;
        }
    };

    button.addEventListener('click', search);
    input.addEventListener('keydown', (event) => {
        if (event.key === 'Enter') {
            event.preventDefault();
            search();
        }
    });

    initExport();
}
