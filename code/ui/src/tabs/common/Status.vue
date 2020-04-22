<template>
    <section v-loading="!device || !wifi" class="status">
        <div class="header">
            <h1>STATUS</h1>
            <h2>Current configuration</h2>
        </div>

        <div v-if="device && wifi" class="page">
            <fieldset>
                <Repeater v-if="relayState" v-model="relayState.list" class="relays" locked>
                    <template #default="tpl">
                        <Row>
                            <C :size="2"><label>Switch #{{tpl.k}}</label></C>
                            <C :size="8">
                                <Inpt type="switch" :value="tpl.value.status"
                                      on="ON"
                                      off="OFF"
                                      :disabled="tpl.value.lock < 2"
                                      @input="(v) => toggleRelay(tpl, v)"/>
                            </C>
                        </Row>
                    </template>
                </Repeater>

                <!-- #!if LIGHT === true -->
                <template v-if="modules.light && light">
                    <Row>
                        <C><label>Color</label></C>
                        <C><span>{{light.color}}</span></C>
                    </Row>

                    <Row>
                        <C><label>Mireds (Cold &harr; Warm)</label></C>
                        <C>
                            <Inpt type="range" name="mireds" min="153" max="500" :value="light.mired"/>
                            {{light.mired}}
                        </C>
                    </Row>

                    <Row>
                        <C><label>Brightness</label></C>
                        <C>
                            <input type="range" min="0" max="255"
                                   :value="light.brightness"
                                   readonly> {{light.brightness}}
                        </C>
                    </Row>

                    <Row v-for="(channel, id) in light.channels" :key="id">
                        <C><label>Channel #{{id}}</label></C>
                        <C>
                            <input type="range" min="0" max="255"
                                   :value="channel"
                                   readonly>
                        </C>
                    </Row>
                </template>
                <!-- #!endif -->

                <!-- #!if SENSOR === true -->
                <template v-if="modules.sns && sns && sns.magnitudes">
                    <Row v-for="(magnitude, i) in sns.magnitudes.list" :key="magnitude.index">
                        <C><label>{{magnitudeType(magnitude.type)}} #{{magnitude.index}},</label></C>
                        <C>
                            <Inpt class="center" type="text" name="magnitude"
                                  :value="i"/>
                            <Hint>{{magnitude.description}}</Hint>
                        </C>
                    </Row>
                </template>
                <!-- #!endif -->

                <!-- #!if RFM69 === true -->
                <Row v-if="modules.rfm69 && rfm69" class="state responsive">
                    <C>
                        <Row>
                            <C><label>Packet count</label></C>
                            <C>{{rfm69.packetCount}}</C>
                        </Row>
                    </C>
                    <C>
                        <Row>
                            <C><label>Node count</label></C>
                            <C>{{rfm69.nodeCount}}</C>
                        </Row>
                    </C>
                </Row>
                <!-- #!endif -->

                <Row class="state responsive">
                    <C>
                        <Row>
                            <C><label>Manufacturer</label></C>
                            <C>{{device.manufacturer}}</C>

                            <C><label>Device</label></C>
                            <C>{{device.name}}</C>

                            <C>Chip ID</C>
                            <C>{{device.chipId}}</C>

                            <C>SDK version</C>
                            <C>{{version.sdk}}</C>

                            <C>Core version</C>
                            <C>{{version.core}}</C>

                            <C>Firmware name</C>
                            <C>{{version.appName}}</C>

                            <C>Firmware version</C>
                            <C>{{version.appVersion}}</C>

                            <template v-if="version.appRevision">
                                <C>Firmware revision</C>
                                <C>{{version.appRevision}}</C>
                            </template>

                            <C>Firmware build date</C>
                            <C>{{version.appBuild}}</C>

                            <C>Firmware size</C>
                            <C>{{version.sketchSize}}</C>

                            <C>Free space</C>
                            <C>{{device.freeSize}} bytes</C>

                            <C>Free heap</C>
                            <C>{{device.heapFree}} bytes</C>

                            <C>Initial heap</C>
                            <C>{{device.heapTotal}} bytes</C>

                            <C>Usable heap</C>
                            <C>{{device.heapUsable}} bytes</C>

                            <C>Heap fragmentation</C>
                            <C>{{device.heapFrag}} %</C>

                            <C>Load average</C>
                            <C>{{device.loadAverage}}%</C>

                            <C>VCC</C>
                            <C>{{device.vcc}} mV</C>
                        </Row>
                    </C>

                    <C>
                        <Row>
                            <C>Network</C>
                            <C>{{wifi.name}}</C>

                            <C>Wifi MAC</C>
                            <C>{{wifi.mac}}</C>

                            <C>BSSID</C>
                            <C>{{wifi.bssid}}</C>

                            <C>Channel</C>
                            <C>{{wifi.channel}}</C>

                            <C>RSSI</C>
                            <C>{{wifi.rssi}}</C>

                            <C>IP</C>
                            <C>
                                <a :href="'//'+wifi.ip">{{wifi.ip}}</a> (<a :href="'telnet://'+wifi.ip">telnet</a>)
                            </C>

                            <template v-if="modules.mqtt && mqtt">
                                <C>MQTT Status</C>
                                <C>{{mqtt.status ? 'CONNECTED' : 'NOT CONNECTED'}}</C>
                            </template>

                            <template v-if="modules.ntp && ntp">
                                <C>NTP Status</C>
                                <C>{{ntp.status ? 'SYNCED' : 'NOT SYNCED'}}</C>
                                <C>Current time</C>
                                <C>{{date(now)}}</C>
                            </template>

                            <C>Uptime</C>
                            <C>{{elapsed(uptime)}}</C>

                            <C>Last update</C>
                            <C>{{lastUpdateSeconds}} seconds ago</C>
                        </Row>
                    </C>
                </Row>
            </fieldset>
        </div>
        <div v-if="modules.cmd || modules.dbg" class="page">
            <div class="header">
                <h1>
                    <span v-if="modules.dbg">DEBUG LOG</span>
                    <span v-if="modules.dbg && modules.cmd"> / </span>
                    <span v-if="modules.cmd">TERMINAL</span>
                </h1>
                <h2>
                    <span v-if="modules.dbg && modules.cmd">Shows debug messages and send commands to the device</span>
                    <span v-else-if="modules.dbg">Shows debug messages from the device</span>
                    <span v-else>Send commands to the device</span>
                </h2>
            </div>
            <fieldset>
                <Row v-if="modules.cmd" class="responsive">
                    <C :size="2"><label>Command</label></C>
                    <C :size="8" no-wrap>
                        <Inpt v-model="dbgcmd" type="text" tabindex="2"/>
                        <Btn name="dbgcmd" @click="sendCmd">
                            Send
                        </Btn>
                        <Hint>
                            Write a command and click send to execute it on the device. The output will be shown in the
                            debug text area below.
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <textarea :value="logs.join('')"
                              class="terminal"
                              wrap="soft"
                              readonly spellcheck="false">
                    </textarea>
                </Row>
                <Row>
                    <Btn name="dbg-clear" color="danger" @click="logs = []">
                        Clear
                    </Btn>
                </Row>
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Btn from "./../../components/Button";
    import Row from "../../layout/Row";
    import C from "../../layout/Col";
    import Hint from "../../components/Hint";
    import Repeater from "../../components/Repeater";
    import ws from "./../../common/websocket";

    export default {
        components: {
            Repeater,
            Hint,
            C,
            Row,
            Inpt,
            Btn
        },
        inheritAttrs: false,
        props: {
            lastUpdate: Number,
            modules: Object,
            version: Object,
            device: Object,
            relay: Object,
            relayState: Object,
            sns: Object,
            wifi: Object,
            light: Object,
            rfm69: Object,
            mqtt: Object,
            ntp: Object,
            weblog: Array
        },
        data() {
            return {
                logs: [],
                dbgcmd: "",
                datenow: Math.floor(Date.now() / 1000) + 1,
                interval: null
            };
        },
        computed: {
            lastUpdateSeconds() {
                return this.datenow - Math.floor(this.lastUpdate / 1000);
            },
            now() {
                return this.lastUpdateSeconds + this.device.now;
            },
            uptime() {
                return this.lastUpdateSeconds + this.device.uptime;
            },
        },
        watch: {
            lastUpdate(rec, old) {
                const incr = Math.floor((rec - old) / 1000);
                this.device.now += incr;
                this.device.uptime += incr;
            },
            weblog: {
                immediate: true,
                handler(rec, old) {
                    if (this.weblog && (typeof old === "undefined" || JSON.stringify(old) !== JSON.stringify(rec))) {
                        const date = "[" + this.date() + "] ";
                        this.weblog.forEach((v) => {
                            this.logs.push(date + v);
                        });
                    }
                }
            }
        },
        mounted() {
            this.interval = setInterval(() => {
                this.datenow++;
            }, 1000);
        },
        beforeDestroy() {
            clearInterval(this.interval);
        },
        methods: {
            date(d) {
                return (d ? new Date(d * 1000) : new Date()).toLocaleString(navigator.languages);
            },
            elapsed(d) {
                try {
                    return parseInt(d / 86400) + "d " + new Date(d * 1000).toISOString().replace(/.*(\d{2}):(\d{2}):(\d{2}).*/, "$1h $2m $3s");
                } catch (e) {
                    return "";
                }
            },
            toggleRelay(tpl, val) {
                if (tpl.value.lock >= 2) {
                    ws.send({action: "relay", data: {id: tpl.k, status: val ? 1 : 0}}, (res) => {
                        if (res.success) {
                            this.$set(tpl.value, "status", val ? 1 : 0);
                        }
                    });
                }
            },
            sendCmd() {
                const dbgcmd = this.dbgcmd;
                if (dbgcmd) {
                    this.logs.push("[" + this.date() + "] > " + dbgcmd + "\n");

                    ws.send({action: "dbgcmd", data: {command: dbgcmd}}, (res) => {
                        if (res.success) {
                            this.dbgcmd = "";
                        } else {
                            this.logs.push("[" + this.date() + "] < Command: `" + dbgcmd + "` failed\n");
                        }
                    });
                }
            },
            magnitudeType(type) {
                const types = [
                    "Temperature", "Humidity", "Pressure",
                    "Current", "Voltage", "Active Power", "Apparent Power",
                    "Reactive Power", "Power Factor", "Energy", "Energy (delta)",
                    "Analog", "Digital", "Event",
                    "PM1.0", "PM2.5", "PM10", "CO2", "Lux", "UVA", "UVB", "UV Index", "Distance", "HCHO",
                    "Local Dose Rate", "Local Dose Rate",
                    "Count", "NO2", "CO", "Resistance", "pH"
                ];
                if (1 <= type && type <= types.length) {
                    return types[type - 1];
                }
                return null;
            }
        }
    };
</script>

<style>
    .state {
        border-top: 1px solid #eee;
        margin-top: 20px;
        padding-top: 30px;
        font-size: 80%;
    }


    .state .row .col:nth-child(odd) {
        text-align: right;
    }

    .state .row .col:nth-child(even) {
        font-weight: bold;
        text-align: left;
    }

    .relays .col > label {
        font-size: 1.3em;
        float: right;
        margin: .4em 15px;
    }

    /* -----------------------------------------------------------------------------
        Logs
       -------------------------------------------------------------------------- */

    #weblog {
        height: 400px;
        margin-bottom: 10px;
    }
</style>
