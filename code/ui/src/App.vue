<template>
    <div>
        <div id="password" class="webmode">
            <div class="content">
                <form id="formPassword" class="pure-form" autocomplete="off">
                    <div id="panel-password" class="panel block">
                        <div class="header">
                            <h1>SECURITY</h1>
                            <h2>
                                Before using this device you have to change the default password for the user
                                <strong>admin</strong>. This password will be used for the <strong>AP mode
                                hotspot</strong>,
                                the <strong>web interface</strong> (where you are now) and the <strong>over-the-air
                                updates</strong>.
                            </h2>
                        </div>

                        <div class="page">
                            <fieldset>
                                <div class="pure-g">
                                    <label class="pure-u-1 pure-u-lg-1-4" for="adminPass1">New Password</label>
                                    <Inpt class="pure-u-1 pure-u-lg-3-4"
                                          name="adminPass1"
                                          minlength="8"
                                          maxlength="63"
                                          type="password"
                                          tabindex="1"
                                          autocomplete="false"
                                          spellcheck="false"
                                          required/>
                                    <span class="no-select password-reveal"></span>
                                </div>

                                <div class="pure-g">
                                    <label class="pure-u-1 pure-u-lg-1-4" for="adminPass2">Repeat password</label>
                                    <Inpt class="pure-u-1 pure-u-lg-3-4"
                                          name="adminPass2"
                                          minlength="8"
                                          maxlength="63"
                                          type="password"
                                          tabindex="2"
                                          autocomplete="false"
                                          spellcheck="false"
                                          required/>
                                    <span class="no-select password-reveal"></span>
                                </div>
                            </fieldset>

                            <div class="pure-g">
                                <div class="pure-u-1 pure-u-lg-1 hint">
                                    Password must be <strong>8..63 characters</strong> (numbers and letters and any of
                                    these
                                    special characters: _,.;:~!?@#$%^&amp;*&lt;&gt;\|(){}[]) and have at least <strong>one
                                    lowercase</strong> and <strong>one uppercase</strong> or <strong>one number</strong>.
                                </div>
                            </div>


                            <div class="pure-g">
                                <button class="pure-u-11-24 pure-u-lg-1-4 pure-button button-generate-password"
                                        type="button"
                                        title="Generate password based on password policy">
                                    Generate
                                </button>
                                <div class="pure-u-2-24 pure-u-lg-1-2"></div>
                                <button class="pure-u-11-24 pure-u-lg-1-4 pure-button button-update-password"
                                        type="button"
                                        title="Save new password">
                                    Save
                                </button>
                            </div>
                        </div>
                    </div>
                </form>
            </div> <!-- content -->
        </div>

        <div id="layout" class="webmode">
            <a id="menuLink" class="menu-link">
                <span></span>
            </a>

            <Menu :tabs="tabs">
                <template #header>
                    <span class="pure-menu-heading hostname">{{hostname}}</span>
                    <span class="pure-menu-heading small title">ESPurna {{version}}</span>
                    <span class="pure-menu-heading small desc">{{description}}</span>
                </template>

                <template #footer>
                    <div class="main-buttons">
                        <button class="pure-button button-update" @click="save">Save</button>
                        <button class="pure-button button-reconnect" @click="reconnect">Reconnect</button>
                        <button class="pure-button button-reboot" @click="reboot">Reboot</button>
                    </div>

                    <div class="footer">
                        &copy; 2016-2019<br>
                        Xose Pérez<br>
                        <a href="https://twitter.com/xoseperez" rel="noopener" target="_blank">@xoseperez</a><br>
                        <a href="http://tinkerman.cat" rel="noopener" target="_blank">http://tinkerman.cat</a><br>
                        <a href="https://github.com/xoseperez/espurna" rel="noopener" target="_blank">ESPurna @
                            GitHub</a><br>
                        UI by <a href="https://github.com/tofandel" rel="noopener" target="_blank">Tofandel</a>
                        GPLv3 license<br>
                    </div>
                </template>

                <template #status>
                    <div class="header">
                        <h1>STATUS</h1>
                        <h2>Current configuration</h2>
                    </div>

                    <div class="page">
                        <form class="pure-form pure-form-aligned">
                            <fieldset>
                                <div id="relays">TODO</div>

                                <!-- removeIf(!light) -->
                                <div id="colors">TODO</div>
                                <div id="cct">TODO</div>
                                <div id="channels">TODO</div>
                                <!-- endRemoveIf(!light) -->

                                <!-- removeIf(!sensor) -->
                                <div id="magnitudes">TODO</div>
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
                                          wrap="off"
                                          readonly></textarea>
                                <div class="pure-u-1-4 pure-u-lg-1-4">
                                    <button type="button" class="pure-button button-dbg-clear pure-u-23-24">
                                        Clear
                                    </button>
                                </div>
                            </div>
                        </fieldset>
                    </div>
                </template>

                <template #general>
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
                                    Hostname may contain only the ASCII letters 'a' through 'z' (in a
                                    case-insensitive
                                    manner), the digits '0' through '9', and the hyphen ('-'). They can neither
                                    start or
                                    end with an hyphen.<br>
                                    For this setting to take effect you should restart the wifi interface by
                                    clicking
                                    the "Reconnect" button.
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
                                    Delay in milliseconds to detect a double click (from 0 to 1000ms).<br>
                                    The lower this number the faster the device will respond to button clicks but
                                    the
                                    harder it will be to get a double click.
                                    Increase this number if you are having trouble to double click the button.
                                    Set this value to 0 to disable double click. You won't be able to set the device
                                    in
                                    AP mode manually but your device will respond immediately to button clicks.<br>
                                    You will have to <strong>reboot the device</strong> after updating for this
                                    setting
                                    to apply.
                                </div>
                            </div>

                            <div class="pure-g module module-alexa">
                                <label class="pure-u-1 pure-u-lg-1-4">Alexa integration</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox" name="alexaEnabled"/>
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
                                    ESPurna will scan for visible WiFi SSIDs and try to connect to networks defined
                                    below in order of <strong>signal strength</strong>, even if multiple AP share
                                    the
                                    same SSID.
                                    When disabled, ESPurna will try to connect to the networks in the same order
                                    they
                                    are listed below.
                                    Disable this option if you are <strong>connecting to a single access
                                    point</strong>
                                    (or router) or to a <strong>hidden SSID</strong>.
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <button class="pure-button button-wifi-scan" type="button">Scan now</button>
                                <div class="pure-u-0 pure-u-lg-1-4 scan loading"></div>
                            </div>

                            <div class="pure-g">
                                <span id="scanResult" class="pure-u-1 terminal" name="scanResult"></span>
                            </div>

                            <legend>Networks</legend>

                            <div id="networks"></div>

                            <button type="button" class="pure-button button-add-network">Add network</button>
                        </fieldset>
                    </div>
                </template>


                <template #admin>
                    <div class="header">
                        <h1>ADMINISTRATION</h1>
                        <h2>Device administration and security settings</h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Settings</label>
                                <div class="pure-u-1-3 pure-u-lg-1-4">
                                    <button type="button" class="pure-button button-settings-backup pure-u-23-24">
                                        Backup
                                    </button>
                                </div>
                                <div class="pure-u-1-3 pure-u-lg-1-4">
                                    <button type="button" class="pure-button button-settings-restore pure-u-23-24">
                                        Restore
                                    </button>
                                </div>
                                <div class="pure-u-1-3 pure-u-lg-1-4">
                                    <button type="button" class="pure-button button-settings-factory pure-u-1">
                                        Factory
                                        Reset
                                    </button>
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Admin password</label>
                                <Inpt name="adminPass1"
                                      class="pure-u-1 pure-u-lg-3-4"
                                      placeholder="New password"
                                      minlength="8"
                                      maxlength="63"
                                      type="password"
                                      action="reboot"
                                      tabindex="11"
                                      autocomplete="false"
                                      spellcheck="false"/>
                                <span class="no-select password-reveal"></span>
                                <div class="pure-u-1 pure-u-lg-1-4"></div>
                                <Inpt name="adminPass2"
                                      class="pure-u-1 pure-u-lg-3-4"
                                      placeholder="Repeat password"
                                      minlength="8"
                                      maxlength="63"
                                      type="password"
                                      action="reboot"
                                      tabindex="12"
                                      autocomplete="false"
                                      spellcheck="false"/>
                                <span class="no-select password-reveal"></span>

                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    The administrator password is used to access this web interface (user 'admin'),
                                    but
                                    also to connect to the device when in AP mode or to flash a new firmware
                                    over-the-air (OTA).<br>
                                    It must be <strong>8..63 characters</strong> (numbers and letters and any of
                                    these
                                    special characters: _,.;:~!?@#$%^&amp;*&lt;&gt;\|(){}[]) and have at least
                                    <strong>one
                                        lowercase</strong> and <strong>one uppercase</strong> or <strong>one
                                    number</strong>.
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">HTTP port</label>
                                <Inpt name="webPort"
                                      class="pure-u-1 pure-u-lg-1-4"
                                      type="text"
                                      action="reboot"
                                      tabindex="13"/>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    This is the port for the web interface and API requests.
                                    If different than 80 (standard HTTP port) you will have to add it explicitly to
                                    your
                                    requests: http://myip:myport/
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Enable WS Auth</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox" name="wsAuth"/>
                                </div>
                            </div>

                            <div class="pure-g module module-api">
                                <label class="pure-u-1 pure-u-lg-1-4">Enable HTTP API</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox" name="apiEnabled"/>
                                </div>
                            </div>

                            <div class="pure-g module module-api">
                                <label class="pure-u-1 pure-u-lg-1-4">Restful API</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox" name="apiRestFul"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    If enabled, API requests to change a status (like a relay) must be done using
                                    PUT.
                                    If disabled you can issue them as GET requests (easier from a browser).
                                </div>
                            </div>

                            <div class="pure-g module module-api">
                                <label class="pure-u-1 pure-u-lg-1-4">Real time API</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox" name="apiRealTime"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    By default, some magnitudes are being preprocessed and filtered to avoid
                                    spurious
                                    values.
                                    If you want to get real-time values (not preprocessed) in the API turn on this
                                    setting.
                                </div>
                            </div>

                            <div class="pure-g module module-api">
                                <label class="pure-u-1 pure-u-lg-1-4">HTTP API Key</label>
                                <Inpt name="apiKey" class="pure-u-3-4 pure-u-lg-1-2" type="text" tabindex="14"/>
                                <div class="pure-u-1-4 pure-u-lg-1-4">
                                    <button type="button" class="pure-button button-apikey pure-u-23-24">Auto
                                    </button>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    This is the key you will have to pass with every HTTP request to the API, either
                                    to
                                    get or write values.
                                    All API calls must contain the <strong>apikey</strong> parameter with the value
                                    above.
                                    To know what APIs are enabled do a call to <strong>/apis</strong>.
                                </div>
                            </div>

                            <div class="pure-g module module-telnet">
                                <label class="pure-u-1 pure-u-lg-1-4">Enable TELNET</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox" name="telnetSTA"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Turn ON to be able to telnet to your device
                                    while connected to your home router.<br>TELNET is always enabled in AP mode.
                                </div>
                            </div>

                            <div class="pure-g module module-telnet">
                                <label class="pure-u-1 pure-u-lg-1-4">TELNET Password</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox" name="telnetAuth"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Request password when starting telnet session
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Heartbeat message</label>
                                <select class="pure-u-1 pure-u-lg-3-4" name="hbMode" tabindex="15">
                                    <option value="0">Disabled</option>
                                    <option value="1">On device startup</option>
                                    <option value="2">Repeat after defined interval</option>
                                    <option value="3">Repeat only device status</option>
                                </select>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Define when heartbeat message will be sent.
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Heartbeat interval</label>
                                <Inpt name="hbInterval" class="pure-u-1 pure-u-lg-1-4" type="number" tabindex="16"/>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    This is the interval in <strong>seconds</strong> how often to send the heartbeat
                                    message.
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Upgrade</label>
                                <Inpt class="pure-u-1-2 pure-u-lg-1-2" name="filename" type="text" readonly/>
                                <div class="pure-u-1-4 pure-u-lg-1-8">
                                    <button type="button" class="pure-button button-upgrade-browse pure-u-23-24">
                                        Browse
                                    </button>
                                </div>
                                <div class="pure-u-1-4 pure-u-lg-1-8">
                                    <button type="button" class="pure-button button-upgrade pure-u-23-24">
                                        Upgrade
                                    </button>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    The device has {{status.free_size}} bytes available for OTA updates. If your image
                                    is larger than this consider
                                    doing a
                                    <a href="https://github.com/xoseperez/espurna/wiki/TwoStepUpdates"
                                       rel="noopener"
                                       target="_blank"><strong>two-step update</strong></a>.
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
                                Configure your NTP (Network Time Protocol) servers and local configuration to keep your
                                device time up to the second for your location.
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
                                <select class="pure-u-1 pure-u-lg-1-4" name="ntpOffset" tabindex="42"></select>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Enable DST</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox" name="ntpDST"/>
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">DST Region</label>
                                <select class="pure-u-1 pure-u-lg-1-4" name="ntpRegion">
                                    <option value="0">Europe</option>
                                    <option value="1">USA</option>
                                </select>
                            </div>
                        </fieldset>
                    </div>
                </template>

                <template #mqtt>
                    <div class="header">
                        <h1>MQTT</h1>
                        <h2>
                            Configure an <strong>MQTT broker</strong> in your network and you will be able to change
                            the
                            switch status via an MQTT message.
                        </h2>
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
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    If left empty the firmware will generate a client ID based on the serial number
                                    of
                                    the chip. You can use the following placeholders: {hostname}, {mac}
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
                                    This is the fingerprint for the SSL certificate of the server.<br>
                                    You can get it using <a href="https://www.grc.com/fingerprints.htm"
                                                            rel="noopener"
                                                            target="_blank">https://www.grc.com/fingerprints.htm</a><br>
                                    or using openssl from a linux box by typing:<br>
                                    <pre>$ openssl s_client -connect &lt;host&gt;:&lt;port&gt; &lt; /dev/null 2&gt;/dev/null | openssl x509 -fingerprint -noout -in /dev/stdin</pre>
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">MQTT Root Topic</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4" name="mqttTopic" type="text" tabindex="31"/>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    This is the root topic for this device. The {hostname} and {mac} placeholders
                                    will
                                    be replaced by the device hostname and MAC address.<br>
                                    - <strong>&lt;root&gt;/relay/#/set</strong> Send a 0 or a 1 as a payload to this
                                    topic to switch it on or off. You can also send a 2 to toggle its current state.
                                    Replace # with the switch ID (starting from 0). If the board has only one switch
                                    it
                                    will be 0.<br>
                                    <!-- removeIf(!light) -->
                                    <span class="module module-color">- <strong>&lt;root&gt;/rgb/set</strong> Set the color using this topic, your can either send an "#RRGGBB" value or "RRR,GGG,BBB" (0-255 each).<br></span>
                                    <span class="module module-color">- <strong>&lt;root&gt;/hsv/set</strong> Set the color using hue (0-360), saturation (0-100) and value (0-100) values, comma separated.<br></span>
                                    <span class="module module-color">- <strong>&lt;root&gt;/brightness/set</strong> Set the brighness (0-255).<br></span>
                                    <span class="module module-color">- <strong>&lt;root&gt;/channel/#/set</strong> Set the value for a single color channel (0-255). Replace # with the channel ID (starting from 0 and up to 4 for RGBWC lights).<br></span>
                                    <span class="module module-color">- <strong>&lt;root&gt;/mired/set</strong> Set the temperature color in mired.<br></span>
                                    <!-- endRemoveIf(!light) -->
                                    - <strong>&lt;root&gt;/status</strong> The device will report a 1 to this topic
                                    every few minutes. Upon MQTT disconnecting this will be set to 0.<br>
                                    - Other values reported (depending on the build) are: <strong>firmware</strong>
                                    and
                                    <strong>version</strong>, <strong>hostname</strong>, <strong>IP</strong>,
                                    <strong>MAC</strong>,
                                    signal strenth (<strong>RSSI</strong>), <strong>uptime</strong> (in seconds),
                                    <strong>free heap</strong> and <strong>power supply</strong>.
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
                                    All messages (except the device status) will be included in a JSON payload along
                                    with the timestamp and hostname
                                    and sent under the <strong>&lt;root&gt;/data</strong> topic.<br>
                                    Messages will be queued and sent after 100ms, so different messages could be
                                    merged
                                    into a single payload.<br>
                                    Subscriptions will still be done to single topics.
                                </div>
                            </div>
                        </fieldset>
                    </div>
                </template>

                <!-- removeIf(!thermostat) -->
                <template #thermostat>
                    <div class="header">
                        <h1>THERMOSTAT</h1>
                        <h2>Thermostat configuration</h2>
                    </div>

                    <div class="page">
                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4">Enable Thermostat</label>
                            <div class="pure-u-1 pure-u-lg-1-4">
                                <Inpt type="checkbox"
                                      name="thermostatEnabled"
                                      tabindex="30"/>
                            </div>
                        </div>

                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4">Thermostat Mode</label>
                            <div class="pure-u-1 pure-u-lg-1-4">
                                <Inpt type="checkbox"
                                      name="thermostatMode"
                                      tabindex="30"/>
                            </div>
                        </div>

                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4" for="thermostatOperationMode">Operation
                                mode</label>
                            <Inpt class="pure-u-1 pure-u-lg-1-4" name="thermostatOperationMode" type="text"
                                  readonly/>
                        </div>

                        <fieldset>
                            <legend>Temperature range</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Max (<span class="tmpUnit"></span>)</label>
                                <Inpt id="tempRangeMaxInput"
                                      class="pure-u-1 pure-u-lg-1-4"
                                      name="tempRangeMax"
                                      type="number"
                                      min="1"
                                      max="100"
                                      tabindex="32"
                                      data="20"
                                      onchange="checkTempRangeMax()"/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Min (<span class="tmpUnit"></span>)</label>
                                <Inpt id="tempRangeMinInput"
                                      class="pure-u-1 pure-u-lg-1-4"
                                      name="tempRangeMin"
                                      type="number"
                                      min="0"
                                      max="99"
                                      tabindex="31"
                                      data="10"
                                      onchange="checkTempRangeMin()"/>
                            </div>
                        </fieldset>

                        <fieldset>
                            <legend>Remote sensor</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="remoteSensorName">Remote sensor
                                    name</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4" name="remoteSensorName" type="text"/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="remoteTmp">Remote temperature (<span
                                        class="tmpUnit"></span>)</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4" name="remoteTmp" type="text" readonly/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="remoteTempMaxWait">Remote temperature
                                    waiting
                                    (s)</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4"
                                      name="remoteTempMaxWait"
                                      type="number"
                                      min="0"
                                      max="1800"
                                      tabindex="33"
                                      data="120"/>
                            </div>
                        </fieldset>

                        <fieldset>
                            <legend>Operation mode</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="maxOnTime">Max heating time (m)</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4"
                                      name="maxOnTime"
                                      type="number"
                                      min="0"
                                      max="180"
                                      tabindex="34"
                                      data="30"/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="minOffTime">Min rest time (m)</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4"
                                      name="minOffTime"
                                      type="number"
                                      min="0"
                                      max="60"
                                      tabindex="35"
                                      data="10"/>
                            </div>
                        </fieldset>

                        <fieldset>
                            <legend>Autonomous mode</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="aloneOnTime">Heating time (m)</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4"
                                      name="aloneOnTime"
                                      type="number"
                                      min="0"
                                      max="180"
                                      tabindex="36"
                                      data="5"/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="aloneOffTime">Rest time (m)</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4"
                                      name="aloneOffTime"
                                      type="number"
                                      min="0"
                                      max="180"
                                      tabindex="37"
                                      data="55"/>
                            </div>
                        </fieldset>

                        <fieldset>
                            <legend>Time worked</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="burnToday">Today</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4" type="text" name="burnToday" readonly/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="burnYesterday">Yesterday</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4" type="text" name="burnYesterday" readonly/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="burnThisMonth">Current month</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4" type="text" name="burnThisMonth" readonly/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="burnPrevMonth">Previous month</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4" type="text" name="burnPrevMonth" readonly/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="burnTotal">Total</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4" type="text" name="burnTotal" readonly/>
                            </div>

                            <div class="pure-g">
                                <button type="button" class="pure-button button-thermostat-reset-counters">
                                    Reset
                                    counters
                                </button>
                            </div>
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!thermostat) -->

                <!-- removeIf(!led) -->
                <template #led>
                    <div class="header">
                        <h1>LED</h1>
                        <h2>Notification LED configuration</h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <div class="pure-g module module-led">
                                <div class="pure-u-0 pure-u-lg-1-4">Modes</div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    <li>
                                        <strong>WiFi status</strong> will blink at 1Hz when trying to connect. If
                                        successfully connected it will briefly blink every 5 seconds if in STA mode
                                        or
                                        every second if in AP mode.
                                    </li>
                                    <li>
                                        <strong>Follow switch</strong> will force the LED to follow the status of a
                                        given switch (you must define which switch to follow in the side field).
                                    </li>
                                    <li>
                                        <strong>Inverse switch</strong> will force the LED to not-follow the status
                                        of a
                                        given switch (you must define which switch to follow in the side field).
                                    </li>
                                    <li>
                                        <strong>Find me</strong> will turn the LED ON when all switches are OFF.
                                        This is
                                        meant to locate switches at night.
                                    </li>
                                    <li>
                                        <strong>Find me &amp; WiFi</strong> will follow the WiFi status but will
                                        stay
                                        mostly on when switches are OFF, and mostly OFF when any of them is ON.
                                    </li>
                                    <li>
                                        <strong>Switches status</strong> will turn the LED ON whenever any switch is
                                        ON,
                                        and OFF otherwise. This is global status notification.
                                    </li>
                                    <li>
                                        <strong>Switches status &amp; WiFi</strong> will follow the WiFi status but
                                        will
                                        stay mostly off when switches are OFF, and mostly ON when any of them is ON.
                                    </li>
                                    <li>
                                        <strong>MQTT managed</strong> will let you manage the LED status via MQTT by
                                        sending a message to "&lt;base_topic&gt;/led/0/set" with a payload of 0, 1
                                        or 2
                                        (to toggle it).
                                    </li>
                                    <li>
                                        <strong>Always ON</strong> and <strong>Always OFF</strong> modes are
                                        self-explanatory.
                                    </li>
                                </div>
                            </div>

                            <div id="ledConfig"></div>
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!led) -->

                <!-- removeIf(!light) -->
                <template #color>
                    <div class="header">
                        <h1>LIGHTS</h1>
                        <h2>Lights configuration</h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Use color</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="useColor"
                                          action="reload"
                                          tabindex="8"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Use the first three channels as RGB channels.
                                    This will also enable the color picker in the web UI. Will only work if the
                                    device
                                    has at least 3 dimmable channels.<br>Reload the page to update the web
                                    interface.
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Use RGB picker</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="useRGB"
                                          action="reload"
                                          tabindex="11"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Use RGB color picker if enabled (plus
                                    brightness), otherwise use HSV (hue-saturation-value) style
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Use white channel</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="useWhite"
                                          action="reload"
                                          tabindex="9"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    For 2 channels warm white and cold white lights
                                    or color lights to use forth dimmable channel as (cold) white light calculated
                                    out
                                    of the RGB values.<br>Will only work if the device has at least 4 dimmable
                                    channels.<br>Enabling this will render useless the "Channel 4" slider in the
                                    status
                                    page.<br>Reload the page to update the web interface.
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Use white color temperature</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="useCCT"
                                          action="reload"
                                          tabindex="10"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Use a dimmable channel as warm white light and
                                    another dimmable channel as cold white light.<br>On devices with two dimmable
                                    channels the first use used for warm white light and the second for cold white
                                    light.<br>On color lights the fifth use used for warm white light and the fourth
                                    for cold white light.<br>Will only work if the device has exactly 2 dimmable
                                    channels or at least 5 dimmable channels and "white channel" above is also
                                    ON.<br>Enabling
                                    this will render useless the "Channel 5" slider in the status page.<br>Reload
                                    the
                                    page to update the web interface.
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Use gamma correction</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="useGamma"
                                          tabindex="11"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Use gamma correction for RGB channels.<br>Will
                                    only work if "use colorpicker" above is also ON.
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Use CSS style</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox" name="useCSS" tabindex="12"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Use CSS style to report colors to MQTT and REST
                                    API. <br>Red will be reported as "#FF0000" if ON, otherwise "255,0,0"
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Color transitions</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="useTransitions"
                                          tabindex="13"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    If enabled color changes will be smoothed.
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Transition time</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt class="pure-u-1"
                                          type="number"
                                          name="lightTime"
                                          min="10"
                                          max="5000"
                                          tabindex="14"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Time in millisecons to transition from one
                                    color to another.
                                </div>
                            </div>

                            <div class="pure-g">
                                <div class="pure-u-1 pure-u-lg-1-4"><label>MQTT group</label></div>
                                <div class="pure-u-1 pure-u-lg-3-4">
                                    <Inpt name="mqttGroupColor"
                                          class="pure-u-1"
                                          tabindex="15"
                                          action="reconnect"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">Sync color between different lights.</div>
                            </div>
                        </fieldset>
                        <fieldset>
                            <legend>SCHEDULE</legend>
                            <h2>Turn switches ON and OFF based on the current time.</h2>
                            <div id="color-schedules"></div>

                            <button type="button" class="pure-button button-add-light-schedule module module-color">
                                Add channel schedule
                            </button>
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!light) -->

                <!-- removeIf(!rfm69) -->
                <template #mapping>
                    <div class="header">
                        <h1>MAPPING</h1>
                        <h2>
                            Configure the map between nodeID/key and MQTT topic. Messages from the given nodeID with
                            the
                            given key will be forwarded to the specified topic.
                            You can also configure a default topic using {nodeid} and {key} as placeholders, if the
                            default topic is empty messages without defined map will be discarded.
                        </h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <legend>Default topic</legend>

                            <div class="pure-g">
                                <Inpt name="rfm69Topic"
                                      type="text"
                                      class="pure-u-23-24"
                                      value=""
                                      size="8"
                                      tabindex="41"
                                      placeholder="Default MQTT Topic (use {nodeid} and {key} as placeholders)"/>
                            </div>

                            <legend>Specific topics</legend>

                            <div id="mapping"></div>

                            <button type="button" class="pure-button button-add-mapping">Add</button>
                        </fieldset>
                        <fieldset>
                            <legend>Messages</legend>
                            <h2>
                                Messages being received. Previous messages are not displayed.
                                You have to keep the page open in order to keep receiving them.
                                You can filter/unfilter by clicking on the values.
                                Left click on a value to show only rows that match that value, middle click to show all
                                rows
                                but those matching that value.
                                Filtered colums have red headers.
                            </h2>

                            <table id="packets" class="display" cellspacing="0">
                                <thead>
                                <tr>
                                    <th>Timestamp</th>
                                    <th>SenderID</th>
                                    <th>PacketID</th>
                                    <th>TargetID</th>
                                    <th>Key</th>
                                    <th>Value</th>
                                    <th>RSSI</th>
                                    <th>Duplicates</th>
                                    <th>Missing</th>
                                </tr>
                                </thead>
                                <tbody>
                                </tbody>
                            </table>

                            <button type="button" class="pure-button button-clear-filters">Clear filters</button>
                            <button type="button" class="pure-button button-clear-messages">Clear messages</button>
                            <button type="button" class="pure-button button-clear-counts">Clear counts</button>
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!rfm69) -->

                <!-- removeIf(!rfbridge) -->
                <template #rfb>
                    <div class="header">
                        <h1>RADIO FREQUENCY</h1>
                        <h2>
                            Sonoff 433 RF Bridge &amp; RF Link Configuration<br><br>
                            This page allows you to configure the RF codes for the Sonoff RFBridge 433 and also for
                            a
                            basic RF receiver.<br><br>
                            To learn a new code click <strong>LEARN</strong> (the Sonoff RFBridge will beep) then
                            press
                            a button on the remote, the new code should show up (and the RFBridge will double beep).
                            If
                            the device double beeps but the code does not update it has not been properly learnt.
                            Keep
                            trying.<br><br>
                            Modify or create new codes manually and then click <strong>SAVE</strong> to store them
                            in
                            the device memory. If your controlled device uses the same code to switch ON and OFF,
                            learn
                            the code with the ON button and copy paste it to the OFF input box, then click SAVE on
                            the
                            last one to store the value.<br><br>
                            Delete any code clicking the <strong>FORGET</strong> button.
                            <br><br>You can also specify any RAW code. For reference see <a rel="noopener"
                                                                                            target="_blank"
                                                                                            href="https://github.com/Portisch/RF-Bridge-EFM8BB1/wiki/Commands">possible
                            commands for Sonoff RF Bridge EFM8BB1</a> (original firmware supports codes from
                            <strong>0xA0</strong>
                            to <strong>0xA5</strong>).
                        </h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <legend>RF Codes</legend>

                            <div id="rfbNodes"></div>

                            <legend>Settings</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Repeats</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt class="pure-u-1 pure-u-lg-23-24"
                                          name="rfbRepeat"
                                          type="number"
                                          min="1"
                                          tabindex="0"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">Number of times to repeat transmission
                                </div>
                            </div>

                            <div class="pure-g module module-rfbdirect">
                                <legend>GPIO</legend>

                                <div class="pure-u-1 pure-u-lg-1 hint">
                                    Pins used by the receiver (RX) and transmitter
                                    (TX). Set to <strong>NONE</strong> to disable
                                </div>

                                <label class="pure-u-1 pure-u-lg-1-4">RX Pin</label>
                                <select class="pure-u-1 pure-u-lg-1-4 gpio-select" name="rfbRX"></select>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>

                                <label class="pure-u-1 pure-u-lg-1-4">TX Pin</label>
                                <select class="pure-u-1 pure-u-lg-1-4 gpio-select" name="rfbTX"></select>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                            </div>
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!rfbridge) -->


                <!-- removeIf(!sensor) -->
                <template #sns>
                    <div class="header">
                        <h1>SENSOR CONFIGURATION</h1>
                        <h2>
                            Configure and calibrate your device sensors.
                        </h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <legend>General</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Read interval</label>
                                <select class="pure-u-1 pure-u-lg-1-4" name="snsRead">
                                    <option value="1">1 second</option>
                                    <option value="6">6 seconds</option>
                                    <option value="10">10 seconds</option>
                                    <option value="15">15 seconds</option>
                                    <option value="30">30 seconds</option>
                                    <option value="60">1 minute</option>
                                    <option value="300">5 minutes</option>
                                    <option value="600">10 minutes</option>
                                    <option value="900">15 minutes</option>
                                    <option value="1800">30 minutes</option>
                                    <option value="3600">60 minutes</option>
                                </select>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Select the interval between readings. These will be filtered and averaged for
                                    the
                                    report.
                                    Please mind some sensors do not have fast refresh intervals. Check the sensor
                                    datasheet to know the minimum read interval.
                                    The default and recommended value is 6 seconds.
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Report every</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt name="snsReport"
                                          class="pure-u-1"
                                          type="number"
                                          min="1"
                                          step="1"
                                          max="60"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Select the number of readings to average and report
                                </div>
                            </div>

                            <div class="pure-g module module-pwr">
                                <label class="pure-u-1 pure-u-lg-1-4">Save every</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt name="snsSave"
                                          class="pure-u-1"
                                          type="number"
                                          min="0"
                                          step="1"
                                          max="200"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Save aggregated data to EEPROM after these many reports. At the moment this only
                                    applies to total energy readings.
                                    Please mind: saving data to EEPROM too often will wear out the flash memory
                                    quickly.
                                    Set it to 0 to disable this feature (default value).
                                </div>
                            </div>

                            <div class="pure-g module module-pwr">
                                <label class="pure-u-1 pure-u-lg-1-4">Power units</label>
                                <select name="pwrUnits" tabindex="16" class="pure-u-1 pure-u-lg-1-4">
                                    <option value="0">Watts (W)</option>
                                    <option value="1">Kilowatts (kW)</option>
                                </select>
                            </div>

                            <div class="pure-g module module-pwr">
                                <label class="pure-u-1 pure-u-lg-1-4">Energy units</label>
                                <select name="eneUnits" tabindex="16" class="pure-u-1 pure-u-lg-1-4">
                                    <option value="0">Joules (J)</option>
                                    <option value="1">Kilowatts·hour (kWh)</option>
                                </select>
                            </div>

                            <div class="pure-g module module-temperature">
                                <label class="pure-u-1 pure-u-lg-1-4">Temperature units</label>
                                <select name="tmpUnits" tabindex="16" class="pure-u-1 pure-u-lg-1-4">
                                    <option value="0">Celsius (&deg;C)</option>
                                    <option value="1">Fahrenheit (&deg;F)</option>
                                </select>
                            </div>

                            <div class="pure-g module module-temperature">
                                <label class="pure-u-1 pure-u-lg-1-4">Temperature correction</label>
                                <Inpt name="tmpCorrection"
                                      class="pure-u-1 pure-u-lg-1-4"
                                      type="number"
                                      action="reboot"
                                      min="-100"
                                      step="0.1"
                                      max="100"
                                      tabindex="18"/>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Temperature correction value is added to the measured value which may be
                                    inaccurate
                                    due to many factors. The value can be negative.
                                </div>
                            </div>

                            <div class="pure-g module module-humidity">
                                <label class="pure-u-1 pure-u-lg-1-4">Humidity correction</label>
                                <Inpt name="humCorrection"
                                      class="pure-u-1 pure-u-lg-1-4"
                                      type="number"
                                      action="reboot"
                                      min="-100"
                                      step="0.1"
                                      max="100"
                                      tabindex="18"/>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Humidity correction value is added to the measured value which may be inaccurate
                                    due
                                    to many factors. The value can be negative.
                                </div>
                            </div>

                            <div class="pure-g module module-mics">
                                <label class="pure-u-1 pure-u-lg-1-4">Calibrate gas sensor</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="snsResetCalibration"
                                          tabindex="55"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Move this switch to ON and press "Save" to
                                    reset gas sensor calibration. Check the sensor datasheet for calibration
                                    conditions.
                                </div>
                            </div>

                            <legend class="module module-hlw module-cse module-emon">Energy monitor</legend>

                            <div class="pure-g module module-emon">
                                <label class="pure-u-1 pure-u-lg-1-4">Voltage</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4" name="pwrVoltage" type="text" tabindex="51"/>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">Mains voltage in your system (in V).</div>
                            </div>

                            <div class="pure-g module module-hlw module-cse">
                                <label class="pure-u-1 pure-u-lg-1-4">Expected Current</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4 pwrExpected"
                                      name="pwrExpectedC"
                                      type="text"
                                      tabindex="52"
                                      placeholder="0"/>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    In Amperes (A). If you are using a pure
                                    resistive load like a bulb, this will be the ratio between the two previous
                                    values,
                                    i.e. power / voltage. You can also use a current clamp around one of the power
                                    wires
                                    to get this value.
                                </div>
                            </div>

                            <div class="pure-g module module-hlw module-cse">
                                <label class="pure-u-1 pure-u-lg-1-4">Expected Voltage</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4 pwrExpected"
                                      name="pwrExpectedV"
                                      type="text"
                                      tabindex="53"
                                      placeholder="0"/>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    In Volts (V). Enter your the nominal AC voltage
                                    for your household or facility, or use multimeter to get this value.
                                </div>
                            </div>

                            <div class="pure-g module module-hlw module-cse module-emon">
                                <label class="pure-u-1 pure-u-lg-1-4">Expected Power</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4 pwrExpected"
                                      name="pwrExpectedP"
                                      type="text"
                                      tabindex="54"
                                      placeholder="0"/>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    In Watts (W). Calibrate your sensor connecting
                                    a pure resistive load (like a bulb) and enter here its nominal power or use a
                                    multimeter.
                                </div>
                            </div>

                            <div class="pure-g module module-pm">
                                <label class="pure-u-1 pure-u-lg-1-4">Energy Ratio</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4"
                                      name="pwrRatioE"
                                      type="text"
                                      tabindex="55"
                                      placeholder="0"/>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">Energy ratio in pulses/kWh.</div>
                            </div>

                            <div class="pure-g module module-hlw module-cse module-emon">
                                <label class="pure-u-1 pure-u-lg-1-4">Reset calibration</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="pwrResetCalibration"
                                          tabindex="56"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Move this switch to ON and press "Save" to
                                    revert to factory calibration values.
                                </div>
                            </div>

                            <div class="pure-g module module-hlw module-cse module-emon module-pm">
                                <label class="pure-u-1 pure-u-lg-1-4">Reset energy</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="pwrResetE"
                                          tabindex="57"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Move this switch to ON and press "Save" to set
                                    energy count to 0.
                                </div>
                            </div>
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!sensor) -->

                <template #relays>
                    <div class="header">
                        <h1>SWITCHES</h1>
                        <h2>Switch / relay configuration</h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <legend class="module module-multirelay">General</legend>

                            <div class="pure-g module module-multirelay">
                                <label class="pure-u-1 pure-u-lg-1-4">Switch sync mode</label>
                                <select name="relaySync" class="pure-u-1 pure-u-lg-3-4" tabindex="3">
                                    <option value="0">No synchronisation</option>
                                    <option value="1">Zero or one switches active</option>
                                    <option value="2">One and just one switch active</option>
                                    <option value="3">All synchronised</option>
                                    <option value="4">Switch #0 controls other switches</option>
                                </select>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Define how the different switches should be
                                    synchronized.
                                </div>
                            </div>

                            <div id="relayConfig"></div>
                        </fieldset>
                        <fieldset>
                            <legend>SCHEDULE</legend>
                            <h2>Turn switches ON and OFF based on the current time.</h2>
                            <div id="schedules"></div>

                            <button type="button"
                                    class="pure-button button-add-switch-schedule module module-relay">
                                Add switch schedule
                            </button>
                        </fieldset>
                    </div>
                </template>

                <!-- removeIf(!lightfox) -->
                <template #lightfox>
                    <div class="header">
                        <h1>LIGHTFOX RF</h1>
                        <h2>
                            LightFox RF configuration<br><br>
                            This page allows you to control LightFox RF receiver options.<br><br>
                            To learn a new code click <strong>LEARN</strong>, wait for 3 seconds then press a button
                            on
                            the remote, one of the relays will toggle. If no device relay toggles the code has not
                            been
                            properly learnt. Keep trying.<br><br>
                            Delete all the codes by clicking the <strong>CLEAR</strong> button and wait for 10
                            seconds.<br><br>
                            You can also specify which RF button controls which relay using controls below.
                        </h2>
                    </div>

                    <div class="page">
                        <div class="pure-g">
                            <label class="pure-u-1 pure-u-lg-1-4">RF Actions</label>
                            <div class="pure-u-1-2 pure-u-lg-3-8">
                                <button type="button" class="pure-button button-lightfox-learn pure-u-23-24">
                                    Learn
                                </button>
                            </div>
                            <div class="pure-u-1-2 pure-u-lg-3-8">
                                <button type="button" class="pure-button button-lightfox-clear pure-u-1">
                                    Clear
                                </button>
                            </div>
                        </div>

                        <fieldset id="lightfoxNodes"></fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!lightfox) -->


                <!-- removeIf(!dcz) -->
                <template #dcz>
                    <div class="header">
                        <h1>DOMOTICZ</h1>
                        <h2>
                            Configure the connection to your Domoticz server.
                        </h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <legend>General</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Enable Domoticz</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="dczEnabled"
                                          tabindex="30"/>
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Domoticz IN Topic</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4" name="dczTopicIn" type="text" tabindex="31"/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Domoticz OUT Topic</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4"
                                      name="dczTopicOut"
                                      type="text"
                                      action="reconnect"
                                      tabindex="32"/>
                            </div>

                            <legend>Sensors &amp; actuators</legend>

                            <div class="pure-g">
                                <div class="pure-u-1 hint">
                                    Set IDX to 0 to disable notifications from that component.
                                </div>
                            </div>

                            <div id="dczRelays"></div>

                            <!-- removeIf(!sensor) -->
                            <div id="dczMagnitudes"></div>
                            <!-- endRemoveIf(!sensor) -->
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!dcz) -->

                <!-- removeIf(!ha) -->
                <template #ha>
                    <div class="header">
                        <h1>HOME ASSISTANT</h1>
                        <h2>
                            Add this device to your Home Assistant.
                        </h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <legend>Discover</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Discover</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="haEnabled"
                                          tabindex="14"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    Home Assistant auto-discovery feature. Enable and save to add the device to your
                                    HA
                                    console.
                                    <!-- removeIf(!light) -->
                                    When using a colour light you might want to disable CSS style so Home Assistant
                                    can
                                    parse the color.
                                    <!-- endRemoveIf(!light) -->
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Prefix</label>
                                <Inpt class="pure-u-1 pure-u-lg-1-4" name="haPrefix" type="text" tabindex="15"/>
                            </div>

                            <legend>Configuration</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Configuration</label>
                                <div class="pure-u-1-4 pure-u-lg-3-4">
                                    <button type="button" class="pure-button button-ha-config pure-u-1-3">Show
                                    </button>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    These are the settings you should copy to your Home Assistant
                                    "configuration.yaml"
                                    file.
                                    If any of the sections below (switch, light, sensor) already exists, do not
                                    duplicate it,
                                    simply copy the contents of the section below the ones already present.
                                </div>
                            </div>
                            <div class="pure-g">
                                <textarea id="haConfig"
                                          class="pure-u-1 terminal"
                                          name="haConfig"
                                          wrap="off"
                                          readonly></textarea>
                            </div>
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!ha) -->

                <!-- removeIf(!thingspeak) -->
                <template #thingspeak>
                    <div class="header">
                        <h1>THINGSPEAK</h1>
                        <h2>
                            Send your sensors data to Thingspeak.
                        </h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <legend>General</legend>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Enable Thingspeak</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="tspkEnabled"
                                          tabindex="30"/>
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Clear cache</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="tspkClear"
                                          tabindex="31"/>
                                </div>
                                <div class="pure-u-0 pure-u-lg-1-2"></div>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">
                                    With every POST to thinkspeak.com only enqueued fields are sent.
                                    If you select to clear the cache after every sending this will result in only
                                    those
                                    fields that have changed will be posted.
                                    If you want all fields to be sent with every POST do not clear the cache.
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Thingspeak API Key</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4" name="tspkKey" type="text" tabindex="32"/>
                            </div>

                            <legend>Sensors &amp; actuators</legend>

                            <div class="pure-g">
                                <div class="pure-u-1 hint">
                                    Enter the field number to send each data to, 0 disable
                                    notifications from that component.
                                </div>
                            </div>

                            <div id="tspkRelays"></div>

                            <!-- removeIf(!sensor) -->
                            <div id="tspkMagnitudes"></div>
                            <!-- endRemoveIf(!sensor) -->
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!thingspeak) -->

                <!-- removeIf(!idb) -->
                <template #idb>
                    <div class="header">
                        <h1>INFLUXDB</h1>
                        <h2>
                            Configure the connection to your InfluxDB server. Leave the host field empty to
                            disable
                            InfluxDB connection.
                        </h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Enable InfluxDB</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox"
                                          name="idbEnabled"
                                          tabindex="40"/>
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Host</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4" name="idbHost" type="text" tabindex="41"/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Port</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4" name="idbPort" type="text" tabindex="42"/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Database</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4" name="idbDatabase" type="text"
                                      tabindex="43"/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Username</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4"
                                      name="idbUsername"
                                      type="text"
                                      tabindex="44"
                                      autocomplete="off"/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Password</label>
                                <Inpt class="pure-u-1 pure-u-lg-3-4"
                                      name="idbPassword"
                                      type="password"
                                      tabindex="45"
                                      autocomplete="new-password"
                                      spellcheck="false"/>
                                <span class="no-select password-reveal"></span>
                            </div>
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!idb) -->

                <!-- removeIf(!nofuss) -->
                <template #nofuss>
                    <div class="header">
                        <h1>NoFUSS</h1>
                        <h2>
                            Automatically upgrade the firmware (see <a
                                href="https://bitbucket.org/xoseperez/nofuss">xoseperez/nofuss</a>
                            for details)
                        </h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Enable</label>
                                <div class="pure-u-1 pure-u-lg-1-4">
                                    <Inpt type="checkbox" name="nofussEnabled"/>
                                </div>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4">Server</label>
                                <Inpt name="nofussServer" class="pure-u-1 pure-u-lg-3-4" type="text" tabindex="15"/>
                                <div class="pure-u-0 pure-u-lg-1-4"></div>
                                <div class="pure-u-1 pure-u-lg-3-4 hint">Address of the NoFUSS server</div>
                            </div>
                        </fieldset>
                    </div>
                </template>
                <!-- endRemoveIf(!nofuss) -->
            </Menu>

            <!-- Templates -->

            <div id="ledConfigTemplate" class="template">
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">LED #<span class="id"></span> mode</label>
                    <select name="ledMode" class="pure-u-1-4">
                        <option value="1">WiFi status</option>
                        <option value="2">Follow switch #</option>
                        <option value="3">Inverse switch #</option>
                        <option value="4">Find me</option>
                        <option value="5">Find me &amp; WiFi</option>
                        <option value="8">Switches status</option>
                        <option value="9">Switches &amp; WiFi</option>
                        <option value="0">MQTT managed</option>
                        <option value="6">Always ON</option>
                        <option value="7">Always OFF</option>
                    </select>
                    &nbsp;
                    <div class="pure-u-1-4">
                        <Inpt class="pure-u-23-24" name="ledRelay" type="number" min="0" data="0"/>
                    </div>
                </div>
            </div>

            <!-- removeIf(!rfbridge) -->
            <div id="rfbNodeTemplate" class="template">
                <legend>Switch #<span></span></legend>

                <div class="pure-g">
                    <div class="pure-u-1 pure-u-lg-1-4"><label>Switch ON</label></div>
                    <Inpt class="pure-u-1 pure-u-lg-1-3"
                          type="text"
                          maxlength="116"
                          name="rfbcode"
                          data-id="1"
                          data-status="1"/>
                    <div class="pure-u-1-3 pure-u-lg-1-8">
                        <button type="button" class="pure-u-23-24 pure-button button-rfb-learn">LEARN</button>
                    </div>
                    <div class="pure-u-1-3 pure-u-lg-1-8">
                        <button type="button" class="pure-u-23-24 pure-button button-rfb-send">SAVE</button>
                    </div>
                    <div class="pure-u-1-3 pure-u-lg-1-8">
                        <button type="button" class="pure-u-23-24 pure-button button-rfb-forget">FORGET</button>
                    </div>
                </div>

                <div class="pure-g">
                    <div class="pure-u-1 pure-u-lg-1-4"><label>Switch OFF</label></div>
                    <Inpt class="pure-u-1 pure-u-lg-1-3"
                          type="text"
                          maxlength="116"
                          name="rfbcode"
                          data-id="1"
                          data-status="0"/>
                    <div class="pure-u-1-3 pure-u-lg-1-8">
                        <button type="button" class="pure-u-23-24 pure-button button-rfb-learn">LEARN</button>
                    </div>
                    <div class="pure-u-1-3 pure-u-lg-1-8">
                        <button type="button" class="pure-u-23-24 pure-button button-rfb-send">SAVE</button>
                    </div>
                    <div class="pure-u-1-3 pure-u-lg-1-8">
                        <button type="button" class="pure-u-23-24 pure-button button-rfb-forget">FORGET</button>
                    </div>
                </div>
            </div>
            <!-- endRemoveIf(!rfbridge) -->

            <!-- removeIf(!lightfox) -->
            <div id="lightfoxNodeTemplate" class="template">
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Button #<span></span></label>
                    <select class="pure-u-1 pure-u-lg-3-4" name="btnRelay" action="reboot"></select>
                </div>
            </div>
            <!-- endRemoveIf(!lightfox) -->

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
                        <button type="button" class="pure-button button-more-network pure-u-1">...</button>
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
                          value="255.255.255.0"
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
                    <button class="pure-button button-del-network more" type="button">Delete network</button>
                </div>
            </div>

            <div id="scheduleTemplate" class="template">
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">When time is</label>
                    <div class="pure-u-1-4 pure-u-lg-1-5">
                        <Inpt class="pure-u-2-3"
                              name="schHour"
                              type="number"
                              min="0"
                              step="1"
                              max="23"
                              value="0"/>
                        <div class="pure-u-1-4 hint center">&nbsp;h</div>
                    </div>
                    <div class="pure-u-1-4 pure-u-lg-1-5">
                        <Inpt class="pure-u-2-3"
                              name="schMinute"
                              type="number"
                              min="0"
                              step="1"
                              max="59"
                              value="0"/>
                        <div class="pure-u-1-4 hint center">&nbsp;m</div>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-3"></div>

                    <label class="pure-u-1 pure-u-lg-1-4">Use UTC time</label>
                    <div class="pure-u-1 pure-u-lg-3-4">
                        <Inpt type="checkbox" name="schUTC"/>
                    </div>

                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <label class="pure-u-1 pure-u-lg-1-4">And weekday is one of</label>
                    <div class="pure-u-2-5 pure-u-lg-1-5">
                        <Inpt class="pure-u-23-24 pure-u-lg-23-24"
                              name="schWDs"
                              type="text"
                              maxlength="15"
                              tabindex="0"
                              value="1,2,3,4,5,6,7"/>
                    </div>
                    <div class="pure-u-3-5 pure-u-lg-1-2 hint center">&nbsp;1 for Monday, 2 for Tuesday...</div>

                    <div id="schActionDiv" class="pure-u-1">
                    </div>

                    <label class="pure-u-1 pure-u-lg-1-4">Enabled</label>
                    <div class="pure-u-1 pure-u-lg-3-4">
                        <Inpt type="checkbox" name="schEnabled"/>
                    </div>

                    <div class="pure-u-1 pure-u-lg-1-2"></div>
                    <button class="pure-button button-del-schedule" type="button">Delete schedule</button>
                </div>
            </div>

            <div id="switchActionTemplate" class="template">
                <label class="pure-u-1 pure-u-lg-1-4">Action</label>
                <div class="pure-u-1 pure-u-lg-1-5">
                    <Inpt class="pure-u-1 pure-u-lg-23-24" name="schAction"
                          :options="['Turn Off', 'Turn On', 'Toggle']"/>
                </div>
                <Inpt class="pure-u-1 pure-u-lg-1-5 isrelay" name="schSwitch" :options="relayOptions"/>
                <Inpt type="hidden" name="schType" value="1"/>
            </div>

            <!-- removeIf(!light) -->
            <div id="lightActionTemplate" class="template">
                <label class="pure-u-1 pure-u-lg-1-4">Brightness</label>
                <div class="pure-u-1 pure-u-lg-1-5">
                    <Inpt class="pure-u-2-3"
                          name="schAction"
                          type="number"
                          min="0"
                          step="1"
                          max="255"
                          value="0"/>
                </div>
                <Inpt class="pure-u-1 pure-u-lg-1-5 islight" name="schSwitch" :options="lightOptions"></Inpt>
                <Inpt type="hidden" name="schType" value="2"/>
            </div>
            <!-- endRemoveIf(!light) -->

            <div id="relayTemplate" class="template">
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Switch #<span class="id"></span></label>
                    <div>
                        <Inpt name="relay" type="checkbox" on="ON" off="OFF"/>
                    </div>
                </div>
            </div>

            <div id="relayConfigTemplate" class="template">
                <legend>Switch #<span class="id"></span> (<span class="gpio"></span>)</legend>
                <div class="pure-g">
                    <div class="pure-u-1 pure-u-lg-1-4"><label>Boot mode</label></div>
                    <select class="pure-u-1 pure-u-lg-3-4" name="relayBoot">
                        <option value="0">Always OFF</option>
                        <option value="1">Always ON</option>
                        <option value="2">Same as before</option>
                        <option value="3">Toggle before</option>
                        <option value="4">Locked OFF</option>
                        <option value="5">Locked ON</option>
                    </select>
                </div>
                <div class="pure-g">
                    <div class="pure-u-1 pure-u-lg-1-4"><label>Pulse mode</label></div>
                    <select class="pure-u-1 pure-u-lg-3-4" name="relayPulse">
                        <option value="0">Don't pulse</option>
                        <option value="1">Normally OFF</option>
                        <option value="2">Normally ON</option>
                    </select>
                </div>
                <div class="pure-g">
                    <div class="pure-u-1 pure-u-lg-1-4"><label>Pulse time (s)</label></div>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt name="relayTime"
                              class="pure-u-1"
                              type="number"
                              min="0"
                              step="0.1"
                              max="3600"/>
                    </div>
                </div>
                <div class="pure-g module module-mqtt">
                    <div class="pure-u-1 pure-u-lg-1-4"><label>MQTT group</label></div>
                    <div class="pure-u-1 pure-u-lg-3-4">
                        <Inpt name="mqttGroup"
                              class="pure-u-1"
                              tabindex="0"
                              data="0"
                              action="reconnect"/>
                    </div>
                </div>
                <div class="pure-g module module-mqtt">
                    <div class="pure-u-1 pure-u-lg-1-4"><label>MQTT group sync</label></div>
                    <select class="pure-u-1 pure-u-lg-3-4" name="mqttGroupSync">
                        <option value="0">Same</option>
                        <option value="1">Inverse</option>
                        <option value="2">Receive Only</option>
                    </select>
                </div>
                <div class="pure-g module module-mqtt">
                    <div class="pure-u-1 pure-u-lg-1-4"><label>On MQTT disconnect</label></div>
                    <select class="pure-u-1 pure-u-lg-3-4" name="relayOnDisc">
                        <option value="0">Don't change</option>
                        <option value="1">Turn the switch OFF</option>
                        <option value="2">Turn the switch ON</option>
                    </select>
                </div>
            </div>

            <div id="dczRelayTemplate" class="template">
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Switch</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt class="pure-u-1 pure-u-lg-23-24 dczRelayIdx"
                              name="dczRelayIdx"
                              type="number"
                              min="0"
                              tabindex="0"
                              data="0"/>
                    </div>
                </div>
            </div>

            <!-- removeIf(!sensor) -->
            <div id="dczMagnitudeTemplate" class="template">
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Magnitude</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt class="pure-u-1 pure-u-lg-23-24 center"
                              name="dczMagnitude"
                              type="number"
                              min="0"
                              tabindex="0"
                              data="0"/>
                    </div>
                    <div class="pure-u-1 pure-u-lg-1-2 hint center"></div>
                </div>
            </div>
            <!-- endRemoveIf(!sensor) -->

            <div id="tspkRelayTemplate" class="template">
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Switch</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt class="pure-u-1 pure-u-lg-23-24"
                              name="tspkRelay"
                              type="number"
                              min="0"
                              max="8"
                              tabindex="0"
                              data="0"/>
                    </div>
                </div>
            </div>

            <!-- removeIf(!sensor) -->
            <div id="tspkMagnitudeTemplate" class="template">
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Magnitude</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt class="pure-u-1 pure-u-lg-23-24 center"
                              name="tspkMagnitude"
                              type="number"
                              min="0"
                              max="8"
                              tabindex="0"
                              data="0"/>
                    </div>
                    <div class="pure-u-1 pure-u-lg-1-2 hint center"></div>
                </div>
            </div>
            <!-- endRemoveIf(!sensor) -->

            <!-- removeIf(!light) -->
            <div id="colorTemplate" class="template">
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Color</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4" data-wcp-layout="block" name="color" readonly/>
                </div>
            </div>

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

            <div id="miredsTemplate" class="template">
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Mireds (Cold &harr; Warm)</label>
                    <Inpt id="mireds" type="range" min="153" max="500" class="slider pure-u-lg-1-4">
                        <span class="slider mireds pure-u-lg-1-4"></span>
                    </inpt>
                </div>
            </div>
            <!-- endRemoveIf(!light) -->

            <!-- removeIf(!sensor) -->
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
            <div id="nodeTemplate" class="template">
                <div class="pure-g">
                    <div class="pure-u-md-1-6 pure-u-1-2">
                        <Inpt name="node"
                              type="text"
                              class="pure-u-11-12"
                              value=""
                              size="8"
                              tabindex="0"
                              placeholder="Node ID"
                              autocomplete="false"/>
                    </div>
                    <div class="pure-u-md-1-6 pure-u-1-2">
                        <Inpt name="key"
                              type="text"
                              class="pure-u-11-12"
                              value=""
                              size="8"
                              tabindex="0"
                              placeholder="Key"/>
                    </div>
                    <div class="pure-u-md-1-2 pure-u-3-4">
                        <Inpt name="topic"
                              type="text"
                              class="pure-md-11-12 pure-u-23-24"
                              value=""
                              size="8"
                              tabindex="0"
                              placeholder="MQTT Topic"/>
                    </div>
                    <div class="pure-u-md-1-6 pure-u-1-4">
                        <button type="button" class="pure-button button-del-mapping pure-u-5-6 pure-u-md-5-6">Del
                        </button>
                    </div>
                </div>
            </div>
            <!-- endRemoveIf(!rfm69) -->

            <iframe id="downloader"></iframe>
            <Inpt id="uploader" type="file"/>
        </div>
