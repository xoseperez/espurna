<template>
    <section>
        <div class="header">
            <h1>STATUS</h1>
            <h2>Current configuration</h2>
        </div>

        <div class="page">
            <form class="pure-form pure-form-aligned">
                <fieldset>
                    <div id="relays">TODO</div>
                    <div id="relayTemplate" class="template">
                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4">Switch #<span class="id"></span></label>
                            <div>
                                <Inpt name="relay" type="checkbox" on="ON" off="OFF"/>
                            </div>
                        </div>
                    </div>

                    <!-- removeIf(!light) -->
                    <div id="colors">TODO</div>
                    <div id="colorTemplate" class="template">
                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4">Color</label>
                            <Inpt class="pure-u-1 pure-u-lg-1-4" data-wcp-layout="block" name="color" readonly/>
                        </div>
                    </div>

                    <div id="cct">TODO</div>
                    <div id="miredsTemplate" class="template">
                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4">Mireds (Cold &harr; Warm)</label>
                            <Inpt id="mireds" type="range" min="153" max="500" class="slider pure-u-lg-1-4">
                                <span class="slider mireds pure-u-lg-1-4"></span>
                            </inpt>
                        </div>
                    </div>

                    <div id="channels">TODO</div>
                    <div id="brightnessTemplate" class="template">
                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4">Brightness</label>
                            <Inpt id="brightness" type="range" min="0" max="255" class="slider pure-u-lg-1-4">
                                <span class="slider brightness pure-u-lg-1-4"></span>
                            </inpt>
                        </div>
                    </div>

                    <div id="channelTemplate" class="template">
                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4">Channel #</label>
                            <Inpt type="range" min="0" max="255" class="slider channels pure-u-lg-1-4" data="99">
                                <span class="slider pure-u-lg-1-4"></span>
                            </inpt>
                        </div>
                    </div>
                    <!-- endRemoveIf(!light) -->

                    <!-- removeIf(!sensor) -->
                    <div id="magnitudes">TODO</div>

                    <div id="magnitudeTemplate" class="template">
                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4"></label>
                            <div class="pure-u-1 pure-u-lg-1-4">
                                <Inpt class="pure-u-1 pure-u-lg-23-24 center" type="text" name="magnitude" data="256" readonly/>
                            </div>
                            <div class="pure-u-1 pure-u-lg-1-2 hint center"></div>
                        </div>
                    </div>
                    <!-- endRemoveIf(!sensor) -->

                    <!-- removeIf(!rfm69) -->
                    <div class="pure-g module module-rfm69">
                        <div class="pure-u-1-2">Packet count</div>
                        <div class="pure-u-11-24"><span class="right">{{status.packet_count}}</span></div>
                    </div>

                    <div class="pure-g module module-rfm69">
                        <div class="pure-u-1-2">Node count</div>
                        <div class="pure-u-11-24"><span class="right">{{status.node_count}}</span></div>
                    </div>
                    <!-- endRemoveIf(!rfm69) -->

                    <div class="pure-u-1 pure-u-lg-1-2 state">
                        <div class="pure-u-1-2">Manufacturer</div>
                        <div class="pure-u-11-24"><span class="right">{{status.manufacturer}}</span>
                        </div>

                        <div class="pure-u-1-2">Device</div>
                        <div class="pure-u-11-24"><span class="right">{{status.device}}</span></div>

                        <div class="pure-u-1-2">Chip ID</div>
                        <div class="pure-u-11-24"><span class="right">{{status.chip_id}}</span></div>

                        <div class="pure-u-1-2">Wifi MAC</div>
                        <div class="pure-u-11-24"><span class="right">{{status.mac}}</span></div>

                        <div class="pure-u-1-2">SDK version</div>
                        <div class="pure-u-11-24"><span class="right">{{version.sdk}}</span></div>

                        <div class="pure-u-1-2">Core version</div>
                        <div class="pure-u-11-24"><span class="right">{{version.core}}</span></div>

                        <div class="pure-u-1-2">Firmware name</div>
                        <div class="pure-u-11-24"><span class="right">{{version.app_name}}</span></div>

                        <div class="pure-u-1-2">Firmware version</div>
                        <div class="pure-u-11-24"><span class="right">{{version.app_version}}</span>
                        </div>

                        <div class="pure-u-1-2">Firmware revision</div>
                        <div class="pure-u-11-24"><span class="right">{{version.app_revision}}</span>
                        </div>

                        <div class="pure-u-1-2">Firmware build date</div>
                        <div class="pure-u-11-24"><span class="right">{{version.app_build}}</span></div>

                        <div class="pure-u-1-2">Firmware size</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{version.sketch_size}}</span>
                        </div>

                        <div class="pure-u-1-2">Free space</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{status.free_size}} bytes</span>
                        </div>
                    </div>

                    <div class="pure-u-1 pure-u-lg-11-24 state">
                        <div class="pure-u-1-2">Network</div>
                        <div class="pure-u-11-24"><span class="right">{{status.network}}</span></div>

                        <div class="pure-u-1-2">BSSID</div>
                        <div class="pure-u-11-24"><span class="right">{{status.bssid}}</span></div>

                        <div class="pure-u-1-2">Channel</div>
                        <div class="pure-u-11-24"><span class="right">{{status.channel}}</span></div>

                        <div class="pure-u-1-2">RSSI</div>
                        <div class="pure-u-11-24"><span class="right">{{status.rssi}}</span></div>

                        <div class="pure-u-1-2">IP</div>
                        <div class="pure-u-11-24">
                            <a :href="'//'+status.device_ip" class="right">{{status.device_ip}}</a>
                            (<a :href="'telnet://'+status.device_ip" class="right">telnet</a>)
                        </div>

                        <div class="pure-u-1-2">Free heap</div>
                        <div class="pure-u-11-24"><span class="right">{{status.heap}} bytes</span>
                        </div>

                        <div class="pure-u-1-2">Load average</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{status.load_average}}%</span>
                        </div>

                        <div class="pure-u-1-2">VCC</div>
                        <div class="pure-u-11-24"><span class="right">{{status.vcc}} mV</span></div>

                        <div class="pure-u-1-2 module module-mqtt">MQTT Status</div>
                        <div class="pure-u-11-24 module module-mqtt">
                            <span class="right">{{status.mqtt}}</span>
                        </div>

                        <div class="pure-u-1-2 module module-ntp">NTP Status</div>
                        <div class="pure-u-11-24 module module-ntp">
                            <span class="right">{{status.ntp}}</span>
                        </div>

                        <div class="pure-u-1-2 module module-ntp">Current time</div>
                        <div class="pure-u-11-24 module module-ntp">
                            <span class="right">{{now}}</span>
                        </div>

                        <div class="pure-u-1-2">Uptime</div>
                        <div class="pure-u-11-24"><span class="right">{{status.uptime}}</span></div>

                        <div class="pure-u-1-2">Last update</div>
                        <div class="pure-u-11-24">
                            <span class="right">{{status.last_update}}</span><span> seconds ago</span>
                        </div>
                    </div>
                </fieldset>
            </form>
            <fieldset>
                <legend>DEBUG LOG</legend>
                <h2>
                    Shows debug messages from the device
                </h2>

                <div class="pure-g module module-cmd">
                    <div class="pure-u-1 hint">
                        Write a command and click send to execute it on the device. The output will be
                        shown
                        in the debug text area below.
                    </div>
                    <Inpt name="dbgcmd" class="pure-u-3-4" type="text" tabindex="2"/>
                    <div class="pure-u-1-4 pure-u-lg-1-4">
                        <button type="button" class="pure-button button-dbgcmd pure-u-23-24">
                            Send
                        </button>
                    </div>
                </div>

                <div class="pure-g">
                    <textarea id="weblog"
                              class="pure-u-1 terminal"
                              name="weblog"
                              wrap="soft"
                              readonly></textarea>
                    <div class="pure-u-1-4 pure-u-lg-1-4">
                        <button type="button" class="pure-button button-dbg-clear pure-u-23-24">
                            Clear
                        </button>
                    </div>
                </div>
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';

    export default {
        components: {
            Inpt
        },
        data() {
            return {
                status: {},
                version: {}
            }
        }
    }
</script>

<style scoped>

</style>