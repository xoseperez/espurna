import { expect, test } from 'vitest';
import {
    PROFILES,
    espurnaTemplates,
    openbekenTemplates,
    openbekenToEspurna,
    parseTasmotaTemplates,
    searchTemplates,
    settingsSummary,
    tasmotaTemplates,
} from '../src/templates.mjs';

// shape of templates.blakadder.com/templates.json, including its broken entries
const TASMOTA_JSON = `{
    "created": "2026-07-06 08:01:19 +0000",
    "templates":
    [

        {
        "name":     "Moes",
        "model":     "MS-104B",
        "link":     "/moes_MS-104B.html",
        "type":     "Switch Module",
        "category": "relay",
        "standard": ["GLOBAL"],
        "template": {"NAME":"Moes MS-104B","GPIO":[0,0,17,0,160,0,0,0,10,9,21,22,0],"FLAG":0,"BASE":18},
        "image":    "/assets/device_images/moes_MS-104B.webp",
        "product":  ""
        },

        {
        "name":     "ESP32-C3-C05 ESP32-C3 4Mb",
        "model":     "DT-ESP-C05",
        "link":     "/DT-ESP-C05.html",
        "type":     "Module",
        "category": "diy",
        "standard": ["GLOBAL"],
        "template": Module 1,
        "image":    "/assets/device_images/DT-ESP-C05.webp",
        "product":  ""
        },

        {
        "name":     "M5Stack AtomU",
        "link":     "/m5stack_AtomU.html",
        "type":     "Development Board",
        "category": "diy",
        "standard": ["GLOBAL"],
        "template": {"NAME":"M5Stack AtomU","GPIO":[0,0,0,0,0,0,0,0,1056,0,0,0,0,0,0,0,0,0,0,0,0,0,640,1376,0,0,0,0,608,0,0,0,0,0,0,32],"FLAG":0,"BASE":1}},
        "image":    "/assets/device_images/m5stack_AtomU.webp",
        "product":  ""
        }
    ]
}`;

test('every templates.json entry is read on its own, broken fields are dropped', () => {
    const entries = parseTasmotaTemplates(TASMOTA_JSON);
    expect(entries.map((x) => x.name))
        .toEqual(['Moes', 'ESP32-C3-C05 ESP32-C3 4Mb', 'M5Stack AtomU']);

    expect(entries[0].template.GPIO).toHaveLength(13);
    expect(entries[1].template).toBeUndefined();
    expect(entries[2].template.GPIO).toHaveLength(36);
});

test('Tasmota templates are listed for the chip they were made for', () => {
    const entries = parseTasmotaTemplates(TASMOTA_JSON);
    expect(tasmotaTemplates(entries, 'esp8266').map((x) => x.model)).toEqual(['MS-104B']);
    expect(tasmotaTemplates(entries, 'esp32').map((x) => x.name)).toEqual(['M5Stack AtomU']);
    expect(tasmotaTemplates(entries, 'beken')).toHaveLength(0);
});

// openbekeniot.github.io/webapp/devices.json
const OPENBEKEN = {
    version: '0.1',
    devices: [
        {vendor: 'Tuya', name: 'Smart Switch', model: 'ATLO-SW1-TUYA', chip: 'BK7231N', board: 'CB3S',
            pins: {'6': 'Rel;1', '8': 'TglChanOnTgl;1', '9': 'WifiLED_n;0', '26': 'Btn_Tgl_All;0'}},
        {vendor: 'Tuya', name: 'Bulb', chip: 'BK7231T', board: 'WB2L',
            pins: {'6': 'PWM;0', '7': 'PWM;1', '8': 'PWM;2'}},
        {vendor: 'Tuya', name: 'No pins', chip: 'BK7231N', board: 'CB3S'},
    ],
};

