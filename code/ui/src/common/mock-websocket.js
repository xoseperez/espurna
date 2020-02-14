//import 'setimmediate';
import {WebSocket, Server} from "mock-websocket";

export default function () {
    const server = new Server("ws://localhost:8080");

    server.on('connection', () => {
        let data = [{
            "device": {
                "heap": 18536,
                "uptime": 291842,
                "load_average": 1,
                "vcc": 3058,
                "now": 1571866442
            },
            "wifi": {
                "rssi": -84
            }
        },
            {
                "modules": {
                    "cmd": 1,
                    "telnet": 1,
                    "dbg": 1,
                    "api": 1,
                    "relay": 1,
                    "led": 1,
                    "mqtt": 1,
                    "ntp": 1,
                    "alexa": 1,
                    "sns": 1,
                    "tspk": 1,
                    "dcz": 1,
                    "ha": 1,
                    "sch": 1
                }
            },
            {
                "weblog": ["[WEBSOCKET] #3 connected, ip: 192.168.1.5, url: /ws\n"],
            }, {
                "wifi": {
                    "rssi": -86,
                    "max": 5,
                    "scan": true,
                    "schema": [
                        "ssid",
                        "pass",
                        "ip",
                        "gw",
                        "mask",
                        "dns",
                        "hardcoded"
                    ],
                    "list": [
                        [
                            "TestWifi",
                            "testpassword",
                            "192.168.1.3",
                            "192.168.1.1",
                            "255.255.255.0",
                            "",
                            0
                        ],
                        [
                            "DefinedWifi",
                            "testpassword",
                            "192.168.1.3",
                            "192.168.1.1",
                            "255.255.255.0",
                            "8.8.8.8",
                            1
                        ],
                        [
                            "TestWifi3",
                            "testpassword",
                            "192.168.1.3",
                            "192.168.1.1",
                            "255.255.255.0",
                            "",
                            0
                        ],
                    ]
                }
            }, {
                "telnet": {
                    "STA": false,
                    "auth": true
                }
            }, {
                "webMode": 0,
                "version": {
                    "app_name": "ESPURNA",
                    "app_version": "1.13.6-dev",
                    "app_build": "2019-10-05 17:29:31",
                    "sketch_size": 494096,
                    "sdk": "1.5.3(aec24ac9)",
                    "core": "2.3.0",
                },
                "device": {
                    "name": "SONOFF_SV",
                    "manufacturer": "ITEAD",
                    "chip_id": "209458",
                    "hostname": "ESPURNA",
                    "desc": "An espurna enabled device",
                    "free_size": 532480,
                    "webPort": 80,
                    "wsAuth": true,
                    "hbMode": 2,
                    "hbInterval": 300
                },
                "wifi": {
                    "name": "TestWifi",
                    "mac": "AA:BB:CC:DD:EE:FF",
                    "bssid": "00:11:22:33:44:55",
                    "channel": 11,
                    "ip": "192.168.1.3",
                },
                "btnDelay": 500, //This should be by relay
            },
            {
                "api": {"enabled": false, "key": "123456789ABCDEF", "realTime": false, "restFul": true}
            }, {
                "relays": {
                    "config": {
                        "start": 0,
                        "schema": [
                            "gpio",
                            "type",
                            "reset",
                            "boot",
                            "pulse",
                            "pulse_time",
                            "group",
                            "group_sync",
                            "on_disc",
                            "dblDl",
                            "lngDl",
                            "lnglngDl",
                            "sndAllEvts",
                            "lastSch"
                        ],
                        "list": [
                            [
                                "GPIO12",
                                0,
                                153,
                                0,
                                1,
                                1,
                                "",
                                0,
                                0,
                                500,
                                1000,
                                10000,
                                1,
                                1
                            ],
                            [
                                "GPIO13",
                                0,
                                153,
                                0,
                                1,
                                1,
                                "",
                                0,
                                0,
                                500,
                                1000,
                                10000,
                                1,
                                0
                            ],
                            [
                                "GPIO14",
                                0,
                                153,
                                0,
                                1,
                                1,
                                "",
                                0,
                                0,
                                1000,
                                2000,
                                10000,
                                0,
                                0
                            ],
                            [
                                "GPIO15",
                                0,
                                153,
                                0,
                                1,
                                1,
                                "",
                                0,
                                0,
                                100,
                                2000,
                                5000,
                                0,
                                0
                            ]
                        ],
                    }
                }
            },
            {
                "relays": {
                    "state": {
                        "start": 0,
                        "schema": [
                            "status",
                            "lock"
                        ],
                        "list": [
                            [0, 2],
                            [1, 2],
                            [0, 1],
                            [1, 1],
                        ]
                    }
                }
            },
            {
                "schedule": {
                    "max": 10,
                    "schema": [
                        "enabled",
                        "UTC",
                        "switch",
                        "action",
                        "type",
                        "hour",
                        "minute",
                        "weekdays"
                    ],
                    "list": []
                }
            },
            {
                "led": {
                    "schema": [
                        "mode",
                        "relay"
                    ],
                    "list": [
                        [3, 1],
                        [9, 0],
                        [7, 0]
                    ]
                }
            },
            {
                "ha": {
                    "prefix": "homeassistant",
                    "enabled": false
                }
            }
        ];

        data.forEach((msg) => {
            //server.send(msg);
            server.send(JSON.stringify(msg));
        });

        setTimeout(() => {
            server.send(JSON.stringify({
                "weblog": [
                    "[NTP] UTC Time  : 2020-02-13 17:49:33\n",
                    "[NTP] Local Time: 2020-02-13 18:49:33\n"
                ],
            }));
        }, 2000);
    });


    server.on('message', (con, msg) => {
        console.log(con, msg);
    });

    return new WebSocket("ws://localhost:8080");
}