</template>

<script>
    import Inpt from './components/Input';
    import Menu from './components/Menu';
    import Form from './components/Form';

    export default {
        name: "espurna-ui",
        components: {
            Inpt,
            Menu,
            Form
        },
        data() {
            return {
                relays: [],
                numberOfChannels: 0,
                tabs: [
                    //Basic settings
                    {k: "status", l: "Status"}, //Move debug to status
                    {k: "general", l: "General"}, //Move wifi to general
                    {k: "admin", l: "Admin"}, //Move ntp to admin
                    {k: "mqtt", l: "MQTT"},

                    //Board Features

                    // <!-- removeIf(!thermostat) -->
                    {k: "thermostat", l: "Thermostat"},
                    // <!-- endRemoveIf(!thermostat) -->

                    // <!-- removeIf(!led) -->
                    {k: "led", l: "LED"},
                    // <!-- endRemoveIf(!led) -->

                    // <!-- removeIf(!light) -->
                    {k: "color", l: "Lights"}, //Move color schedules here as well
                    // <!-- endRemoveIf(!light) -->

                    // <!-- removeIf(!rfm69) -->
                    {k: "mapping", l: "RFM69 Mapping"}, //Move messages to mapping
                    // <!-- endRemoveIf(!rfm69) -->

                    // <!-- removeIf(!rfbridge) -->
                    {k: "rfb", l: "RF Bridge"},
                    // <!-- endRemoveIf(!rfbridge) -->

                    // <!-- removeIf(!sensor) -->
                    {k: "sns", l: "Sensors"},
                    // <!-- endRemoveIf(!sensor) -->

                    {k: "relays", l: "Switches"}, //Move schedules to switches

                    //Integrations
                    // <!-- removeIf(!ha) -->
                    {k: "ha", l: "Home Assistant"},
                    // <!-- endRemoveIf(!ha) -->

                    // <!-- removeIf(!lightfox) -->
                    {k: "lightfox", l: "LightFox"},
                    // <!-- endRemoveIf(!lightfox) -->

                    // <!-- removeIf(!dcz) -->
                    {k: "dcz", l: "Domoticz"},
                    // <!-- endRemoveIf(!dcz) -->

                    // <!-- removeIf(!nofuss) -->
                    {k: "nofuss", l: "NoFuss"},
                    // <!-- endRemoveIf(!nofuss) -->

                    // <!-- removeIf(!thingspeak) -->
                    {k: "thingspeak", l: "ThingSpeak"},
                    // <!-- endRemoveIf(!thingspeak) -->

                    // <!-- removeIf(!idb) -->
                    {k: "idb", l: "InfluxDB"},
                    // <!-- endRemoveIf(!idb) -->

                    //{k: "debug", l: "Debug"}
                ],
            }
        },
        computed: {
            relayOptions() {
                let options = [];
                for (let i = 0; i < this.relays.length; ++i) {
                    options.push("Switch #" + i);
                }
                return options;
            },
            lightOptions() {
                let options = [];
                for (let i = 0; i < this.numberOfChannels; ++i) {
                    options.push("Channel #" + i);
                }
                return options;
            }
        },
        methods: {
            save() {

            },
            reconnect() {

            },
            reboot() {

            }
        }
    };
