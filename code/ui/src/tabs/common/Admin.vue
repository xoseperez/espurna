<template>
    <section>
        <div class="header">
            <h1>ADMINISTRATION</h1>
            <h2>Device administration and security settings</h2>
        </div>
        <div class="page">
            <fieldset>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Settings</label>
                    <div class="pure-u-1-3 pure-u-lg-1-4">
                        <Btn name="settings-backup" class="pure-u-23-24">Backup</Btn>
                    </div>
                    <div class="pure-u-1-3 pure-u-lg-1-4">
                        <Btn name="settings-restore" color="accent" class="pure-u-23-24">Restore</Btn>
                    </div>
                    <div class="pure-u-1-3 pure-u-lg-1-4">
                        <Btn name="settings-factory" color="danger" class="pure-u-1">Factory Reset</Btn>
                    </div>
                </div>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Admin password</label>
                    <Inpt name="adminPass1" class="pure-u-1 pure-u-lg-3-4" placeholder="New password"
                          minlength="8"
                          maxlength="63" type="password" action="reboot"
                          tabindex="11" autocomplete="false"
                          spellcheck="false"/>
                    <span class="no-select password-reveal"></span>
                    <div class="pure-u-1 pure-u-lg-1-4"></div>
                    <Inpt name="adminPass2" class="pure-u-1 pure-u-lg-3-4" placeholder="Repeat password"
                          minlength="8"
                          maxlength="63" type="password" action="reboot"
                          tabindex="12" autocomplete="false"
                          spellcheck="false"/>
                    <span class="no-select password-reveal"></span>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        The administrator password is used to access this web
                        interface (user 'admin'), but also to connect to the device when in AP mode or to flash a new
                        firmware over-the-air (OTA).<br> It must be <strong>8..63 characters</strong> (numbers and
                        letters and any of these special characters: _,.;:~!?@#$%^&amp;*&lt;&gt;\|(){}[]) and have at
                        least <strong>one lowercase</strong> and <strong>one uppercase</strong> or <strong>one
                            number</strong>.
                    </div>
                </div>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">HTTP port</label>
                    <Inpt name="webPort" class="pure-u-1 pure-u-lg-1-4" type="text" action="reboot" tabindex="13"/>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        This is the port for the web interface and API requests.
                        If different than 80 (standard HTTP port) you will have to add it explicitly to your requests:
                        http://myip:myport/
                    </div>
                </div>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Enable WS Auth</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch" name="wsAuth"/>
                    </div>
                </div>
                <div class="pure-g module module-api">
                    <label class="pure-u-1 pure-u-lg-1-4">Enable HTTP API</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch" name="apiEnabled"/>
                    </div>
                </div>
                <div class="pure-g module module-api">
                    <label class="pure-u-1 pure-u-lg-1-4">Restful API</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch" name="apiRestFul"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        If enabled, API requests to change a status (like a relay)
                        must be done using PUT. If disabled you can issue them as GET requests (easier from a browser).
                    </div>
                </div>
                <div class="pure-g module module-api">
                    <label class="pure-u-1 pure-u-lg-1-4">Real time API</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch" name="apiRealTime"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        By default, some magnitudes are being preprocessed and
                        filtered to avoid spurious values. If you want to get real-time values (not preprocessed) in the
                        API turn on this setting.
                    </div>
                </div>
                <div class="pure-g module module-api">
                    <label class="pure-u-1 pure-u-lg-1-4">HTTP API Key</label>
                    <Inpt name="apiKey" class="pure-u-3-4 pure-u-lg-1-2" type="text" tabindex="14"/>
                    <div class="pure-u-1-4 pure-u-lg-1-4">
                        <Btn name="apikey" color="accent" class="pure-u-23-24">Auto</Btn>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        This is the key you will have to pass with every HTTP
                        request to the API, either to get or write values. All API calls must contain the
                        <strong>apikey</strong> parameter with the value above. To know what APIs are enabled do a call
                        to <strong>/apis</strong>.
                    </div>
                </div>
                <div class="pure-g module module-telnet">
                    <label class="pure-u-1 pure-u-lg-1-4">Enable TELNET</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch" name="telnetSTA"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Turn ON to be able to telnet to your device while
                        connected to your home router.<br>TELNET is always enabled in AP mode.
                    </div>
                </div>
                <div class="pure-g module module-telnet">
                    <label class="pure-u-1 pure-u-lg-1-4">TELNET Password</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch" name="telnetAuth"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint"> Request password when starting telnet session</div>
                </div>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Heartbeat message</label>
                    <Inpt type="select" class="pure-u-1 pure-u-lg-3-4" name="hbMode"
                          tabindex="15" :options="[
                              'Disabled',
                              'On device startup',
                              'Repeat after defined interval',
                              'Repeat only device status'
                          ]"/>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint"> Define when heartbeat message will be sent.</div>
                </div>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Heartbeat interval</label>
                    <Inpt name="hbInterval" class="pure-u-1 pure-u-lg-1-4" type="number" tabindex="16"/>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        This is the interval in <strong>seconds</strong> how often
                        to send the heartbeat message.
                    </div>
                </div>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Upgrade</label>
                    <input class="pure-u-1-2 pure-u-lg-1-2" name="filename" type="text" :value="filename">
                    <div class="pure-u-1-4 pure-u-lg-1-8">
                        <Btn name="upgrade-browse" class="pure-u-23-24">Browse</Btn>
                    </div>
                    <div class="pure-u-1-4 pure-u-lg-1-8">
                        <Btn name="upgrade" color="danger" class="pure-u-23-24">Upgrade</Btn>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        The device has {{status.free_size}} bytes available for
                        OTA updates. If your image is larger than this consider doing a <A href="https://github.com/xoseperez/espurna/wiki/TwoStepUpdates"><strong>two-step update</strong></A>.
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4">
                        <progress id="upgrade-progress"></progress>
                    </div>
                    <Inpt name="upgrade" type="file" tabindex="17"/>
                </div>
            </fieldset>
            <fieldset>
                <legend>NTP</legend>
                <h2>
                    Configure your NTP (Network Time Protocol) servers and local configuration to keep your device time
                    up to the second for your location.
                </h2>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Device Current Time</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4" name="now" type="text" readonly/>
                </div>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">NTP Server</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4" name="ntpServer" type="text" tabindex="41"/>
                </div>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Time Zone</label>
                    <Inpt type="select" class="pure-u-1 pure-u-lg-1-4" name="ntpOffset"
                          :options="ntpOffsets"
                          tabindex="42"/>
                </div>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Enable DST</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch" name="ntpDST"/>
                    </div>
                </div>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">DST Region</label>
                    <Inpt type="select" class="pure-u-1 pure-u-lg-1-4" name="ntpRegion" :options="['Europe', 'USA']"/>
                </div>
            </fieldset>
        </div>
    </section>
</template>
<script>
    import Inpt from './../../components/Input';
    import Btn from './../../components/Button';
    import A from './../../components/ExtLink';

    export default {
        components: {Inpt, Btn, A},
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