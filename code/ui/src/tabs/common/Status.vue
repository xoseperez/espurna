<template>
    <section>
        <div class="header">
            <h1>STATUS</h1>
            <h2>Current configuration</h2>
        </div>

        <div class="page">
            <form class="pure-form pure-form-aligned">
                <fieldset>
                    <div v-for="(val, relay) in relays" :key="relay" class="pure-g">
                        <label class="pure-u-1 pure-u-lg-1-4">Switch #{{relay}}</label>
                        <div>
                            <Inpt :value="val" type="switch" name="relay"
                                  on="ON"
                                  off="OFF"
                                  @input="() => {}"/>
                        </div>
                    </div>

                    <!-- #if process.env.VUE_APP_LIGHT === 'true' -->
                    <div class="pure-g color">
                        <label class="pure-u-1 pure-u-lg-1-4">Color</label>
                        <span class="pure-u-lg-1-4">{{light.color}}</span>
                    </div>
                    <div class="pure-g mireds">
                        <label class="pure-u-1 pure-u-lg-1-4">Mireds (Cold &harr; Warm)</label>
                        <input type="range" min="153" max="500"
                               class="slider pure-u-lg-1-4" :value="light.mired"
                               readonly>
                        <span class="slider pure-u-lg-1-4">{{light.mired}}</span>
                    </div>
                    <div class="pure-g">
                        <label class="pure-u-1 pure-u-lg-1-4">Brightness</label>
                        <input type="range" min="0" max="255"
                               class="slider pure-u-lg-1-4" :value="light.brightness"
                               readonly>
                        <span class="slider brightness pure-u-lg-1-4">{{light.brightness}}</span>
                    </div>

                    <div v-for="(channel, id) in light.channels" :key="id" class="pure-g">
                        <label class="pure-u-1 pure-u-lg-1-4">Channel #{{id}}</label>
                        <input type="range" min="0" max="255"
                               :value="channel" class="slider channels pure-u-lg-1-4"
                               readonly>
                        <span class="slider pure-u-lg-1-4"></span>
                    </div>
                    <!-- #endif -->

                    <!-- #if process.env.VUE_APP_SENSOR === 'true' -->
                    <div id="magnitudes">TODO</div>

                    <div id="magnitudeTemplate" class="template">
                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4"></label>
                            <div class="pure-u-1 pure-u-lg-1-4">
                                <Inpt class="pure-u-1 pure-u-lg-23-24 center" type="text" name="magnitude"
                                      data="256"
                                      readonly/>
                            </div>
                            <div class="pure-u-1 pure-u-lg-1-2 hint center"></div>
                        </div>
                    </div>
                    <!-- #endif -->

                    <!-- #if process.env.VUE_APP_RFM69 === 'true' -->
                    <div class="pure-g module module-rfm69">
                        <div class="pure-u-1-2">Packet count</div>
                        <div class="pure-u-11-24"><span class="right">{{rfm69.packet_count}}</span></div>
                    </div>

                    <div class="pure-g module module-rfm69">
                        <div class="pure-u-1-2">Node count</div>
                        <div class="pure-u-11-24"><span class="right">{{rfm69.node_count}}</span></div>
                    </div>
                    <!-- #endif -->

                    <div class="pure-u-1 pure-u-lg-1-2 state">
                        <div class="pure-u-1-2">Manufacturer</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{device.manufacturer}}</span>
                        </div>

                        <div class="pure-u-1-2">Device</div>
                        <div class="pure-u-11-24"><span class="right">{{device.name}}</span></div>

                        <div class="pure-u-1-2">Chip ID</div>
                        <div class="pure-u-11-24"><span class="right">{{device.chip_id}}</span></div>

                        <div class="pure-u-1-2">SDK version</div>
                        <div class="pure-u-11-24"><span class="right">{{version.sdk}}</span></div>

                        <div class="pure-u-1-2">Core version</div>
                        <div class="pure-u-11-24"><span class="right">{{version.core}}</span></div>

                        <div class="pure-u-1-2">Firmware name</div>
                        <div class="pure-u-11-24"><span class="right">{{version.app_name}}</span></div>

                        <div class="pure-u-1-2">Firmware version</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{version.app_version}}</span>
                        </div>

                        <div class="pure-u-1-2">Firmware revision</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{version.app_revision}}</span>
                        </div>

                        <div class="pure-u-1-2">Firmware build date</div>
                        <div class="pure-u-11-24"><span class="right">{{version.app_build}}</span></div>

                        <div class="pure-u-1-2">Firmware size</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{version.sketch_size}}</span>
                        </div>

                        <div class="pure-u-1-2">Free space</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{device.free_size}} bytes</span>
                        </div>
                        <div class="pure-u-1-2">Free heap</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{device.heap}} bytes</span>
                        </div>

                        <div class="pure-u-1-2">Load average</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{device.load_average}}%</span>
                        </div>

                        <div class="pure-u-1-2">VCC</div>
                        <div class="pure-u-11-24"><span class="right">{{device.vcc}} mV</span></div>
                    </div>

                    <div class="pure-u-1 pure-u-lg-11-24 state">
                        <div class="pure-u-1-2">Network</div>
                        <div class="pure-u-11-24"><span class="right">{{wifi.name}}</span></div>

                        <div class="pure-u-1-2">Wifi MAC</div>
                        <div class="pure-u-11-24"><span class="right">{{wifi.mac}}</span></div>

                        <div class="pure-u-1-2">BSSID</div>
                        <div class="pure-u-11-24"><span class="right">{{wifi.bssid}}</span></div>

                        <div class="pure-u-1-2">Channel</div>
                        <div class="pure-u-11-24"><span class="right">{{wifi.channel}}</span></div>

                        <div class="pure-u-1-2">RSSI</div>
                        <div class="pure-u-11-24"><span class="right">{{wifi.rssi}}</span></div>

                        <div class="pure-u-1-2">IP</div>
                        <div class="pure-u-11-24">
                            <a :href="'//'+wifi.ip" class="right">{{wifi.ip}}</a>
                            (<a :href="'telnet://'+wifi.ip" class="right">telnet</a>)
                        </div>

                        <div class="pure-u-1-2 module module-mqtt">MQTT Status</div>
                        <div class="pure-u-11-24 module module-mqtt">
                            <span class="right">{{mqtt.status ? 'CONNECTED' : 'NOT CONNECTED'}}</span>
                        </div>

                        <div class="pure-u-1-2 module module-ntp">NTP Status</div>
                        <div class="pure-u-11-24 module module-ntp">
                            <span class="right">{{ntp.status ? 'SYNCED' : 'NOT SYNCED'}}</span>
                        </div>

                        <div class="pure-u-1-2 module module-ntp">Current time</div>
                        <div class="pure-u-11-24 module module-ntp">
                            <span class="right">{{date(now)}}</span>
                        </div>

                        <div class="pure-u-1-2">Uptime</div>
                        <div class="pure-u-11-24"><span class="right">{{elapsed(uptime)}}</span></div>

                        <div class="pure-u-1-2">Last update</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{lastUpdate}}</span><span> seconds ago</span>
                        </div>
                    </div>
                </fieldset>
            </form>
            <fieldset>
                <legend>DEBUG LOG</legend>
                <h2>
                    Shows debug messages from the device
                </h2>

                <Row ref="cmd">
                    <C :size="12">
                        <div class="hint">
                            Write a command and click send to execute it on the device. The output will be shown
                            in the debug text area below.
                        </div>
                    </C>
                    <C>
                        <Inpt name="dbgcmd" type="text" tabindex="2"/>
                    </C>
                    <C>
                        <Btn name="dbgcmd">
                            Send
                        </Btn>
                    </C>
                </Row>

                <Row>
                    <textarea id="weblog"
                              class="terminal"
                              name="weblog"
                              wrap="soft"
                              readonly spellcheck="false"></textarea>
                    <Btn name="dbg-clear" color="danger">
                        Clear
                    </Btn>
                </Row>
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';
    import Btn from './../../components/Button'
    import Row from "../../layout/Row";
    import C from "../../layout/Col";

    export default {
        components: {
            C,
            Row,
            Inpt,
            Btn
        },
        props: {
            version: {
                type: Object,
                default() {
                    return {};
                }
            },
            device: {
                type: Object,
                default() {
                    return {};
                }
            },
            wifi: {
                type: Object,
                default() {
                    return {};
                }
            },
            now: {
                type: Number,
                default: 0
            },
            lastUpdate: {
                type: Number,
                default: 0
            },
            uptime: {
                type: Number,
                default: 0
            },
            light: {
                type: Object,
                default() {
                    return {channels: []}
                }
            },
            rfm69: {
                type: Object,
                default() {
                    return {}
                }
            },
            mqtt: {
                type: Object,
                default() {
                    return {}
                }
            },
            ntp: {
                type: Object,
                default() {
                    return {}
                }
            }
        },
        data() {
            return {}
        },
        methods: {
            date(d) {
                return new Date(d * 1000).toLocaleString(navigator.languages);
            },
            elapsed(d) {
                try {
                    return parseInt(d / 86400) + "d " + new Date(d * 1000).toISOString().replace(/.*(\d{2}):(\d{2}):(\d{2}).*/, "$1h $2m $3s");
                } catch (e) {
                    return ""
                }
            },
            toggleRelay(id, val) {
                sendAction("relay", {id: id, status: value ? 1 : 0});
                this.$set(relays, id, !val)
            }
        }
    }
</script>

<style>

    /* -----------------------------------------------------------------------------
        Logs
       -------------------------------------------------------------------------- */

    #weblog {
        height: 400px;
        margin-bottom: 10px;
    }
</style>