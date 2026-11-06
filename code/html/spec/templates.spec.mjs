import { expect, test } from 'vitest';
import {
    PROFILES,
    espurnaTemplates,
    exportTemplate,
    githubTemplates,
    hardwareSettings,
    hasPins,
    openbekenTemplates,
    openbekenToEspurna,
    parseSettingsBackup,
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

// ESPurna devices.json, a JSON array of entries
const GITHUB = [
    {name: 'Sparkleiot MINI (XH-CB2S)', arch: 'beken', mcu: 'BK7231N', image: 'beken-bk7231n-nokey',
        notes: 'S1 is tied to N', settings: {
            relayGpio0: '24', relayType0: 'normal', relayDummy: '1',
            btnGpio0: '7', btnMode0: 'pushbutton', btnRelay0: '0',
            ledGpio0: '6', ledInv0: '1', ledMode0: 'wifi',
            sysManufacturer: 'SPARKLEIOT', sysDevice: 'MINI_XH_CB2S',
            // not hardware, never applied from a list
            mqttServer: 'evil.example.com', adminPass: 'x', ssid0: 'evil', relayTopicSub0: 'a/b',
        }},
    {name: 'Other Beken chip', arch: 'beken', mcu: 'BK7231T', settings: {relayGpio0: '6'}},
    {name: 'Any ESP8266', arch: 'esp8266', settings: {relayGpio0: 12, ledGpio0: '13'}},
    {name: 'No pins', arch: 'esp8266', settings: {sysDevice: 'NOTHING'}},
    {name: 'No settings', arch: 'esp8266'},
    'garbage',
];

test('ESPurna devices are listed for their chip, mcu is optional', () => {
    expect(githubTemplates(GITHUB, 'beken', 'BK7231N').map((x) => x.name))
        .toEqual(['Sparkleiot MINI (XH-CB2S)']);
    expect(githubTemplates(GITHUB, 'beken', 'bk7231t').map((x) => x.name))
        .toEqual(['Other Beken chip']);
    expect(githubTemplates(GITHUB, 'esp8266', 'ESP8266').map((x) => x.name))
        .toEqual(['Any ESP8266']);
    expect(githubTemplates(GITHUB, 'esp32', 'ESP32-D0WD-V3')).toHaveLength(0);
    expect(githubTemplates({devices: GITHUB}, 'beken', 'BK7231N')).toHaveLength(0);
    expect(githubTemplates(null, 'beken', 'BK7231N')).toHaveLength(0);
});

test('ESPurna devices only bring hardware settings, names and the image', () => {
    const [mini] = githubTemplates(GITHUB, 'beken', 'BK7231N');
    expect(mini.source).toEqual('github');
    expect(mini.image).toEqual('beken-bk7231n-nokey');
    expect(mini.notes).toEqual('S1 is tied to N');
    expect(mini.names).toEqual({sysManufacturer: 'SPARKLEIOT', sysDevice: 'MINI_XH_CB2S'});
    expect(mini.settings).toEqual({
        relayGpio0: '24', relayType0: 'normal', relayDummy: '1',
        btnGpio0: '7', btnMode0: 'pushbutton', btnRelay0: '0',
        ledGpio0: '6', ledInv0: '1', ledMode0: 'wifi',
    });

    const [esp] = githubTemplates(GITHUB, 'esp8266', 'ESP8266');
    expect(esp.settings['relayGpio0']).toEqual('12');
    expect(esp.names).toEqual({});
});

// /config of the device, values are not escaped
const BACKUP = [
    '{',
    '"app": "ESPURNA",',
    '"version": "1.22.0-dev",',
    '"backup": "1",',
    '"adminPass": "fibonacci1",',
    '"apiKey": "0123456789ABCDEF",',
    '"btnDefVal0": "high",',
    '"btnGpio0": "7",',
    '"btnGpio1": "26",',
    '"btnMode0": "pushbutton",',
    '"btnMode1": "switch",',
    '"btnPinMode0": "pull-up",',
    '"btnPress1": "relay-toggle",',
    '"btnRelay0": "0",',
    '"btnRelay1": "1",',
    '"btnRlse1": "relay-toggle",',
    '"btnTermCmd0": "reboot",',
    '"desc": "Coridor",',
    '"haEnabled": "1",',
    '"hostname": "CORIDOR12",',
    '"ledGpio0": "6",',
    '"ledInv0": "1",',
    '"ledMode0": "wifi",',
    '"mqttPassword": "mqtt-secret",',
    '"mqttServer": "mqtt.lan",',
    '"mqttUser": "espurna",',
    '"ntpTZ": "EET",',
    '"pass0": "with "quotes" and \\ backslash",',
    '"relayDummy": "1",',
    '"relayGpio0": "24",',
    '"relayName0": "Facade",',
    '"relaySync": "1",',
    '"relayTopicSub1": "ESPURNA-SYNC/Outside/Garbage",',
    '"ssid0": "foxy",',
    '"sysManufacturer": "SPARKLEIOT",',
    '"sysDevice": "MINI_XH_CB2S",',
    '"telnetAuth": "1",',
    '"wifiApPass": "hunter2222",',
    '"wifiApSsid": "igoru",',
    '"wsAuth": "1"',
    '}',
].join('\n');

test('settings backup is read line by line, values as they are', () => {
    const backup = parseSettingsBackup(BACKUP);
    expect(backup['pass0']).toEqual('with "quotes" and \\ backslash');
    expect(backup['relayGpio0']).toEqual('24');
    expect(backup['wsAuth']).toEqual('1');
    expect(backup['app']).toEqual('ESPURNA');
});

const SECRETS = [
    'adminPass', 'apiKey', 'desc', 'haEnabled', 'hostname', 'mqttPassword', 'mqttServer', 'mqttUser',
    'ntpTZ', 'pass0', 'relayName0', 'relaySync', 'relayTopicSub1', 'ssid0', 'telnetAuth',
    'wifiApPass', 'wifiApSsid', 'wsAuth', 'btnTermCmd0', 'app', 'version',
];

test('export takes hardware keys only, secrets never get in', () => {
    const backup = parseSettingsBackup(BACKUP);
    const entry = exportTemplate(backup, {arch: 'beken', mcu: 'BK7231N', manufacturer: 'GENERIC', device: 'CB2S'});

    expect(Object.keys(entry.settings).sort()).toEqual([
        'btnDefVal0', 'btnGpio0', 'btnGpio1', 'btnMode0', 'btnMode1', 'btnPinMode0', 'btnPress1',
        'btnRelay0', 'btnRelay1', 'btnRlse1', 'ledGpio0', 'ledInv0', 'ledMode0', 'relayDummy', 'relayGpio0',
        'sysDevice', 'sysManufacturer',
    ]);

    const text = JSON.stringify(entry);
    for (const key of SECRETS) {
        expect(entry.settings[key]).toBeUndefined();
        expect(text).not.toContain(`"${key}"`);
    }

    for (const value of ['fibonacci1', '0123456789ABCDEF', 'mqtt-secret', 'mqtt.lan', 'foxy',
        'hunter2222', 'igoru', 'CORIDOR12', 'Coridor', 'quotes', 'Facade', 'ESPURNA-SYNC'])
    {
        expect(text).not.toContain(value);
    }

    // stored names win over the build ones
    expect(entry.name).toEqual('SPARKLEIOT MINI_XH_CB2S');
    expect(entry.model).toEqual('MINI_XH_CB2S');
    expect(entry.arch).toEqual('beken');
    expect(entry.mcu).toEqual('BK7231N');
    expect(entry.image).toEqual('');
});

test('export falls back to the build names, ESP entries are not limited to the chip revision', () => {
    const entry = exportTemplate({relayGpio0: '12', mqttServer: 'mqtt.lan'},
        {arch: 'esp32', mcu: 'ESP32-D0WD-V3', manufacturer: 'LC_TECH', device: 'ESP32_RELAY_X2'});
    expect(entry.settings).toEqual({relayGpio0: '12', sysManufacturer: 'LC_TECH', sysDevice: 'ESP32_RELAY_X2'});
    expect(entry.mcu).toBeUndefined();
});

test('exported entry is found again in the list with the same settings', () => {
    const entry = exportTemplate(parseSettingsBackup(BACKUP),
        {arch: 'beken', mcu: 'BK7231N', manufacturer: '', device: ''});
    const [template] = githubTemplates(JSON.parse(JSON.stringify([entry])), 'beken', 'BK7231N');
    expect({...template.settings, ...template.names}).toEqual(entry.settings);
});

// generated devices.json entries (scripts/boards_to_devices.py), sensor drivers and their pins
const GENERATED = [
    {name: 'Generic ESP01S DHT11 10', arch: 'esp8266', mcu: 'ESP8266', image: 'esp8266-1m-sensor', settings: {
        dhtPin: '2', dhtType: '11', sysManufacturer: 'GENERIC', sysDevice: 'ESP01S_DHT11_V10'}},
    {name: 'Generic V9261F', arch: 'esp8266', settings: {
        v9261fPort: '1', uartTx0: '153', uartRx0: '3', uartBaud0: '4800', uartParity0: 'odd', uartInv0: '1'}},
    {name: 'Ehomediy WT02', arch: 'esp8266', settings: {i2cSDA: '0', i2cSCL: '2', bmx280Number: '1'}},
    {name: 'Generic PZEM004T', arch: 'esp8266', settings: {
        pzemPort: '1', uartTx0: '1', uartRx0: '3', uartBaud0: '9600'}},
    {name: 'Itead Sonoff POW R2', arch: 'esp8266', settings: {
        relayGpio0: '12', cse7766Port: '1', uartTx0: '153', uartRx0: '3', uartBaud0: '4800',
        currRatio0: '1.25', voltRatio0: '0.9', pwrPRatio0: '1.1'}},
    {name: 'Gosund P1', arch: 'esp8266', settings: {
        btnGpio1: '17', btnProv1: 'analog', btnLevel1: '220',
        hlw8012SEL: '12', hlw8012CF: '4', hlw8012CF1: '5', hlw8012SelCurr: '1', hlw8012IntMode: '2'}},
    {name: 'Magichome LED controller', arch: 'esp8266', settings: {
        ltDimmerGPIO0: '14', irRx: '4', irRxPreset: '1'}},
    {name: 'PSH WIFI Sensor', arch: 'esp8266', settings: {
        dhtPin: '14', dhtType: '12', ldrPin: '17', ldrType: '2', ldrOnGround: '0', ldrResistor: '10000',
        ntcPin: '17', ntcBeta: '3350', ntcR0: '10000', ntcRUp: '0', ntcRDown: '10000'}},
    {name: 'Sonoff RF Bridge', arch: 'esp8266', settings: {
        relayDummy: '8', uartTx0: '1', uartRx0: '3', uartBaud0: '19200', rfbProv: 'efm8bb1'}},
    {name: 'UART off only', arch: 'esp8266', settings: {uartTx0: '153', uartRx0: '153', ech1560Inv: '1'}},
];

test('every hardware key of the generated entries is applied', () => {
    for (const entry of GENERATED.filter((x) => x.name !== 'UART off only')) {
        const {sysManufacturer, sysDevice, ...settings} = /** @type {{[key: string]: string}} */(entry.settings);
        const [template] = githubTemplates([entry], 'esp8266', 'ESP8266');
        expect(template?.settings).toEqual(settings);
        expect(template?.names).toEqual(sysDevice ? {sysManufacturer, sysDevice} : {});
    }
});

test('entries with any hardware pin are listed, GPIO_NONE is no pin', () => {
    expect(githubTemplates(GENERATED, 'esp8266', 'ESP8266').map((x) => x.name))
        .toEqual(GENERATED.map((x) => x.name).filter((x) => x !== 'UART off only'));

    expect(hasPins({dallasPin: '2'})).toBe(true);
    expect(hasPins({hlw8012CF1: '5'})).toBe(true);
    expect(hasPins({irRx: '4'})).toBe(true);
    expect(hasPins({uartRx0: '3'})).toBe(true);
    expect(hasPins({uartTx0: '153', rfbProv: 'efm8bb1'})).toBe(false);
    expect(hasPins({dhtType: '11', pzemPort: '1', ntcBeta: '3350'})).toBe(false);
    expect(hasPins({relayDummy: '8', sysDevice: 'X'})).toBe(false);
});

test('summary names RF, IR and the sensor drivers', () => {
    const summary = (/** @type {string} */name) => settingsSummary(
        GENERATED.find((x) => x.name === name)?.settings ?? {});

    expect(summary('PSH WIFI Sensor'))
        .toEqual('0 relay(s), 0 button(s), 0 LED(s), sensor(s): dht, ldr, ntc');
    expect(summary('Magichome LED controller'))
        .toEqual('0 relay(s), 0 button(s), 0 LED(s), 1 light channel(s), IR');
    expect(summary('Gosund P1'))
        .toEqual('0 relay(s), 1 button(s), 0 LED(s), sensor(s): hlw8012');
    // UART at 9600 is a meter, not the RF Bridge
    expect(summary('Generic PZEM004T'))
        .toEqual('0 relay(s), 0 button(s), 0 LED(s), sensor(s): pzem');
    expect(summary('Sonoff RF Bridge'))
        .toEqual('0 relay(s), 0 button(s), 0 LED(s), 8 dummy relay(s), RF 433 MHz');
});

test('no allow-listed key looks like a secret or a network setting', () => {
    const keys = Object.keys(hardwareSettings(Object.fromEntries(
        GENERATED.flatMap((x) => Object.entries(x.settings)))));
    expect(keys.length).toBeGreaterThan(40);

    // the same test the list itself makes, ref. NEVER_KEYS
    for (const key of keys) {
        expect(key).not.toMatch(/pass|key|secret|token|ssid|wifi|mqtt|api|host|topic/i);
    }

    expect(hardwareSettings({
        irRx: '4', irRxPreset: '1', irCmdFF906F: 'relay 0 on', irRelayOn0: 'A', irRxMqtt: '1',
        rfbProv: 'efm8bb1', rfbON0: 'A1B2', rfbRepeat: '3',
        dhtPin: '2', snsReport: '1', tmpUnits: '1', pwrVoltage: '230', eneTotal0: '100',
        i2cSDA: '4', i2cSCL: '5', btnLevel1: '220',
    })).toEqual({
        irRx: '4', irRxPreset: '1', rfbProv: 'efm8bb1', dhtPin: '2', i2cSDA: '4', i2cSCL: '5', btnLevel1: '220',
    });
});

test('hardware keys are allow-listed by name, not by prefix', () => {
    expect(hardwareSettings({
        relayGpio3: '5', relayTopicPub0: 'x', relayPayloadOn: 'ON', relayBoot0: '1',
        btnMqttRetain: '1', ledPattern0: 'x', uartTx0: '153', rfbON0: 'A1B2', rfbRX: '26',
        ltDimmerGPIO0: '4', sysLoopStall: '1', btnGpio0: {nested: true},
    })).toEqual({relayGpio3: '5', uartTx0: '153', rfbRX: '26', ltDimmerGPIO0: '4'});
});
