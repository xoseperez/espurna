<template>
    <section>
        <div class="header">
            <h1>MQTT</h1>
            <h2>
                Configure an <strong>MQTT broker</strong> in your network and you will be able to change the switch
                status via an MQTT message.
            </h2>
        </div>

        <Group v-model="mqtt.settings" class="page form">
            <fieldset>
                <Row>
                    <C><label>Enable MQTT</label></C>
                    <C>
                        <Inpt type="switch"
                              name="enabled"
                              tabindex="30"/>
                    </C>
                </Row>

                <Row>
                    <C><label>MQTT Broker</label></C>
                    <C>
                        <Inpt name="server"
                              type="text"
                              tabindex="21"
                              placeholder="IP or address of your broker"/>
                    </C>
                </Row>

                <Row>
                    <C><label>MQTT Port</label></C>
                    <C>
                        <Inpt name="port"
                              type="text"
                              tabindex="22"
                              value="1883"/>
                    </C>
                </Row>

                <Row>
                    <C><label>MQTT User</label></C>
                    <C>
                        <Inpt name="user"
                              type="text"
                              tabindex="23"
                              placeholder="Leave blank if no user"
                              autocomplete="mqtt-user"/>
                        <Hint>
                            You can use the following placeholders: {hostname}, {mac}
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>MQTT Password</label></C>
                    <C>
                        <Inpt name="password"
                              type="password"
                              tabindex="24"
                              placeholder="Leave blank if no pass"
                              autocomplete="mqtt-password"
                              spellcheck="false"/>
                    </C>
                </Row>

                <Row>
                    <C><label>MQTT Client ID</label></C>
                    <C>
                        <Inpt name="clientID" type="text" tabindex="25"/>
                        <Hint>
                            If left empty the firmware will generate a client ID based
                            on the serial number of the chip. You can use the following placeholders: {hostname},
                            {mac}
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>MQTT Quality of Service</label></C>
                    <C>
                        <Inpt type="select" name="QoS"
                              tabindex="26" :options="[
                                  '0: At most once',
                                  '1: At least once',
                                  '2: Exactly once'
                              ]"/>
                    </C>
                </Row>

                <Row>
                    <C><label>MQTT Retain</label></C>
                    <C>
                        <Inpt type="switch"
                              name="retain"
                              tabindex="27"/>
                    </C>
                </Row>

                <Row>
                    <C><label>MQTT Keep Alive</label></C>
                    <C>
                        <Inpt type="number"
                              name="keep"
                              min="10"
                              max="3600"
                              tabindex="28"/>
                    </C>
                </Row>

                <template v-if="modules.mqttSSL">
                    <Row>
                        <C><label>Use secure connection (SSL)</label></C>
                        <C>
                            <Inpt type="switch"
                                  name="useSSL"
                                  tabindex="29"/>
                        </C>
                    </Row>

                    <Row>
                        <C><label>SSL Fingerprint</label></C>
                        <C>
                            <Inpt name="fingerprint"
                                  type="text"
                                  maxlength="59"
                                  tabindex="30"/>
                            <Hint>
                                This is the fingerprint for the SSL certificate of the server.<br> You can get it
                                using <A
                                    href="https://www.grc.com/fingerprints.htm">https://www.grc.com/fingerprints.htm</A><br>
                                or using openssl from a linux box by typing:<br>
                                <pre>$ openssl s_client -connect &lt;host&gt;:&lt;port&gt; &lt; /dev/null 2&gt;/dev/null | openssl x509 -fingerprint -noout -in /dev/stdin</pre>
                            </Hint>
                        </C>
                    </Row>
                </template>

                <Row>
                    <C>
                        <label>MQTT Root Topic</label>
                    </C>
                    <C>
                        <Inpt name="topic" type="text" tabindex="31"/>
                        <Hint>
                            This is the root topic for this device. The {hostname} and {mac} placeholders will be
                            replaced by the device hostname and MAC address.<br> -
                            <strong>&lt;root&gt;/relay/#/set</strong> Send a 0 or a 1 as a payload to this topic to
                            switch it on or off. You can also send a 2 to toggle its current state. Replace # with
                            the switch ID (starting from 0). If the board has only one switch it will be 0.<br>
                            <!-- #!if LIGHT === true -->
                            <template v-if="modules.color">
                                - <strong>&lt;root&gt;/rgb/set</strong> Set the color using this topic, your can
                                either send an "#RRGGBB" value or "RRR,GGG,BBB" (0-255 each).<br>
                                - <strong>&lt;root&gt;/hsv/set</strong> Set the color using hue (0-360), saturation
                                (0-100) and value (0-100) values, comma separated.<br>
                                - <strong>&lt;root&gt;/brightness/set</strong> Set the brighness (0-255).<br>
                                - <strong>&lt;root&gt;/channel/#/set</strong> Set the value for a single color
                                channel (0-255). Replace # with the channel ID (starting from 0 and up to 4 for
                                RGBWC lights).<br>
                                - <strong>&lt;root&gt;/mired/set</strong> Set the temperature color in mired.<br>
                            </template>
                            <!-- #!endif -->
                            - <strong>&lt;root&gt;/status</strong> The
                            device will report a 1 to this topic every few minutes. Upon MQTT disconnecting this
                            will be set to 0.<br> - Other values reported (depending on the build) are:
                            <strong>firmware</strong> and
                            <strong>version</strong>, <strong>hostname</strong>, <strong>IP</strong>,
                            <strong>MAC</strong>,
                            signal strenth (<strong>RSSI</strong>), <strong>uptime</strong> (in seconds),
                            <strong>free heap</strong> and <strong>power supply</strong>.
                        </Hint>
                    </c>
                </Row>



                <Row>
                    <C><label>Use JSON payload</label></C>
                    <C>
                        <Inpt type="switch"
                              name="useJson"
                              tabindex="33"/>
                        <Hint>
                            All messages (except the device status) will be included
                            in a JSON payload along with the timestamp and hostname and sent under the
                            <strong>&lt;root&gt;/data</strong>
                            topic.<br> Messages will be queued and sent after 100ms, so different messages could be
                            merged
                            into a single payload.<br> Subscriptions will still be done to single topics.
                        </Hint>
                    </C>
                </Row>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Row from "../../layout/Row";
    import C from "../../layout/Col";
    import Hint from "../../components/Hint";
    import Group from "../../components/Group";

    export default {
        components: {
            Group,
            Hint,
            Row,
            Inpt,
            C
        },
        inheritAttrs: false,
        props: {
            modules: {
                type: Object,
                default: () => ({})
            },
            mqtt: {
                type: Object,
                default: () => ({})
            }
        }
    }
</script>

<style lang="less">

</style>