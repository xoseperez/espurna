<template>
    <section>
        <div class="header">
            <h1>GENERAL</h1>
            <h2>General configuration values</h2>
        </div>

        <Group v-model="device" class="page form">
            <fieldset>
                <Row>
                    <C><label>Hostname</label></C>
                    <C>
                        <Inpt name="hostname"
                              maxlength="31"
                              type="text"
                              action="reboot"
                              tabindex="1"/>
                        <Hint>
                            This name will identify this device in your network (http://&lt;hostname&gt;.local).<br>
                            Hostname may contain only the ASCII letters 'a' through 'z' (in a case-insensitive manner),
                            the digits '0' through '9', and the hyphen ('-'). They can neither start or end with an
                            hyphen.<br> For this setting to take effect you should restart the wifi interface by
                            clicking the "Reconnect" button.
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>Description</label></C>
                    <C>
                        <Inpt name="desc"
                              maxlength="64"
                              type="text"
                              tabindex="2"/>
                        <Hint>
                            Human-friendly name for your device. Will be reported with the heartbeat.<br> You can use
                            this to specify the location or some other identification information.
                        </Hint>
                    </C>
                </Row>
            </fieldset>
        </Group>
        <Group v-model="wifi" class="page form">
            <fieldset>
                <legend>Wifi</legend>

                <Row>
                    <C><label>Scan networks</label></C>
                    <C>
                        <Inpt type="switch" name="scan"/>
                        <Hint>
                            ESPurna will scan for visible WiFi SSIDs and try to connect to networks defined below in
                            order of <strong>signal strength</strong>, even if multiple AP share the same SSID. When
                            disabled, ESPurna will try to connect to the networks in the same order they are listed
                            below. Disable this option if you are <strong>connecting to a single access point</strong>
                            (or router) or to a <strong>hidden SSID</strong>.
                        </Hint>
                        <Btn v-loading="scanLoading" class="small"
                             name="wifi-scan" color="primary" @click="wifiScan">
                            Scan now
                        </Btn>
                        <textarea v-if="scanResult.length"
                                  :value="scanFormatted"
                                  class="terminal"
                                  wrap="soft"
                                  readonly spellcheck="false"></textarea>
                    </C>
                </Row>

                <Row v-if="scanResult.length">
                    <div class="terminal">
                        <p v-for="(res, i) in scanResult" :key="i">
                            {{res}}
                        </p>
                    </div>
                </Row>

                <legend>Networks</legend>
                <Wifi v-model="wifi.list" :max="wifi.max"/>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Btn from "../../components/Button";
    import C from "../../layout/Col";
    import Hint from "../../components/Hint";
    import Row from "../../layout/Row";
    import Group from "../../components/Group";
    import Wifi from "../../components/Wifi";
    import ws from "./../../common/websocket";
    import prepareData from "../../common/prepareData";

    export default {
        components: {
            Wifi,
            Group,
            Row,
            Hint,
            C,
            Btn,
            Inpt
        },
        inheritAttrs: false,
        props: {
            wifi: {
                type: Object,
                default: () => ({})
            },
            device: {
                type: Object,
                default: () => ({})
            }
        },
        data() {
            return {
                scanResult: [],
                scanLoading: false
            };
        },
        computed: {
            scanFormatted() {
                return this.scanResult.map((v) => {
                    return JSON.stringify(v);
                }).join("\n");
            }
        },
        methods: {
            wifiScan() {
                this.scanLoading = true;
                this.scanResult = [];
                ws.send({action: "scan"}, (res) => {
                    this.scanLoading = false;
                    if (res.success) {
                        prepareData(this.scanResult, res);
                    }
                });
            },
        }
    };
</script>

<style lang="less">

</style>
