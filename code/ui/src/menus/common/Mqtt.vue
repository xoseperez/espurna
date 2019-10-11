<template>
    <section>
        <div class="header">
            <h1>MQTT</h1>
            <h2> Configure an <strong>MQTT broker</strong> in your network and you will be able to change the switch
                status via an MQTT message. </h2>
        </div>

        <div class="page">
            <fieldset>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Enable MQTT</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="mqttEnabled"
                              tabindex="30"/>
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">MQTT Broker</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4"
                          name="mqttServer"
                          type="text"
                          tabindex="21"
                          placeholder="IP or address of your broker"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">MQTT Port</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4"
                          name="mqttPort"
                          type="text"
                          tabindex="22"
                          value="1883"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">MQTT User</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4"
                          name="mqttUser"
                          type="text"
                          tabindex="23"
                          placeholder="Leave blank if no user"
                          autocomplete="off"/>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        You can use the following placeholders: {hostname}, {mac}
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">MQTT Password</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4"
                          name="mqttPassword"
                          type="password"
                          tabindex="24"
                          placeholder="Leave blank if no pass"
                          autocomplete="new-password"
                          spellcheck="false"/>
                    <span class="no-select password-reveal"></span>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">MQTT Client ID</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4" name="mqttClientID" type="text" tabindex="25"/>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint"> If left empty the firmware will generate a client ID based
                        on the serial number of the chip. You can use the following placeholders: {hostname}, {mac}
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">MQTT QoS</label>
                    <select class="pure-u-1 pure-u-lg-1-4" name="mqttQoS" tabindex="26">
                        <option value="0">0: At most once</option>
                        <option value="1">1: At least once</option>
                        <option value="2">2: Exactly once</option>
                    </select>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">MQTT Retain</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="mqttRetain"
                              tabindex="27"/>
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">MQTT Keep Alive</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4"
                          type="number"
                          name="mqttKeep"
                          min="10"
                          max="3600"
                          tabindex="28"/>
                </div>

                <div class="pure-g module module-mqttssl">
                    <label class="pure-u-1 pure-u-lg-1-4">Use secure connection (SSL)</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="mqttUseSSL"
                              tabindex="29"/>
                    </div>
                </div>

                <div class="pure-g module module-mqttssl">
                    <label class="pure-u-1 pure-u-lg-1-4">SSL Fingerprint</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4"
                          name="mqttFP"
                          type="text"
                          maxlength="59"
                          tabindex="30"/>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        This is the fingerprint for the SSL certificate of the server.<br> You can get it using <a
                            href="https://www.grc.com/fingerprints.htm" rel="noopener" target="_blank">https://www.grc.com/fingerprints.htm</a><br>
                        or using openssl from a linux box by typing:<br>
                        <pre>$ openssl s_client -connect &lt;host&gt;:&lt;port&gt; &lt; /dev/null 2&gt;/dev/null | openssl x509 -fingerprint -noout -in /dev/stdin</pre>
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">MQTT Root Topic</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4" name="mqttTopic" type="text" tabindex="31"/>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        This is the root topic for this device. The {hostname} and {mac} placeholders will be replaced
                        by the device hostname and MAC address.<br> - <strong>&lt;root&gt;/relay/#/set</strong> Send a 0
                        or a 1 as a payload to this topic to switch it on or off. You can also send a 2 to toggle its
                        current state. Replace # with the switch ID (starting from 0). If the board has only one switch
                        it will be 0.<br>
                        <!-- removeIf(!light) -->
                        <span class="module module-color">- <strong>&lt;root&gt;/rgb/set</strong> Set the color using this topic, your can either send an "#RRGGBB" value or "RRR,GGG,BBB" (0-255 each).<br></span>
                        <span class="module module-color">- <strong>&lt;root&gt;/hsv/set</strong> Set the color using hue (0-360), saturation (0-100) and value (0-100) values, comma separated.<br></span>
                        <span class="module module-color">- <strong>&lt;root&gt;/brightness/set</strong> Set the brighness (0-255).<br></span>
                        <span class="module module-color">- <strong>&lt;root&gt;/channel/#/set</strong> Set the value for a single color channel (0-255). Replace # with the channel ID (starting from 0 and up to 4 for RGBWC lights).<br></span>
                        <span class="module module-color">- <strong>&lt;root&gt;/mired/set</strong> Set the temperature color in mired.<br></span>
                        <!-- endRemoveIf(!light) -->
                        - <strong>&lt;root&gt;/status</strong> The
                        device will report a 1 to this topic every few minutes. Upon MQTT disconnecting this will be set
                        to 0.<br> - Other values reported (depending on the build) are: <strong>firmware</strong> and
                        <strong>version</strong>, <strong>hostname</strong>, <strong>IP</strong>, <strong>MAC</strong>,
                        signal strenth (<strong>RSSI</strong>), <strong>uptime</strong> (in seconds), <strong>free
                        heap</strong> and <strong>power supply</strong>.
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Send all button events</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="mqttSendAllButtonEvents"
                              tabindex="32"/>
                    </div>
                    <div class="pure-u-1 pure-u-lg-1-2"></div>
                    <div class="pure-u-1 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        If you need to use double taps (code: 3) or long taps (code: 4) for switches, enable
                        this feature
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use JSON payload</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="mqttUseJson"
                              tabindex="33"/>
                    </div>
                    <div class="pure-u-1 pure-u-lg-1-2"></div>
                    <div class="pure-u-1 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        All messages (except the device status) will be included
                        in a JSON payload along with the timestamp and hostname and sent under the <strong>&lt;root&gt;/data</strong>
                        topic.<br> Messages will be queued and sent after 100ms, so different messages could be merged
                        into a single payload.<br> Subscriptions will still be done to single topics.
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
        }
    }
</script>

<style scoped>

</style>