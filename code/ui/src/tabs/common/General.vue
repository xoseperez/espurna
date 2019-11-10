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
                            the
                            digits '0' through '9', and the hyphen ('-'). They can neither start or end with an
                            hyphen.<br>
                            For this setting to take effect you should restart the wifi interface by clicking the
                            "Reconnect" button.
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
                            Human-friendly name for your device. Will be reported with the heartbeat.<br>
                            You can use this to specify the location or some other identification
                            information.
                        </Hint>
                    </C>
                </Row>
            </fieldset>
            <fieldset>
                <legend>Wifi</legend>

                <Row>
                    <C><label>Scan networks</label></C>
                    <C>
                        <Inpt type="switch" name="wifiScan"/>
                        <Hint>
                            ESPurna will scan for visible WiFi SSIDs and try to connect to networks defined below in
                            order of <strong>signal strength</strong>, even if multiple AP share the same SSID.
                            When disabled, ESPurna will try to connect to the networks in the same order they are listed
                            below. Disable this option if you are <strong>connecting to a single access point</strong>
                            (or router) or to a <strong>hidden SSID</strong>.
                        </Hint>
                        <Btn name="wifi-scan" color="primary" @click="wifiScan">Scan now</Btn>
                        <div v-if="scanLoading" class="loading"></div>
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
                <Repeater v-model="wifi.list" @created="({row})=>$set(row, 'more', false)">
                    <template #default="tpl">
                        <Row>
                            <C><label :for="'ssid-'+tpl.row.key">Network SSID</label></C>
                            <C no-wrap>
                                <Inpt :id="'ssid-'+tpl.row.key" name="ssid"
                                      type="text"
                                      action="reconnect"
                                      tabindex="0"
                                      placeholder="Network SSID"
                                      required
                                      autocomplete="network-ssid"/>
                                <Btn @click="() => { $set(tpl.row, 'more', !tpl.row.more) }">...</Btn>
                            </C>
                            <template v-if="tpl.row.more">
                                <C><label>Password</label></C>
                                <C>
                                    <Inpt name="pass"
                                          type="password"
                                          action="reconnect"
                                          tabindex="0"
                                          autocomplete="network-password"
                                          spellcheck="false"/>
                                </C>


                                <C><label>Static IP</label></C>
                                <C>
                                    <Inpt name="ip"
                                          type="text"
                                          action="reconnect"
                                          maxlength="15"
                                          tabindex="0"
                                          autocomplete="false"/>
                                    <Hint>Leave empty for DHCP negotiation</Hint>
                                </C>

                                <C><label>Gateway IP</label></C>
                                <C>
                                    <Inpt name="gw"
                                          type="text"
                                          action="reconnect"
                                          maxlength="15"
                                          tabindex="0"
                                          autocomplete="false"/>
                                    <Hint>Set when using a static IP</Hint>
                                </C>

                                <C><label>Network Mask</label></C>
                                <C>
                                    <Inpt name="mask"
                                          type="text"
                                          action="reconnect"
                                          placeholder="255.255.255.0"
                                          maxlength="15"
                                          tabindex="0"
                                          autocomplete="false"/>
                                    <Hint>Usually 255.255.255.0 for /24 networks</Hint>
                                </C>

                                <C><label>DNS IP</label></C>
                                <C>
                                    <Inpt name="dns"
                                          type="text"
                                          action="reconnect"
                                          value="8.8.8.8"
                                          maxlength="15"
                                          tabindex="0"
                                          autocomplete="false"/>
                                    <Hint>
                                        Set the Domain Name Server IP to use when using a static IP
                                    </Hint>
                                </C>
                            </template>
                        </Row>
                    </template>
                    <template #btnRemove="tpl">
                        <Btn name="del-network" color="danger" @click="tpl.click">Delete network</Btn>
                    </template>
                    <template #btnAdd="tpl">
                        <Btn name="add-network" @click="tpl.click">Add network</Btn>
                    </template>
                </Repeater>
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
    import Repeater from "../../components/Repeater";
    import Group from "../../components/Group";

    export default {
        components: {
            Group,
            Repeater,
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
            }
        },
        methods: {
            wifiScan() {
                this.scanLoading = true;
                this.scanResult = [];
                this.$emit("action", "scan");
            },
        }
    }
</script>

<style lang="less">

</style>