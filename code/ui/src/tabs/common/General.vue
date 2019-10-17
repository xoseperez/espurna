<template>
    <section>
        <div class="header">
            <h1>GENERAL</h1>
            <h2>General configuration values</h2>
        </div>

        <div class="page">
            <fieldset>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Hostname</label>
                    <Inpt name="hostname"
                          class="pure-u-1 pure-u-lg-1-4"
                          maxlength="31"
                          type="text"
                          action="reboot"
                          tabindex="1"/>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        This name will identify this device in your network (http://&lt;hostname&gt;.local).<br>
                        Hostname may contain only the ASCII letters 'a' through 'z' (in a case-insensitive manner), the
                        digits '0' through '9', and the hyphen ('-'). They can neither start or end with an hyphen.<br>
                        For this setting to take effect you should restart the wifi interface by clicking the
                        "Reconnect" button.
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Description</label>
                    <Inpt name="desc"
                          class="pure-u-1 pure-u-lg-3-4"
                          maxlength="64"
                          type="text"
                          tabindex="2"/>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Human-friendly name for your device. Will be reported with the heartbeat.<br>
                        You can use this to specify the location or some other identification
                        information.
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Double click delay</label>
                    <Inpt name="btnDelay"
                          class="pure-u-1 pure-u-lg-1-4"
                          type="number"
                          action="reboot"
                          min="0"
                          step="100"
                          max="1000"
                          tabindex="6"/>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Delay in milliseconds to detect a double click (from 0 to 1000ms).<br> The lower this number the
                        faster the device will respond to button clicks but the harder it will be to get a double click.
                        Increase this number if you are having trouble to double click the button. Set this value to 0
                        to disable double click. You won't be able to set the device in AP mode manually but your device
                        will respond immediately to button clicks.<br> You will have to
                        <strong>reboot the device</strong> after updating for this setting to apply.
                    </div>
                </div>

                <div class="pure-g module module-alexa">
                    <label class="pure-u-1 pure-u-lg-1-4">Alexa integration</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch" name="alexaEnabled"/>
                    </div>
                </div>

                <div class="pure-g module module-alexa">
                    <label class="pure-u-1 pure-u-lg-1-4">Alexa device name</label>
                    <Inpt name="alexaName"
                          class="pure-u-1 pure-u-lg-1-4"
                          maxlength="31"
                          type="text"
                          action="reboot"
                          tabindex="7"/>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        This name will be used in Alexa integration.<br>
                    </div>
                </div>
            </fieldset>
            <fieldset>
                <legend>Wifi</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Scan networks</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="wifiScan"
                              tabindex="1"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        ESPurna will scan for visible WiFi SSIDs and try to connect to networks defined below in order
                        of <strong>signal strength</strong>, even if multiple AP share the same SSID.
                        When disabled, ESPurna will try to connect to the networks in the same order they are listed
                        below. Disable this option if you are <strong>connecting to a single access point</strong>
                        (or router) or to a <strong>hidden SSID</strong>.
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <button class="btn btn-wifi-scan" type="button">Scan now</button>
                    <div v-if="scanLoading" class="pure-u-0 pure-u-lg-1-4 scan loading"></div>
                </div>

                <div v-if="scanResult.length" class="pure-g">
                    <div class="pure-u-1 terminal">
                        <p v-for="res in scanResult">
                            {{res}}
                        </p>
                    </div>
                </div>

                <legend>Networks</legend>

                <div id="networks"></div>

                <button type="button" class="btn btn-add-network">Add network</button>
            </fieldset>
        </div>


        <div id="networkTemplate" class="template">
            <div class="pure-g">
                <label class="pure-u-1 pure-u-lg-1-4">Network SSID</label>
                <div class="pure-u-5-6 pure-u-lg-2-3">
                    <Inpt name="ssid"
                          type="text"
                          action="reconnect"
                          class="pure-u-23-24"
                          value=""
                          tabindex="0"
                          placeholder="Network SSID"
                          required
                          autocomplete="false"/>
                </div>
                <div class="pure-u-1-6 pure-u-lg-1-12">
                    <button type="button" class="btn btn-more-network pure-u-1">...</button>
                </div>

                <label class="pure-u-1 pure-u-lg-1-4 more">Password</label>
                <Inpt class="pure-u-1 pure-u-lg-3-4 more"
                      name="pass"
                      type="password"
                      action="reconnect"
                      value=""
                      tabindex="0"
                      autocomplete="new-password"
                      spellcheck="false"/>
                <span class="no-select password-reveal more"></span>

                <label class="pure-u-1 pure-u-lg-1-4 more">Static IP</label>
                <Inpt class="pure-u-1 pure-u-lg-3-4 more"
                      name="ip"
                      type="text"
                      action="reconnect"
                      value=""
                      maxlength="15"
                      tabindex="0"
                      autocomplete="false"/>
                <div class="pure-u-0 pure-u-lg-1-4 more"></div>
                <div class="pure-u-1 pure-u-lg-3-4 hint more">Leave empty for DHCP negotiation</div>

                <label class="pure-u-1 pure-u-lg-1-4 more">Gateway IP</label>
                <Inpt class="pure-u-1 pure-u-lg-3-4 more"
                      name="gw"
                      type="text"
                      action="reconnect"
                      value=""
                      maxlength="15"
                      tabindex="0"
                      autocomplete="false"/>
                <div class="pure-u-0 pure-u-lg-1-4 more"></div>
                <div class="pure-u-1 pure-u-lg-3-4 hint more">Set when using a static IP</div>

                <label class="pure-u-1 pure-u-lg-1-4 more">Network Mask</label>
                <Inpt class="pure-u-1 pure-u-lg-3-4 more"
                      name="mask"
                      type="text"
                      action="reconnect"
                      placeholder="255.255.255.0"
                      maxlength="15"
                      tabindex="0"
                      autocomplete="false"/>
                <div class="pure-u-0 pure-u-lg-1-4 more"></div>
                <div class="pure-u-1 pure-u-lg-3-4 hint more">Usually 255.255.255.0 for /24 networks</div>

                <label class="pure-u-1 pure-u-lg-1-4 more">DNS IP</label>
                <Inpt class="pure-u-1 pure-u-lg-3-4 more"
                      name="dns"
                      type="text"
                      action="reconnect"
                      value="8.8.8.8"
                      maxlength="15"
                      tabindex="0"
                      autocomplete="false"/>
                <div class="pure-u-0 pure-u-lg-1-4 more"></div>
                <div class="pure-u-1 pure-u-lg-3-4 hint more">
                    Set the Domain Name Server IP to use when using a static IP
                </div>

                <div class="pure-u-0 pure-u-lg-1-4 more"></div>
                <Btn name="del-network" color="danger">Delete network</Btn>
            </div>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';
    import Btn from "../../components/Button";

    export default {
        components: {
            Btn,
            Inpt
        },
        data() {
            return {
                scanResult: [],
                scanLoading: false
            }
        }
    }
</script>

<style lang="less">

</style>