</script>

<style lang="less">
    /* -----------------------------------------------------------------------------
    General
   -------------------------------------------------------------------------- */

    #menu .pure-menu-heading {
        font-size: 100%;
        padding: .5em .5em;
        white-space: normal;
        text-transform: initial;
    }

    .pure-g {
        margin-bottom: 0;
    }

    .pure-form legend {
        font-weight: bold;
        letter-spacing: 0;
        margin: 10px 0 1em 0;
    }

    .pure-form .pure-g > label {
        margin: .4em 0 .2em;
    }

    .pure-form input {
        margin-bottom: 10px;
    }

    .pure-form input[type=text][disabled] {
        color: #777777;
    }

    @media screen and (max-width: 32em) {
        .header > h1 {
            line-height: 100%;
            font-size: 2em;
        }
    }

    h2 {
        font-size: 1em;
    }

    .panel {
        display: none;
    }

    .block {
        display: block;
    }

    .page {
        margin-top: 10px;
    }

    .hint {
        color: #ccc;
        font-size: 80%;
        margin: -10px 0 10px 0;
    }

    .hint a {
        color: inherit;
    }

    legend.module,
    .module {
        display: none;
    }

    .template {
        display: none;
    }

    input[name=upgrade] {
        display: none;
    }

    select {
        margin-bottom: 10px;
        width: 100%;
    }

    input.center {
        margin-bottom: 0;
    }

    div.center {
        margin: .5em 0 1em;
    }

    .webmode {
        display: none;
    }

    #password .content {
        margin: 0 auto;
    }

    #layout .content {
        margin: 0;
    }

    div.state {
        border-top: 1px solid #eee;
        margin-top: 20px;
        padding-top: 30px;
    }

    .state div {
        font-size: 80%;
    }

    .state span {
        font-size: 80%;
        font-weight: bold;
    }

    .right {
        text-align: right;
    }

    .pure-g span.terminal,
    .pure-g textarea.terminal {
        font-family: 'Courier New', monospace;
        font-size: 80%;
        line-height: 100%;
        background-color: #000;
        color: #0F0;
    }

    /* -----------------------------------------------------------------------------
        Buttons
       -------------------------------------------------------------------------- */

    .pure-button {
        border-radius: 4px;
        color: white;
        letter-spacing: 0;
        margin-bottom: 10px;
        text-shadow: 0 1px 1px rgba(0, 0, 0, 0.2);
        padding: 8px 8px;
    }

    .main-buttons {
        margin: 20px auto;
        text-align: center;
    }

    .main-buttons button {
        width: 100px;
    }

    .button-del-schedule {
        margin-top: 15px;
    }

    .button-reboot,
    .button-reconnect,
    .button-ha-del,
    .button-rfb-forget,
    .button-lightfox-clear,
    .button-del-network,
    .button-del-mapping,
    .button-del-schedule,
    .button-dbg-clear,
    .button-upgrade,
    .button-clear-filters,
    .button-clear-messages,
    .button-clear-counts,
    .button-settings-factory {
        background: rgb(192, 0, 0); /* redish */
    }

    .button-update,
    .button-update-password,
    .button-add-network,
    .button-add-mapping,
    .button-upgrade-browse,
    .button-rfb-learn,
    .button-lightfox-learn,
    .button-ha-add,
    .button-ha-config,
    .button-settings-backup,
    .button-settings-restore,
    .button-dbgcmd,
    .button-apikey {
        background: rgb(0, 192, 0); /* green */
    }

    .button-add-switch-schedule,
    .button-add-light-schedule {
        background: rgb(0, 192, 0); /* green */
        display: none;
    }

    .button-more-network,
    .button-more-schedule,
    .button-wifi-scan,
    .button-rfb-send {
        background: rgb(255, 128, 0); /* orange */
    }

    .button-generate-password {
        background: rgb(66, 184, 221); /* blue */
    }

    .button-upgrade-browse,
    .button-clear-filters,
    .button-clear-messages,
    .button-clear-counts,
    .button-dbgcmd,
    .button-ha-add,
    .button-apikey,
    .button-upgrade {
        margin-left: 5px;
    }

    .button-thermostat-reset-counters {
        background: rgb(204, 139, 41);
    }

    /* -----------------------------------------------------------------------------
        Sliders
       -------------------------------------------------------------------------- */

    input.slider {
        margin-top: 10px;
    }

    span.slider {
        font-size: 70%;
        letter-spacing: 0;
        margin-left: 10px;
        margin-top: 7px;
    }

    /* -----------------------------------------------------------------------------
        Checkboxes
       -------------------------------------------------------------------------- */

    .toggleWrapper {
        overflow: hidden;
        width: auto;
        height: 30px;
        margin: 0 0 10px 0;
        padding: 0;
        border-radius: 4px;
        box-shadow: inset 1px 1px #CCC;
    }

    .toggleWrapper input {
        position: absolute;
        left: -99em;
    }

    label[for].toggle {
        margin: 0;
        padding: 0;
    }

    .toggle {
        letter-spacing: normal;
        cursor: pointer;
        display: inline-block;
        position: relative;
        width: 130px;
        height: 100%;
        background: #e9e9e9;
        color: #a9a9a9;
        border-radius: 4px;
        transition: all 200ms cubic-bezier(0.445, 0.05, 0.55, 0.95);
    }

    .toggle:before,
    .toggle:after {
        position: absolute;
        line-height: 30px;
        font-size: .7em;
        z-index: 2;
        transition: all 200ms cubic-bezier(0.445, 0.05, 0.55, 0.95);
    }

    .toggle:before {
        content: "NO";
        left: 20px;
    }

    input[name="relay"] + .toggle:before {
        content: "OFF";
    }

    input[name="thermostatMode"] + .toggle:before {
        content: "Heater";
    }

    .toggle:after {
        content: "YES";
        right: 20px;
    }

    input[name="relay"] + .toggle:after {
        content: "ON";
    }

    input[name="thermostatMode"] + .toggle:after {
        content: "Cooler";
    }

    .toggle__handler {
        display: inline-block;
        position: relative;
        z-index: 1;
        background: #c00000;
        width: 50%;
        height: 100%;
        border-radius: 4px 0 0 4px;
        top: 0;
        left: 0;
        transition: all 200ms cubic-bezier(0.445, 0.05, 0.55, 0.95);
        transform: translateX(0);
    }

    input:checked + .toggle:after {
        color: #fff;
    }

    input:checked + .toggle:before {
        color: #a9a9a9;
    }

    input + .toggle:before {
        color: #fff;
    }

    input:checked + .toggle .toggle__handler {
        width: 50%;
        background: #00c000;
        transform: translateX(65px);
        border-color: #000;
        border-radius: 0 4px 4px 0;
    }

    input[name="thermostatMode"]:checked + .toggle .toggle__handler {
        background: #00c0c0;
    }

    input[disabled] + .toggle .toggle__handler {
        background: #ccc;
    }

    /* -----------------------------------------------------------------------------
        Loading
       -------------------------------------------------------------------------- */

    .loading {
        background-image: url('~@/assets/loading.gif');
        display: none;
        height: 20px;
        margin: 8px 0 0 10px;
        width: 20px;
    }

    /* -----------------------------------------------------------------------------
        Menu
       -------------------------------------------------------------------------- */

    #menu .small {
        font-size: 60%;
        padding-left: 9px;
    }

    #menu div.footer {
        color: #999;
        font-size: 80%;
        padding: 10px;
    }

    #menu div.footer a {
        padding: 0;
        text-decoration: none;
    }

    /* -----------------------------------------------------------------------------
        RF Bridge panel
       -------------------------------------------------------------------------- */

    #panel-rfb fieldset {
        margin: 10px 2px;
        padding: 20px;
    }

    #panel-rfb input {
        margin-right: 5px;
    }

    #panel-rfb label {
        padding-top: 5px;
    }

    #panel-rfb input {
        text-align: center;
    }

    /* -----------------------------------------------------------------------------
        Admin panel
       -------------------------------------------------------------------------- */

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

    /* -----------------------------------------------------------------------------
        Wifi panel
       -------------------------------------------------------------------------- */

    #networks .pure-g,
    #schedules .pure-g {
        border-bottom: 1px solid #eee;
        margin-bottom: 10px;
        padding: 10px 0 10px 0;
    }

    #networks .more {
        display: none;
    }

    #haConfig,
    #scanResult {
        margin-top: 10px;
        display: none;
        padding: 10px;
    }

    /* -----------------------------------------------------------------------------
        Table
       -------------------------------------------------------------------------- */

    .right {
        text-align: right;
    }

    table.dataTable.display tbody td {
        text-align: center;
    }

    #packets_filter {
        display: none;
    }

    .filtered {
        color: rgb(202, 60, 60);
    }

    /* -----------------------------------------------------------------------------
        Logs
       -------------------------------------------------------------------------- */

    #weblog {
        height: 400px;
        margin-bottom: 10px;
    }

    /* -----------------------------------------------------------------------------
        Password input controls
       -------------------------------------------------------------------------- */
    .password-reveal {
        font-family: EmojiSymbols, Segoe UI Symbol;
        background: rgba(0, 0, 0, 0);
        display: inline-block;
        float: right;
        z-index: 50;
        margin-top: 6px;
        margin-left: -30px;
        vertical-align: middle;
        font-size: 1.2em;
        height: 100%;
    }

    .password-reveal:after {
        content: "👁";
    }

    input[type="password"] + .password-reveal {
        color: rgba(205, 205, 205, 0.3);
    }

    input[type="text"] + .password-reveal {
        color: rgba(66, 184, 221, 0.8);
    }

    .no-select {
        user-select: none;
    }

    input::-ms-clear,
    input::-ms-reveal {
        display: none;
    }

    /* css minifier must not combine these.
     * style will not apply otherwise */
    input::-ms-input-placeholder {
        color: #ccd;
    }

    input::placeholder {
        color: #ccc;
    }

</style>
