/** @import { SendEvent } from './connection.mjs' */

import { updateVariables } from './settings.mjs';

export function init() {
    updateVariables({
        schTypes: [
            [1, "disabled"],
            [2, "calendar"],
            [3, "relative"],
        ],
        ledModes: [
            [1, "wifi"],
            [2, "relay"],
            [3, "relay-inverse"],
            [4, "findme"],
            [5, "findme-wifi"],
            [6, "on"],
            [7, "off"],
            [8, "relays"],
            [9, "relays-wifi"],
            [0, "manual"],
        ],
    });
    updateVariables({
        ntpServer: "192.168.1.1",
        ntpTZ: 'UTC-2',
        ntpStartDelay: 10,
        ntpDhcp: true,
        ntpUpdateIntvl: 1800,
        webMode: 0,
        webPort: 80,
        useWhite: false,
        useCCT: false,
        useColor: true,
        useRGB: true,
        now: "2024-01-01T00:00:00+01:00",
        light: {
            channels: ['r', 'g', 'b'],
        },
        gpioConfig: {
            types: [['hardware', 1]],
            hardware: [
                0,
                1,
                2,
                3,
                4,
                5,
                9,
                10,
                12,
                13,
                14,
                15,
                16,
            ],
        },
        curtainType: 0,
        curtainBoot: 0,
        relayConfig: {
            size: 5,
            start: 0,
            schema: [
                'relayName',
                'relayProv',
                'relayType',
                'relayGpioType',
                'relayGpio',
                'relayResetGpio',
                'relayBoot',
                'relayDelayOn',
                'relayDelayOff',
                'relayTime',
                'relayBtnGpio',
                'relayPulse',
                'relayTopicPub',
                'relayTopicSub',
                'relayTopicMode',
                'relayMqttDisc',
            ],
            values: [
                ['', 'gpio', 0, 1, 4, 153, 0, 0, 0, 0, 153, 0, '', '', 0, 0],
                ['222', 'gpio', 0, 1, 5, 153, 0, 0, 0, 0, 153, 0, '', '', 0, 0],
                ['', 'gpio', 0, 1, 12, 153, 0, 0, 0, 0, 153, 0, '', '', 0, 0],
                ['444', 'gpio', 0, 1, 13, 153, 0, 0, 0, 0, 153, 0, '', '', 0, 0],
                ['foo', 'gpio', 0, 1, 14, 153, 0, 0, 0, 0, 153, 0, '', '', 0, 0],
            ],
        },
        ledConfig: {
            schema: [
                'ledGpio',
                'ledInv',
                'ledMode',
                'ledRelay',
            ],
            leds: [
                [15, 0, 3, 0],
                [16, 0, 2, 1],
            ],
        },
        rfm69Topic: 'foo',
        rfm69: {
            packets: 123,
            nodes: 456,
            schema: [
                'rfm69Node',
                'rfm69Key',
                'rfm69Topic',
            ],
            mapping: [
                [1, 'one', 'two'],
                [2, 'three', 'four'],
                [3, 'five', 'six'],
                [4, 'seven', 'eight'],
                [5, 'nine', 'ten'],
            ],
        },
        rfbRepeat: 25,
        rfbCount: 5,
        rfbRX: 1,
        rfbTX: 3,
    });
    updateVariables({
        hostname: 'localhost',
        desc: `${new URL(import.meta.url)}`,
        ssid: 'ESPURNA-12345',
        bssid: '11:22:33:aa:ee',
        channel: 13,
        rssi: -54,
        staip: '10.10.10.5',
        apip: '192.168.4.1',
        app_name: 'ESPurna',
        app_version: '9.9.9-dev',
        app_build: '9999-12-12 23:59:59',
        sketch_size: '555555',
        free_size: 493021,
        uptime: '5d 30m 13s',
        manufacturer: 'WEB',
        device: navigator.userAgent,
        chipid: window.name,
        sdk: 'WEB',
        core: 'WEB',
        heap: 999999,
        loadaverage: 99,
        vcc: '3.3',
        mqttStatus: true,
        ntpStatus: true,
        dczRelays: [
            0,
            0,
            0,
            0,
            0,
        ],
        tspkRelays: [
            0,
            0,
            0,
            0,
            0,
        ],
        light: {
            rgb: "#aaffaa",
            brightness: 100,
            state: true,
        },
        curtainState: {
            get: 20,
            set: 50,
            button: 0,
            moving: 0,
            type: 2,
        },
        rfb: {
            start: 0,
            codes: [
                ['aaaaaa', 'bbbbbb'],
                ['cccccc', 'dddddd'],
                ['ffffff', 'ffffff'],
                ['333333', '000000'],
                ['222222', '111111'],
            ],
        },
        rpnRules: [
            '2 2 +',
            '5 5 *',
            'yield',
            'rssi not',
            '$value $value + "foo/bar" mqtt.send'
        ],
        rpnTopics: {
            schema: [
                'rpnName',
                'rpnTopic',
            ],
            topics: [
                ['value', 'hello/world'],
                ['other', 'world/hello'],
            ],
        },
        schConfig: {
            schedules: [
                [2, 0, 'relay 0 1', '05,10:00'],
                [2, 0, 'relay 1 2', '05:00'],
                [1, 0, 'relay 2 0\nrelay 0 0', '10:00'],
                [3, 0, 'mqtt.send "foo" "bar"', '15m after cal#2'],
            ],
            schema: [
                'schType',
                'schRestore',
                'schAction',
                'schTime',
            ],
            max: 4,
        },
        'magnitudes-init': {
            types: {
                schema: [
                    'type',
                    'prefix',
                    'name',
                    'units',
                ],
                values: [
                    [1, 'tmp', 'Temperature', [2,3,4]],
                    [2, 'hum', 'Humidity', [5]],
                    [3, 'press', 'Pressure', [6]],
                    [4, 'curr', 'Current', [7]],
                    [5, 'volt', 'Voltage', [8]],
                    [6, 'pwrP', 'Active Power', [13]],
                    [10, 'ene', 'Energy', [15,16]],
                    [11, 'eneDelta', 'Energy (delta)', [15,16]],
                ],
            },
            errors: {
                schema: [
                    'type',
                    'name',
                ],
                values: [
                    [0, 'OK'],
                    [1, 'FAIL'],
                    [99, 'OUT_OF_RANGE'],
                ],
            },
            units: {
                schema: [
                    'type',
                    'name',
                ],
                values: [
                    [2, '°C'],
                    [3, '°F'],
                    [4, 'K'],
                    [5, '%'],
                    [6, 'hPa'],
                    [7, 'A'],
                    [8, 'V'],
                    [13, 'W'],
                    [15, 'J'],
                    [16, 'kWh'],
                ],
            },
        },
        snsRealTime: false,
        snsRead: 6,
        snsInit: 10,
        snsReport: 10,
        snsSave: 0,
        thermostatMode: true,
        thermostatTmpUnits: 'C',
        thermostatOperationMode: 'local window',
        remoteTmp: '23',
        wifiConfig: {
            schema: [
                'ssid',
                'pass',
                'ip',
                'gw',
                'mask',
                'dns',
                'bssid',
                'chan',
            ],
            networks: [
                ['ESPURNA-12345', 'fibonacci', '', '', '', '', '', ''],
            ],
            max: 5,
        },
    });

    for (let module of ["dcz", "tspk"]) {
        updateVariables({
            'magnitudes-module': {
                prefix: module,
                values: [
                    [1, 0, 0],
                    [1, 1, 0],
                    [2, 0, 0],
                    [3, 0, 0],
                    [5, 0, 0],
                    [4, 0, 0],
                    [6, 0, 0],
                    [10, 0, 0],
                    [11, 0, 0],
                ],
                schema: [
                    "type",
                    "index_global",
                    "index_module",
                ]
            },
        });
    }

    updateVariables({
        'magnitudes-list': {
            schema: [
                'type',
                'index_global',
                'description',
                'units',
            ],
            values: [
                [1, 0, "Foo measurements", 2],
                [1, 1, "Foo measurements", 2],
                [2, 0, "Bar simulation", 5],
                [3, 0, "Baz calculation", 6],
                [5, 0, "Powermeter", 8],
                [4, 0, "Powermeter", 7],
                [6, 0, "Powermeter", 13],
                [10, 0, "Powermeter", 16],
                [11, 0, "Powermeter", 15],
            ],
        },
    });

    updateVariables({
        'magnitudes-settings': {
            values: [
                [0,null,1,"NaN","NaN",0,0],
                [0,null,"NaN",2,"NaN",0,0],
                [0,null,"NaN","NaN",3,0,0],
                [0,null,"NaN","NaN","NaN",0,0],
                [0,1,"NaN","NaN",4,0,0],
                [0,1,"NaN",5,"NaN",0,0],
                [0,1,6,"NaN","NaN",0,0],
                [null,1,"NaN","NaN","NaN",0,0],
                [null,null,1,2,3,0,0],
            ],
            schema: [
                "Correction",
                "Ratio",
                "MinThreshold",
                "MaxThreshold",
                "ZeroThreshold",
                "MinDelta",
                "MaxDelta"
            ]
        },
    });

    updateVariables({
        magnitudes: {
            schema: [
                'value',
                'units',
                'error',
            ],
            values: [
                ["28", 2, 0],
                ["23", 2, 0],
                ["98", 5, 0],
                ["296", 6, 99],
                ["240", 8, 0],
                ["2.4", 7, 0],
                ["576", 13, 0],
                ["12345", 16, 0],
                ["111", 15, 0],
            ],
        },
        relayState: {
            schema: [
                'status',
                'lock',
            ],
            values: [
                [0, 0],
                [1, 2],
                [0, 1],
                [1, 0],
                [1, 0],
            ],
        },
    });

    for (let n = 100; n < 200; n += 15) {
        updateVariables({
            scanResult: [
                'aa:bb:cc:ee:ff',
                `WPA${n / 100}PSK`,
                -Math.ceil(100 * Math.random()),
                Math.ceil(Math.random() * 13),
                `FooBar${n / 50}`,
            ],
            rfm69: {
                message: [
                    n,
                    456,
                    789,
                    'one',
                    'eno',
                    -127,
                    0,
                    0,
                ],
            },
        });
    }

    /** @type {number | NodeJS.Timeout | null} */
    let random_messages = setInterval(() => {
        const rnd = () =>
            Math.ceil(Math.random() * 100000)
            .toString()
            .padStart(6, '0');

        const msg = [...Array(10)].map(() => rnd());

        updateVariables({
            log: [`[${rnd()}] ${msg}\n`],
        });
    }, 1000);

    window.addEventListener("app-send", (event) => {
        const data = /** @type {SendEvent} */(event).detail.data;
        if (data === "{}") {
            return;
        }

        if (random_messages !== null) {
            clearInterval(random_messages);
            random_messages = null;
        }

        updateVariables({
            "log": [`${data}\n`],
        });
    });
}
