<template>
    <section>
        <div class="header">
            <h1>ADMINISTRATION</h1>
            <h2>Device administration and security settings</h2>
        </div>
        <div class="page">
            <fieldset>
                <Row>
                    <C>
                        <label>Settings</label>
                    </C>
                    <C>
                        <Btn name="settings-backup">Backup</Btn>
                    </C>
                    <C>
                        <Btn name="settings-restore" color="accent">Restore</Btn>
                    </C>
                    <C>
                        <Btn name="settings-factory" color="danger">Factory Reset</Btn>
                    </C>
                </Row>
                <Row>
                    <C><label>Admin password</label></C>
                    <C>
                        <div>
                            <Inpt name="adminPass1" placeholder="New password"
                                  minlength="8"
                                  maxlength="63" type="password" action="reboot"
                                  tabindex="11" autocomplete="false"
                                  spellcheck="false"/>
                            <span class="password-reveal"></span>
                        </div>
                        <div>
                            <Inpt name="adminPass2" placeholder="Repeat password"
                                  minlength="8"
                                  maxlength="63" type="password" action="reboot"
                                  tabindex="12" autocomplete="false"
                                  spellcheck="false"/>
                            <span class="password-reveal"></span>
                        </div>
                        <div class="hint">
                            The administrator password is used to access this web interface (user 'admin'), but also to
                            connect to the device when in AP mode or to flash a new firmware over-the-air (OTA).<br> It
                            must be <strong>8..63 characters</strong> (numbers and letters and any of these special
                            characters: _,.;:~!?@#$%^&amp;*&lt;&gt;\|(){}[]) and have at least
                            <strong>one lowercase</strong> and <strong>one uppercase</strong> or
                            <strong>one number</strong>.
                        </div>
                    </C>
                </Row>
                <Row>
                    <C><label>HTTP port</label></C>
                    <C>
                        <Inpt name="webPort" type="text" action="reboot" tabindex="13"/>
                        <div class="hint">
                            This is the port for the web interface and API requests.
                            If different than 80 (standard HTTP port) you will have to add it explicitly to your
                            requests:
                            http://myip:myport/
                        </div>
                    </C>
                </Row>
                <Row>
                    <C><label>Enable WS Auth</label></C>
                    <C>
                        <Inpt type="switch" name="wsAuth"/>
                    </C>
                </Row>
                <Row ref="api">
                    <C><label>Enable HTTP API</label></C>
                    <C>
                        <Inpt type="switch" name="apiEnabled"/>
                    </C>
                </Row>
                <Row ref="api">
                    <C><label>Restful API</label></C>
                    <C>
                        <Inpt type="switch" name="apiRestFul"/>
                        <div class="hint">
                            If enabled, API requests to change a status (like a relay)
                            must be done using PUT. If disabled you can issue them as GET requests (easier from a
                            browser).
                        </div>
                    </C>
                </Row>
                <Row ref="api">
                    <C><label>Real time API</label></C>
                    <C>
                        <Inpt type="switch" name="apiRealTime"/>
                        <div class="hint">
                            By default, some magnitudes are being preprocessed and
                            filtered to avoid spurious values. If you want to get real-time values (not preprocessed) in
                            the
                            API turn on this setting.
                        </div>
                    </C>
                </Row>
                <Row ref="api">
                    <C><label>HTTP API Key</label></C>
                    <C>
                        <Inpt name="apiKey" type="text" tabindex="14"/>
                        <Btn name="apikey" color="accent">Auto</Btn>
                        <div class="hint">
                            This is the key you will have to pass with every HTTP
                            request to the API, either to get or write values. All API calls must contain the
                            <strong>apikey</strong> parameter with the value above. To know what APIs are enabled do a
                            call to <strong>/apis</strong>.
                        </div>
                    </C>
                </Row>
                <Row ref="telnet">
                    <C><label>Enable TELNET</label></C>
                    <C>
                        <Inpt type="switch" name="telnetSTA"/>
                        <div class="hint">
                            Turn ON to be able to telnet to your device while
                            connected to your home router.<br>TELNET is always enabled in AP mode.
                        </div>
                    </C>
                </Row>
                <Row ref="telnet">
                    <C><label>TELNET Password</label></C>
                    <C>
                        <Inpt type="switch" name="telnetAuth"/>
                        <div class="hint">Request password when starting telnet session</div>
                    </C>
                </Row>
                <Row>
                    <C><label>Heartbeat message</label></C>
                    <C>
                        <Inpt type="select" name="hbMode"
                              tabindex="15" :options="[
                                  'Disabled',
                                  'On device startup',
                                  'Repeat after defined interval',
                                  'Repeat only device status'
                              ]"/>
                        <div class="hint">Define when heartbeat message will be sent.</div>
                    </C>
                </Row>
                <Row>
                    <C><label>Heartbeat interval</label></C>
                    <C>
                        <Inpt name="hbInterval" type="number" tabindex="16"/>
                        <div class="hint">
                            This is the interval in <strong>seconds</strong> how often
                            to send the heartbeat message.
                        </div>
                    </C>
                </Row>
                <Row>
                    <C><label>Upgrade</label></C>
                    <C>
                        <input name="filename" type="text" :value="filename" readonly>
                        <Btn name="upgrade-browse">Browse</Btn>
                        <Btn name="upgrade" color="danger">Upgrade</Btn>
                        <div class="hint">
                            The device has {{status.free_size}} bytes available for
                            OTA updates. If your image is larger than this consider doing a
                            <A href="https://github.com/xoseperez/espurna/wiki/TwoStepUpdates"><strong>two-step
                                update</strong></A>.
                        </div>
                        <div>
                            <progress id="upgrade-progress"></progress>
                        </div>
                        <Inpt name="upgrade" type="file" tabindex="17"/>
                    </C>
                </Row>
            </fieldset>
            <fieldset>
                <legend>NTP</legend>
                <h2>
                    Configure your NTP (Network Time Protocol) servers and local configuration to keep your device time
                    up to the second for your location.
                </h2>
                <Row>
                    <C><label>Device Current Time</label></C>
                    <C>
                        <Inpt name="now" type="text" readonly/>
                    </C>
                </Row>
                <Row>
                    <C><label>NTP Server</label></C>
                    <C>
                        <Inpt name="ntpServer" type="text" tabindex="41"/>
                    </C>
                </Row>
                <Row>
                    <C><label>Time Zone</label></C>
                    <C>
                        <Inpt type="select" name="ntpOffset"
                              :options="ntpOffsets"
                              tabindex="42"/>
                    </C>
                </Row>
                <Row>
                    <C><label>Enable DST</label></C>
                    <C>
                        <Inpt type="switch" name="ntpDST"/>
                    </C>
                </Row>
                <Row>
                    <C><label>DST Region</label></C>
                    <C>
                        <Inpt type="select" name="ntpRegion"
                              :options="['Europe', 'USA']"/>
                    </C>
                </Row>
            </fieldset>
        </div>
    </section>
</template>
<script>
    import Inpt from './../../components/Input';
    import Btn from './../../components/Button';
    import A from './../../components/ExtLink';
    import Row from "../../layout/Row";
    import C from "../../layout/Col";

    export default {
        components: {C, Row, Inpt, Btn, A},
        data() {
            return {
                status: {}
            }
        },
        computed: {
            ntpOffsets() {
                const tz = [
                    -720, -660, -600, -570, -540,
                    -480, -420, -360, -300, -240,
                    -210, -180, -120, -60, 0,
                    60, 120, 180, 210, 240,
                    270, 300, 330, 345, 360,
                    390, 420, 480, 510, 525,
                    540, 570, 600, 630, 660,
                    720, 765, 780, 840
                ];
                let offsets = [];

                tz.forEach((v) => {
                    const text = "GMT" + (v >= 0 ? "+" : "-") + new Date((1440 + Math.abs(v)) * 60000).toISOString().substr(11, 5);
                    offsets.push({k: v, l: text})
                });

                return offsets;
            }
        }
    }
</script>
<style>
    #upgrade-progress {
        display: none;
        height: 20px;
        margin-top: 10px;
        width: 100%;
    }

    #uploader,
    #downloader {
        display: none;
    }

</style>