test('OpenBeken roles become relays, switches, buttons and LEDs', () => {
    const settings = openbekenToEspurna(OPENBEKEN.devices[0].pins);

    expect(settings['relayGpio0']).toEqual('6');
    expect(settings['relayType0']).toEqual('normal');

    // pin order: P8 wall switch, P26 button
    expect(settings['btnGpio0']).toEqual('8');
    expect(settings['btnMode0']).toEqual('switch');
    expect(settings['btnRelay0']).toEqual('0');
    expect(settings['btnLclk0']).toEqual('none');

    expect(settings['btnGpio1']).toEqual('26');
    expect(settings['btnMode1']).toEqual('pushbutton');
    expect(settings['btnPinMode1']).toEqual('pull-up');
    expect(settings['btnRelay1']).toEqual('0');

    expect(settings['ledGpio0']).toEqual('9');
    expect(settings['ledInv0']).toEqual('1');
    expect(settings['ledMode0']).toEqual('wifi');
});

test('OpenBeken PWM channels are lights', () => {
    const settings = openbekenToEspurna(OPENBEKEN.devices[1].pins);
    expect(settings['ltDimmerGPIO0']).toEqual('6');
    expect(settings['ltDimmerGPIO2']).toEqual('8');
    expect(settings['relayProv0']).toEqual('light-state');
});

test('OpenBeken devices are listed for the same chip only', () => {
    const found = openbekenTemplates(OPENBEKEN, 'BK7231N');
    expect(found.map((x) => x.name)).toEqual(['Tuya Smart Switch']);
    expect(found[0].model).toEqual('ATLO-SW1-TUYA CB3S');
});

test('profiles of this firmware match their board builds', () => {
    const mini = PROFILES.find((x) => x.model === 'ITEAD SONOFF_MINI');
    expect(mini?.settings['relayGpio0']).toEqual('12');
    expect(mini?.settings['btnGpio1']).toEqual('4');
    expect(mini?.settings['btnMode1']).toEqual('switch');
    expect(mini?.settings['ledGpio0']).toEqual('13');

    const moes = PROFILES.find((x) => x.model === 'MOES MS_104B');
    expect(moes?.settings['btnFreq0']).toEqual('1');
    expect(moes?.settings['btnGpio0']).toEqual('13');
    expect(moes?.settings['btnRelay1']).toEqual('1');
    expect(moes?.settings['btnRelay2']).toEqual('0');

    const rf = PROFILES.find((x) => x.model === 'ITEAD SONOFF_RFBRIDGE');
    expect(rf?.settings['btnRelay0']).toBeUndefined();
    expect(rf?.settings['relayDummy']).toEqual('8');

    expect(espurnaTemplates('beken').map((x) => x.model)).toEqual(['TUYA CB3S_RELAY_RF']);
    expect(espurnaTemplates('esp32')).toHaveLength(2);
});

test('every word of the query has to match', () => {
    const templates = espurnaTemplates('esp8266');
    expect(searchTemplates(templates, 'sonoff mini').map((x) => x.name)).toEqual(['Sonoff Mini']);
    expect(searchTemplates(templates, 'lc-tech x4').map((x) => x.name)).toEqual(['LC-Tech ESP12F relay x4']);
    expect(searchTemplates(templates, '   ')).toHaveLength(0);
});

test('summary', () => {
    const rf = PROFILES.find((x) => x.model === 'ITEAD SONOFF_RFBRIDGE');
    expect(settingsSummary(rf?.settings ?? {}))
        .toEqual('0 relay(s), 1 button(s), 1 LED(s), 8 dummy relay(s), RF 433 MHz');
});

test('templates name the manufacturer and the device, the description is left alone', () => {
    const [profile] = searchTemplates(espurnaTemplates('beken'), 'tywra');
    expect(profile.names).toEqual({sysManufacturer: 'TUYA', sysDevice: 'CB3S_RELAY_RF'});

    const [obk] = openbekenTemplates(OPENBEKEN, 'BK7231N');
    expect(obk.names).toEqual({sysManufacturer: 'Tuya', sysDevice: 'ATLO-SW1-TUYA'});

    const [moes] = tasmotaTemplates(parseTasmotaTemplates(TASMOTA_JSON), 'esp8266');
    expect(moes.names).toEqual({sysManufacturer: 'Moes', sysDevice: 'MS-104B'});
});
