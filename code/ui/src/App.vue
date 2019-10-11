<template>
    <div>
        <div id="password" class="webmode">
            <div class="content">
                <form id="formPassword" class="pure-form" autocomplete="off">
                    <div id="panel-password" class="panel block">
                        <div class="header">
                            <h1>SECURITY</h1>
                            <h2>
                                Before using this device you have to change the default password for the user <strong>admin</strong>.
                                This password will be used for the <strong>AP mode hotspot</strong>, the <strong>web
                                interface</strong> (where you are now) and the <strong>over-the-air updates</strong>.
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
                                    these special characters: _,.;:~!?@#$%^&amp;*&lt;&gt;\|(){}[]) and have at least
                                    <strong>one lowercase</strong> and <strong>one uppercase</strong> or <strong>one
                                    number</strong>.
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
                    <Status/>
                </template>

                <template #general>
                    <General/>
                </template>

                <template #admin>
                    <Admin/>
                </template>

                <template #mqtt>
                    <Mqtt/>
                </template>

                <!-- removeIf(!thermostat) -->
                <template #thermostat>
                    <Thermostat/>
                </template>
                <!-- endRemoveIf(!thermostat) -->

                <!-- removeIf(!led) -->
                <template #led>
                    <Led/>
                </template>
                <!-- endRemoveIf(!led) -->

                <!-- removeIf(!light) -->
                <template #color>
                    <Color/>
                </template>
                <!-- endRemoveIf(!light) -->

                <!-- removeIf(!rfm69) -->
                <template #rfm69>
                    <Rfm69/>
                </template>
                <!-- endRemoveIf(!rfm69) -->

                <!-- removeIf(!rfbridge) -->
                <template #rfb>
                    <Rfb/>
                </template>
                <!-- endRemoveIf(!rfbridge) -->


                <!-- removeIf(!sensor) -->
                <template #sns>
                    <Sensors/>
                </template>
                <!-- endRemoveIf(!sensor) -->

                <template #relays>
                    <Relays/>
                </template>

                <!-- removeIf(!lightfox) -->
                <template #lightfox>
                    <Lightfox/>
                </template>
                <!-- endRemoveIf(!lightfox) -->


                <!-- removeIf(!dcz) -->
                <template #dcz>
                    <Dcz/>
                </template>
                <!-- endRemoveIf(!dcz) -->

                <!-- removeIf(!ha) -->
                <template #ha>
                    <Ha/>
                </template>
                <!-- endRemoveIf(!ha) -->

                <!-- removeIf(!thingspeak) -->
                <template #thingspeak>
                    <Thingspeak/>
                </template>
                <!-- endRemoveIf(!thingspeak) -->

                <!-- removeIf(!idb) -->
                <template #idb>
                    <Idb/>
                </template>
                <!-- endRemoveIf(!idb) -->

                <!-- removeIf(!nofuss) -->
                <template #nofuss>
                    <Nofuss/>
                </template>
                <!-- endRemoveIf(!nofuss) -->
            </Menu>


            <div id="switchActionTemplate" class="template">
                <label class="pure-u-1 pure-u-lg-1-4">Action</label>
                <div class="pure-u-1 pure-u-lg-1-5">
                    <Inpt class="pure-u-1 pure-u-lg-23-24" name="schAction"
                          :options="['Turn Off', 'Turn On', 'Toggle']"/>
                </div>
                <Inpt class="pure-u-1 pure-u-lg-1-5 isrelay" name="schSwitch" :options="relayOptions"/>
                <Inpt type="hidden" name="schType" value="1"/>
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

            <iframe id="downloader"></iframe>
            <Inpt id="uploader" type="file"/>
        </div>
</template>

<script>
    import Inpt from './components/Input';
    import Menu from './components/Menu';
    import Form from './components/Form';
    import Nofuss from "./menus/integrations/NoFuss";
    import Idb from "./menus/integrations/InfluxDB";
    import Thingspeak from "./menus/integrations/ThingSpeak";
    import Ha from "./menus/integrations/HomeAssistant";
    import Dcz from "./menus/integrations/Domoticz";
    import Lightfox from "./menus/features/LightFox";
    import Relays from "./menus/features/Relays";
    import Sensors from "./menus/features/Sensors";
    import Rfb from "./menus/features/Rfb";
    import Rfm69 from "./menus/features/Rfm69";
    import Color from "./menus/features/Color";
    import Led from "./menus/features/Led";
    import Thermostat from "./menus/features/Thermostat";
    import Mqtt from "./menus/common/Mqtt";
    import Admin from "./menus/common/Admin";
    import General from "./menus/common/General";
    import Status from "./menus/common/Status";

    export default {
        name: "espurna-ui",
        components: {
            Status,
            General,
            Admin,
            Mqtt,
            Thermostat,
            Led,
            Color,
            Rfm69,
            Rfb,
            Sensors,
            Relays,
            Lightfox,
            Dcz,
            Ha,
            Thingspeak,
            Idb,
            Nofuss,
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
                    {k: "rfm69", l: "RFM69 Mapping"}, //Move messages to mapping
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
