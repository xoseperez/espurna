//import 'setimmediate';
import {WebSocket, Server} from "mock-websocket";

export default function () {
    const server = new Server("ws://localhost:8080");

    server.on('connection', () => {
        let data = [{
            "device": {
                "heap": 18536,
                "uptime": 291842,
                "loadaverage": 1,
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
                "weblog": {
                    "msg": ["[WEBSOCKET] #3 connected, ip: 192.168.1.5, url: /ws\n"],
                    "pre": ["[842843] "]
                }
            }, {
                "wifi": {
                    "rssi": -86,
                    "maxNetworks": 5,
                    "scan": true,
                    "schema": [
                        "ssid",
                        "pass",
                        "ip",
                        "gw",
                        "mask",
                        "dns",
                    ],
                    "list": [
                        [
                            "TestWifi",
                            "testpassword",
                            "192.168.1.3",
                            "192.168.1.1",
                            "255.255.255.0",
                            ""
                        ]
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
                            0
                        ]
                    ],
                }
            }
        ];

        data.forEach((msg) => {
            server.send(msg);
            //server.send(JSON.stringify(msg));
        })
    });


    server.on('message', (con, msg) => {
        console.log(con, msg);
    });

    return new WebSocket("ws://localhost:8080");
}