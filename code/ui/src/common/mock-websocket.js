//import 'setimmediate';
import {WebSocket, Server} from "mock-websocket";

export default function () {
    const server = new Server("ws://localhost:8080");

    server.on('connection', () => {
        let data = [{
            "device": {
                "_heap": 18536,
                "_uptime": 291842,
                "_load_average": 1,
                "_vcc": 3058,
                "_now": Math.floor(Date.now() / 1000)
            },
            "wifi": {
                "_rssi": -84
            }
        },
            {
                "_modules": {
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
                "_weblog": ["[WEBSOCKET] #3 connected, ip: 192.168.1.5, url: /ws\n"],
            }, {
                "wifi": {
                    "_rssi": -86,
                    "_max": 5,
                    "scan": true,
                    "_schema": [
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
                "_version": {
                    "app_name": "ESPURNA",
                    "app_version": "1.13.6-dev",
                    "app_build": "2019-10-05 17:29:31",
                    "sketch_size": 494096,
                    "sdk": "1.5.3(aec24ac9)",
                    "core": "2.3.0",
                },
                "device": {
                    "_name": "SONOFF_SV",
                    "_manufacturer": "ITEAD",
                    "_chip_id": "209458",
                    "hostname": "ESPURNA",
                    "desc": "An espurna enabled device",
                    "_free_size": 532480,
                    "_total_size": 1032480,
                    "webPort": 80,
                    "wsAuth": true,
                    "hbMode": 2,
                    "hbInterval": 300
                },
                "wifi": {
                    "_name": "TestWifi",
                    "_mac": "AA:BB:CC:DD:EE:FF",
                    "_bssid": "00:11:22:33:44:55",
                    "_channel": 11,
                    "_ip": "192.168.1.3",
                },
                "btnDelay": 500, //This should be by relay
            },
            {
                "api": {"enabled": false, "key": "123456789ABCDEF", "realTime": false, "restFul": true}
            }, {
                "relay": {
                    "config": {
                        "_start": 0,
                        "_schema": [
                            "GPIO",
                            "type",
                            //"resetGPIO",
                            "boot",
                            "pulse",
                            "time",
                            "group",
                            "groupSync",
                            "onDisc",
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
                                //153,
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
                                //153,
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
                                //153,
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
                                //153,
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
                "relay": {
                    "state": {
                        "_start": 0,
                        "_schema": [
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
                    "_max": 10,
                    "_schema": [
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
                    "_schema": [
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
            },
            {
                "_loaded": true
            }
        ];

        let i = 50;
        data.forEach((msg) => {
            //server.send(msg);
            setTimeout(() => {
                server.send(JSON.stringify(msg));
            }, i);
            i += 50;
        });

        setTimeout(() => {
            server.send(JSON.stringify({
                "_weblog": [
                    "[NTP] UTC Time  : 2020-02-13 17:49:33\n",
                    "[NTP] Local Time: 2020-02-13 18:49:33\n"
                ],
            }));
        }, i + 4000);
    });


    server.on('message', (con, msg) => {
        console.log(con, msg);
        try {
            msg = JSON.parse(msg);

            if (msg.id) {
                setTimeout(() => {
                    //Send success for cb
                    server.send(JSON.stringify({id: msg.id}));
                }, 100);
            }
        } catch (e) {

        }
    });

    return new WebSocket("ws://localhost:8080");
}
