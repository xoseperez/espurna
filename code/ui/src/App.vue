<template>
    <div>
        <div id="password" class="webmode" style="display: none">
            <div class="content">
                <Form id="formPassword" class="pure-form" autocomplete="off">
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
                                    <Inpt id="adminPass1" class="pure-u-1 pure-u-lg-3-4"
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
                                    <Inpt id="adminPass2" class="pure-u-1 pure-u-lg-3-4"
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
                </Form>
            </div> <!-- content -->
        </div>

        <Form>
            <Menu id="layout" :tabs="tabs" class="webmode">
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
                        <A href="https://twitter.com/xoseperez">@xoseperez</A><br>
                        <A href="http://tinkerman.cat">http://tinkerman.cat</A><br>
                        <A href="https://github.com/xoseperez/espurna">ESPurna @
                            GitHub</A><br>
                        UI by <A href="https://github.com/tofandel">Tofandel</A><br>
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

                <!-- #if process.env.VUE_APP_THERMOSTAT === 'true' -->
                <template #thermostat>
                    <Tstat/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_LED === 'true' -->
                <template #led>
                    <Led/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_LIGHT === 'true' -->
                <template #color>
                    <Color/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_RFM69 === 'true' -->
                <template #rfm69>
                    <Rfm69/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_RFBRIDGE === 'true' -->
                <template #rfb>
                    <Rfb/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_SENSOR === 'true' -->
                <template #sns>
                    <Sns/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_RELAYS === 'true' -->
                <template #relays>
                    <Relays/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_LIGHTFOX === 'true' -->
                <template #lightfox>
                    <Lfox/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_DCZ === 'true' -->
                <template #dcz>
                    <Dcz/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_HA === 'true' -->
                <template #ha>
                    <Ha/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_THINGSPEAK === 'true' -->
                <template #thingspeak>
                    <Tspk/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_IDB === 'true' -->
                <template #idb>
                    <Idb/>
                </template>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_NOFUSS === 'true' -->
                <template #nofuss>
                    <Nfss/>
                </template>
                <!-- #endif -->
            </Menu>
        </Form>


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
            <Inpt class="pure-u-1 pure-u-lg-1-5 islight" name="schSwitch" :options="lightOptions"/>
            <Inpt type="hidden" name="schType" value="2"/>
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

            <iframe id="downloader"></iframe>
            <Inpt id="uploader" type="file"/>
        </div>
    </div>
</template>

<script>
    import Inpt from './components/Input';
    import Menu from './components/Menu';
    import Form from './components/Form';
    import A from './components/ExtLink';


    import Mqtt from "./menus/common/Mqtt";
    import Admin from "./menus/common/Admin";
    import General from "./menus/common/General";
    import Status from "./menus/common/Status";

    let tabs = [
        //Basic settings
        {k: "status", l: "Status"}, //Move debug to status
        {k: "general", l: "General"}, //Move wifi to general
        {k: "admin", l: "Admin"}, //Move ntp to admin
        {k: "mqtt", l: "MQTT"}
    ];
    let components = {A, Inpt, Menu, Form, Mqtt, Admin, General, Status};

    //Board Features

    // #if process.env.VUE_APP_THERMOSTAT === 'true'
    import Tstat from "./menus/features/Thermostat";

    components.Tstat = Tstat;
    tabs.push({k: "thermostat", l: "Thermostat"});
    // #endif

    // #if process.env.VUE_APP_LED === 'true'
    import Led from "./menus/features/Led";

    components.Led = Led;
    tabs.push({k: "led", l: "LED"});
    // #endif

    // #if process.env.VUE_APP_LIGHT === 'true'
    import Color from "./menus/features/Color";

    components.Color = Color;
    tabs.push({k: "color", l: "Lights"}); //Moved color schedules here
    // #endif

    // #if process.env.VUE_APP_RFM69 === 'true'
    import Rfm69 from "./menus/features/Rfm69";

    components.Rfm69 = Rfm69;
    tabs.push({k: "rfm69", l: "RFM69 Mapping"}); //Moved messages and mapping here
    // #endif

    // #if process.env.VUE_APP_RFBRIDGE === 'true'
    import Rfb from "./menus/features/Rfb";

    components.Rfb = Rfb;
    tabs.push({k: "rfb", l: "RF Bridge"});
    // #endif

    // #if process.env.VUE_APP_SENSOR === 'true'
    import Sns from "./menus/features/Sensors";

    components.Sns = Sns;
    tabs.push({k: "sns", l: "Sensors"});
    // #endif

    // #if process.env.VUE_APP_RELAYS === 'true'
    import Relays from "./menus/features/Relays";

    components.Relays = Relays;
    tabs.push({k: "relays", l: "Switches"}); //Moved schedules to switches
    // #endif

    //Integrations
    // #if process.env.VUE_APP_HA === 'true'
    import Ha from "./menus/integrations/HomeAssistant";

    components.Ha = Ha;
    tabs.push({k: "ha", l: "Home Assistant"});
    // #endif

    // #if process.env.VUE_APP_LIGHTFOX === 'true'
    import Lfox from "./menus/features/LightFox";

    components.Lfox = Lfox;
    tabs.push({k: "lightfox", l: "LightFox"});
    // #endif

    // #if process.env.VUE_APP_DCZ === 'true'
    import Dcz from "./menus/integrations/Domoticz";

    components.Dcz = Dcz;
    tabs.push({k: "dcz", l: "Domoticz"});
    // #endif

    // #if process.env.VUE_APP_NOFUSS === 'true'
    import Nfss from "./menus/integrations/NoFuss";

    components.Nfss = Nfss;
    tabs.push({k: "nofuss", l: "NoFuss"});
    // #endif

    // #if process.env.VUE_APP_THINGSPEAK === 'true'
    import Tspk from "./menus/integrations/ThingSpeak";

    components.Tspk = Tspk;
    tabs.push({k: "thingspeak", l: "ThingSpeak"});
    // #endif

    // #if process.env.VUE_APP_IDB === 'true'
    import Idb from "./menus/integrations/InfluxDB";

    components.Idb = Idb;
    tabs.push({k: "idb", l: "InfluxDB"});
    // #endif


    export default {
        name: "espurna-ui",
        components,
        data() {
            return {
                relays: [],
                numberOfChannels: 0,
                tabs: tabs
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
            reconnect(ask) {
                let question = (typeof ask === "undefined" || false === ask) ?
                    null :
                    "Are you sure you want to disconnect from the current WIFI network?";
                this.doAction(question, "reconnect");
            },
            reboot(ask) {
                let question = (typeof ask === "undefined" || false === ask) ?
                    null :
                    "Are you sure you want to reboot the device?";
                this.doAction(question, "reboot");
            },
            doAction(question, action) {
                this.checkChanges();

                if (question) {
                    let response = window.confirm(question);
                    if (false === response) {
                        return;
                    }
                }

                this.sendAction(action, {});
                this.doReload(5000);
            }
            , sendAction(action, data) {
                websock.send(JSON.stringify({action: action, data: data}));
            }
            , sendConfig(data) {
                websock.send(JSON.stringify({config: data}));
            }
        }
    };
</script>

<style lang="less">
    /* -----------------------------------------------------------------------------
    General
   -------------------------------------------------------------------------- */

    .menu .pure-menu-heading {
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

    .menu .small {
        font-size: 60%;
        padding-left: 9px;
    }

    .menu div.footer {
        color: #999;
        font-size: 80%;
        padding: 10px;
    }

    .menu div.footer a {
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

    // Side menu.css
    body {
        color: #777;
    }

    .pure-img-responsive {
        max-width: 100%;
        height: auto;
    }

    /*
    Add transition to containers so they can push in and out.
    */
    #layout,
    .menu,
    .menu-link {
        transition: all 0.2s ease-out;
    }

    /*
    This is the parent `<div>` that contains the menu and the content area.
    */
    #layout {
        position: relative;
        left: 0;
        padding-left: 0;
    }

    #layout.active .menu {
        left: 160px;
        width: 160px;
    }

    #layout.active .menu-link {
        left: 160px;
    }

    /*
    The content `<div>` is where all your content goes.
    */
    .content {
        padding: 0 2em;
        max-width: 800px;
        margin: 0 auto 50px;
        line-height: 1.6em;
    }

    .header {
        margin: 0;
        color: #333;
        text-align: center;
        padding: 2.5em 2em 0;
        border-bottom: 1px solid #eee;
    }

    .header h1 {
        margin: 0.2em 0;
        font-size: 3em;
        font-weight: 300;
    }

    .header h2 {
        font-weight: 300;
        color: #ccc;
        padding: 0;
        margin-top: 0;
    }

    .content-subhead {
        margin: 50px 0 20px 0;
        font-weight: 300;
        color: #888;
    }


    //Pure
    html {
        font-family: sans-serif;
        -webkit-text-size-adjust: 100%
    }

    body {
        margin: 0
    }

    article, aside, details, figcaption, figure, footer, header, hgroup, main, menu, nav, section, summary {
        display: block
    }

    audio, canvas, progress, video {
        display: inline-block;
        vertical-align: baseline
    }

    audio:not([controls]) {
        display: none;
        height: 0
    }

    [hidden], template {
        display: none
    }

    a {
        background-color: transparent
    }

    a:active, a:hover {
        outline: 0
    }

    abbr[title] {
        border-bottom: 1px dotted
    }

    b, strong {
        font-weight: 700
    }

    dfn {
        font-style: italic
    }

    h1 {
        font-size: 2em;
        margin: .67em 0
    }

    mark {
        background: #ff0;
        color: #000
    }

    small {
        font-size: 80%
    }

    sub, sup {
        font-size: 75%;
        line-height: 0;
        position: relative;
        vertical-align: baseline
    }

    sup {
        top: -.5em
    }

    sub {
        bottom: -.25em
    }

    img {
        border: 0
    }

    svg:not(:root) {
        overflow: hidden
    }

    figure {
        margin: 1em 40px
    }

    hr {
        -webkit-box-sizing: content-box;
        box-sizing: content-box;
        height: 0
    }

    pre {
        overflow: auto
    }

    code, kbd, pre, samp {
        font-family: monospace, monospace;
        font-size: 1em
    }

    button, input, optgroup, select, textarea {
        color: inherit;
        font: inherit;
        margin: 0
    }

    button {
        overflow: visible
    }

    button, select {
        text-transform: none
    }

    button, html input[type=button], input[type=reset], input[type=submit] {
        -webkit-appearance: button;
        cursor: pointer
    }

    button[disabled], html input[disabled] {
        cursor: default
    }

    button::-moz-focus-inner, input::-moz-focus-inner {
        border: 0;
        padding: 0
    }

    input {
        line-height: normal
    }

    input[type=checkbox], input[type=radio] {
        -webkit-box-sizing: border-box;
        box-sizing: border-box;
        padding: 0
    }

    input[type=number]::-webkit-inner-spin-button, input[type=number]::-webkit-outer-spin-button {
        height: auto
    }

    input[type=search] {
        -webkit-appearance: textfield;
        -webkit-box-sizing: content-box;
        box-sizing: content-box
    }

    input[type=search]::-webkit-search-cancel-button, input[type=search]::-webkit-search-decoration {
        -webkit-appearance: none
    }

    fieldset {
        border: 1px solid silver;
        margin: 0 2px;
        padding: .35em .625em .75em
    }

    legend {
        border: 0;
        padding: 0
    }

    textarea {
        overflow: auto
    }

    optgroup {
        font-weight: 700
    }

    table {
        border-collapse: collapse;
        border-spacing: 0
    }

    td, th {
        padding: 0
    }

    .hidden, [hidden] {
        display: none !important
    }

    .pure-img {
        max-width: 100%;
        height: auto;
        display: block
    }

    .pure-g {
        letter-spacing: -.31em;
        text-rendering: optimizespeed;
        font-family: FreeSans, Arimo, "Droid Sans", Helvetica, Arial, sans-serif;
        display: flex;
        flex-flow: row wrap;
        align-content: flex-start
    }

    .pure-u {
        display: inline-block;
        zoom: 1;
        letter-spacing: normal;
        word-spacing: normal;
        vertical-align: top;
        text-rendering: auto
    }

    .pure-g [class*=pure-u] {
        font-family: sans-serif
    }

    .pure-u-1, .pure-u-1-1, .pure-u-1-12, .pure-u-1-2, .pure-u-1-24, .pure-u-1-3, .pure-u-1-4, .pure-u-1-5, .pure-u-1-6, .pure-u-1-8, .pure-u-10-24, .pure-u-11-12, .pure-u-11-24, .pure-u-12-24, .pure-u-13-24, .pure-u-14-24, .pure-u-15-24, .pure-u-16-24, .pure-u-17-24, .pure-u-18-24, .pure-u-19-24, .pure-u-2-24, .pure-u-2-3, .pure-u-2-5, .pure-u-20-24, .pure-u-21-24, .pure-u-22-24, .pure-u-23-24, .pure-u-24-24, .pure-u-3-24, .pure-u-3-4, .pure-u-3-5, .pure-u-3-8, .pure-u-4-24, .pure-u-4-5, .pure-u-5-12, .pure-u-5-24, .pure-u-5-5, .pure-u-5-6, .pure-u-5-8, .pure-u-6-24, .pure-u-7-12, .pure-u-7-24, .pure-u-7-8, .pure-u-8-24, .pure-u-9-24 {
        display: inline-block;
        zoom: 1;
        letter-spacing: normal;
        word-spacing: normal;
        vertical-align: top;
        text-rendering: auto
    }

    .pure-u-1-24 {
        width: 4.1667%
    }

    .pure-u-1-12, .pure-u-2-24 {
        width: 8.3333%
    }

    .pure-u-1-8, .pure-u-3-24 {
        width: 12.5%
    }

    .pure-u-1-6, .pure-u-4-24 {
        width: 16.6667%
    }

    .pure-u-1-5 {
        width: 20%
    }

    .pure-u-5-24 {
        width: 20.8333%
    }

    .pure-u-1-4, .pure-u-6-24 {
        width: 25%
    }

    .pure-u-7-24 {
        width: 29.1667%
    }

    .pure-u-1-3, .pure-u-8-24 {
        width: 33.3333%
    }

    .pure-u-3-8, .pure-u-9-24 {
        width: 37.5%
    }

    .pure-u-2-5 {
        width: 40%
    }

    .pure-u-10-24, .pure-u-5-12 {
        width: 41.6667%
    }

    .pure-u-11-24 {
        width: 45.8333%
    }

    .pure-u-1-2, .pure-u-12-24 {
        width: 50%
    }

    .pure-u-13-24 {
        width: 54.1667%
    }

    .pure-u-14-24, .pure-u-7-12 {
        width: 58.3333%
    }

    .pure-u-3-5 {
        width: 60%
    }

    .pure-u-15-24, .pure-u-5-8 {
        width: 62.5%
    }

    .pure-u-16-24, .pure-u-2-3 {
        width: 66.6667%
    }

    .pure-u-17-24 {
        width: 70.8333%
    }

    .pure-u-18-24, .pure-u-3-4 {
        width: 75%
    }

    .pure-u-19-24 {
        width: 79.1667%
    }

    .pure-u-4-5 {
        width: 80%
    }

    .pure-u-20-24, .pure-u-5-6 {
        width: 83.3333%
    }

    .pure-u-21-24, .pure-u-7-8 {
        width: 87.5%
    }

    .pure-u-11-12, .pure-u-22-24 {
        width: 91.6667%
    }

    .pure-u-23-24 {
        width: 95.8333%
    }

    .pure-u-1, .pure-u-1-1, .pure-u-24-24, .pure-u-5-5 {
        width: 100%
    }

    .pure-button {
        display: inline-block;
        zoom: 1;
        line-height: normal;
        white-space: nowrap;
        vertical-align: middle;
        text-align: center;
        cursor: pointer;
        user-select: none;
        box-sizing: border-box
    }

    .pure-button::-moz-focus-inner {
        padding: 0;
        border: 0
    }

    .pure-button-group {
        letter-spacing: -.31em;
        text-rendering: optimizespeed
    }

    .opera-only :-o-prefocus, .pure-button-group {
        word-spacing: -.43em
    }

    .pure-button-group .pure-button {
        letter-spacing: normal;
        word-spacing: normal;
        vertical-align: top;
        text-rendering: auto
    }

    .pure-button {
        font-family: inherit;
        font-size: 100%;
        padding: .5em 1em;
        color: #444;
        color: rgba(0, 0, 0, .8);
        border: 1px solid #999;
        border: none transparent;
        background-color: #e6e6e6;
        text-decoration: none;
        border-radius: 2px
    }

    .pure-button-hover, .pure-button:focus, .pure-button:hover {
        background-image: -webkit-gradient(linear, left top, left bottom, from(transparent), color-stop(40%, rgba(0, 0, 0, .05)), to(rgba(0, 0, 0, .1)));
        background-image: -webkit-linear-gradient(transparent, rgba(0, 0, 0, .05) 40%, rgba(0, 0, 0, .1));
        background-image: linear-gradient(transparent, rgba(0, 0, 0, .05) 40%, rgba(0, 0, 0, .1))
    }

    .pure-button:focus {
        outline: 0
    }

    .pure-button-active, .pure-button:active {
        -webkit-box-shadow: 0 0 0 1px rgba(0, 0, 0, .15) inset, 0 0 6px rgba(0, 0, 0, .2) inset;
        box-shadow: 0 0 0 1px rgba(0, 0, 0, .15) inset, 0 0 6px rgba(0, 0, 0, .2) inset
    }

    .pure-button-disabled, .pure-button-disabled:active, .pure-button-disabled:focus, .pure-button-disabled:hover, .pure-button[disabled] {
        border: none;
        background-image: none;
        opacity: .4;
        cursor: not-allowed;
        -webkit-box-shadow: none;
        box-shadow: none;
        pointer-events: none
    }

    .pure-button-hidden {
        display: none
    }

    .pure-button-primary, .pure-button-selected, a.pure-button-primary, a.pure-button-selected {
        background-color: #0078e7;
        color: #fff
    }

    .pure-button-group .pure-button {
        margin: 0;
        border-radius: 0;
        border-right: 1px solid #111;
        border-right: 1px solid rgba(0, 0, 0, .2)
    }

    .pure-button-group .pure-button:first-child {
        border-top-left-radius: 2px;
        border-bottom-left-radius: 2px
    }

    .pure-button-group .pure-button:last-child {
        border-top-right-radius: 2px;
        border-bottom-right-radius: 2px;
        border-right: none
    }

    .pure-form input[type=color], .pure-form input[type=date], .pure-form input[type=datetime-local], .pure-form input[type=datetime], .pure-form input[type=email], .pure-form input[type=month], .pure-form input[type=number], .pure-form input[type=password], .pure-form input[type=search], .pure-form input[type=tel], .pure-form input[type=text], .pure-form input[type=time], .pure-form input[type=url], .pure-form input[type=week], .pure-form select, .pure-form textarea {
        padding: .5em .6em;
        display: inline-block;
        border: 1px solid #ccc;
        -webkit-box-shadow: inset 0 1px 3px #ddd;
        box-shadow: inset 0 1px 3px #ddd;
        border-radius: 4px;
        vertical-align: middle;
        -webkit-box-sizing: border-box;
        box-sizing: border-box
    }

    .pure-form input:not([type]) {
        padding: .5em .6em;
        display: inline-block;
        border: 1px solid #ccc;
        -webkit-box-shadow: inset 0 1px 3px #ddd;
        box-shadow: inset 0 1px 3px #ddd;
        border-radius: 4px;
        -webkit-box-sizing: border-box;
        box-sizing: border-box
    }

    .pure-form input[type=color] {
        padding: .2em .5em
    }

    .pure-form input[type=color]:focus, .pure-form input[type=date]:focus, .pure-form input[type=datetime-local]:focus, .pure-form input[type=datetime]:focus, .pure-form input[type=email]:focus, .pure-form input[type=month]:focus, .pure-form input[type=number]:focus, .pure-form input[type=password]:focus, .pure-form input[type=search]:focus, .pure-form input[type=tel]:focus, .pure-form input[type=text]:focus, .pure-form input[type=time]:focus, .pure-form input[type=url]:focus, .pure-form input[type=week]:focus, .pure-form select:focus, .pure-form textarea:focus {
        outline: 0;
        border-color: #129fea
    }

    .pure-form input:not([type]):focus {
        outline: 0;
        border-color: #129fea
    }

    .pure-form input[type=checkbox]:focus, .pure-form input[type=file]:focus, .pure-form input[type=radio]:focus {
        outline: thin solid #129fea;
        outline: 1px auto #129fea
    }

    .pure-form .pure-checkbox, .pure-form .pure-radio {
        margin: .5em 0;
        display: block
    }

    .pure-form input[type=color][disabled], .pure-form input[type=date][disabled], .pure-form input[type=datetime-local][disabled], .pure-form input[type=datetime][disabled], .pure-form input[type=email][disabled], .pure-form input[type=month][disabled], .pure-form input[type=number][disabled], .pure-form input[type=password][disabled], .pure-form input[type=search][disabled], .pure-form input[type=tel][disabled], .pure-form input[type=text][disabled], .pure-form input[type=time][disabled], .pure-form input[type=url][disabled], .pure-form input[type=week][disabled], .pure-form select[disabled], .pure-form textarea[disabled] {
        cursor: not-allowed;
        background-color: #eaeded;
        color: #cad2d3
    }

    .pure-form input:not([type])[disabled] {
        cursor: not-allowed;
        background-color: #eaeded;
        color: #cad2d3
    }

    .pure-form input[readonly], .pure-form select[readonly], .pure-form textarea[readonly] {
        background-color: #eee;
        color: #777;
        border-color: #ccc
    }

    .pure-form input:focus:invalid, .pure-form select:focus:invalid, .pure-form textarea:focus:invalid {
        color: #b94a48;
        border-color: #e9322d
    }

    .pure-form input[type=checkbox]:focus:invalid:focus, .pure-form input[type=file]:focus:invalid:focus, .pure-form input[type=radio]:focus:invalid:focus {
        outline-color: #e9322d
    }

    .pure-form select {
        height: 2.25em;
        border: 1px solid #ccc;
        background-color: #fff
    }

    .pure-form select[multiple] {
        height: auto
    }

    .pure-form label {
        margin: .5em 0 .2em
    }

    .pure-form fieldset {
        margin: 0;
        padding: .35em 0 .75em;
        border: 0
    }

    .pure-form legend {
        display: block;
        width: 100%;
        padding: .3em 0;
        margin-bottom: .3em;
        color: #333;
        border-bottom: 1px solid #e5e5e5
    }

    .pure-form-stacked input[type=color], .pure-form-stacked input[type=date], .pure-form-stacked input[type=datetime-local], .pure-form-stacked input[type=datetime], .pure-form-stacked input[type=email], .pure-form-stacked input[type=file], .pure-form-stacked input[type=month], .pure-form-stacked input[type=number], .pure-form-stacked input[type=password], .pure-form-stacked input[type=search], .pure-form-stacked input[type=tel], .pure-form-stacked input[type=text], .pure-form-stacked input[type=time], .pure-form-stacked input[type=url], .pure-form-stacked input[type=week], .pure-form-stacked label, .pure-form-stacked select, .pure-form-stacked textarea {
        display: block;
        margin: .25em 0
    }

    .pure-form-stacked input:not([type]) {
        display: block;
        margin: .25em 0
    }

    .pure-form-aligned .pure-help-inline, .pure-form-aligned input, .pure-form-aligned select, .pure-form-aligned textarea, .pure-form-message-inline {
        display: inline-block;
        vertical-align: middle
    }

    .pure-form-aligned textarea {
        vertical-align: top
    }

    .pure-form-aligned .pure-control-group {
        margin-bottom: .5em
    }

    .pure-form-aligned .pure-control-group label {
        text-align: right;
        display: inline-block;
        vertical-align: middle;
        width: 10em;
        margin: 0 1em 0 0
    }

    .pure-form-aligned .pure-controls {
        margin: 1.5em 0 0 11em
    }

    .pure-form .pure-input-rounded, .pure-form input.pure-input-rounded {
        border-radius: 2em;
        padding: .5em 1em
    }

    .pure-form .pure-group fieldset {
        margin-bottom: 10px
    }

    .pure-form .pure-group input, .pure-form .pure-group textarea {
        display: block;
        padding: 10px;
        margin: 0 0 -1px;
        border-radius: 0;
        position: relative;
        top: -1px
    }

    .pure-form .pure-group input:focus, .pure-form .pure-group textarea:focus {
        z-index: 3
    }

    .pure-form .pure-group input:first-child, .pure-form .pure-group textarea:first-child {
        top: 1px;
        border-radius: 4px 4px 0 0;
        margin: 0
    }

    .pure-form .pure-group input:first-child:last-child, .pure-form .pure-group textarea:first-child:last-child {
        top: 1px;
        border-radius: 4px;
        margin: 0
    }

    .pure-form .pure-group input:last-child, .pure-form .pure-group textarea:last-child {
        top: -2px;
        border-radius: 0 0 4px 4px;
        margin: 0
    }

    .pure-form .pure-group button {
        margin: .35em 0
    }

    .pure-form .pure-input-1 {
        width: 100%
    }

    .pure-form .pure-input-3-4 {
        width: 75%
    }

    .pure-form .pure-input-2-3 {
        width: 66%
    }

    .pure-form .pure-input-1-2 {
        width: 50%
    }

    .pure-form .pure-input-1-3 {
        width: 33%
    }

    .pure-form .pure-input-1-4 {
        width: 25%
    }

    .pure-form .pure-help-inline, .pure-form-message-inline {
        display: inline-block;
        padding-left: .3em;
        color: #666;
        vertical-align: middle;
        font-size: .875em
    }

    .pure-form-message {
        display: block;
        color: #666;
        font-size: .875em
    }

    @media only screen and (max-width: 480px) {
        .pure-form button[type=submit] {
            margin: .7em 0 0
        }

        .pure-form input:not([type]), .pure-form input[type=color], .pure-form input[type=date], .pure-form input[type=datetime-local], .pure-form input[type=datetime], .pure-form input[type=email], .pure-form input[type=month], .pure-form input[type=number], .pure-form input[type=password], .pure-form input[type=search], .pure-form input[type=tel], .pure-form input[type=text], .pure-form input[type=time], .pure-form input[type=url], .pure-form input[type=week], .pure-form label {
            margin-bottom: .3em;
            display: block
        }

        .pure-group input:not([type]), .pure-group input[type=color], .pure-group input[type=date], .pure-group input[type=datetime-local], .pure-group input[type=datetime], .pure-group input[type=email], .pure-group input[type=month], .pure-group input[type=number], .pure-group input[type=password], .pure-group input[type=search], .pure-group input[type=tel], .pure-group input[type=text], .pure-group input[type=time], .pure-group input[type=url], .pure-group input[type=week] {
            margin-bottom: 0
        }

        .pure-form-aligned .pure-control-group label {
            margin-bottom: .3em;
            text-align: left;
            display: block;
            width: 100%
        }

        .pure-form-aligned .pure-controls {
            margin: 1.5em 0 0 0
        }

        .pure-form .pure-help-inline, .pure-form-message, .pure-form-message-inline {
            display: block;
            font-size: .75em;
            padding: .2em 0 .8em
        }
    }

    .pure-menu {
        -webkit-box-sizing: border-box;
        box-sizing: border-box
    }

    .pure-menu-fixed {
        position: fixed;
        left: 0;
        top: 0;
        z-index: 3
    }

    .pure-menu-item, .pure-menu-list {
        position: relative
    }

    .pure-menu-list {
        list-style: none;
        margin: 0;
        padding: 0
    }

    .pure-menu-item {
        padding: 0;
        margin: 0;
        height: 100%
    }

    .pure-menu-heading, .pure-menu-link {
        display: block;
        text-decoration: none;
        white-space: nowrap
    }

    .pure-menu-horizontal {
        width: 100%;
        white-space: nowrap
    }

    .pure-menu-horizontal .pure-menu-list {
        display: inline-block
    }

    .pure-menu-horizontal .pure-menu-heading, .pure-menu-horizontal .pure-menu-item, .pure-menu-horizontal .pure-menu-separator {
        display: inline-block;
        zoom: 1;
        vertical-align: middle
    }

    .pure-menu-item .pure-menu-item {
        display: block
    }

    .pure-menu-children {
        display: none;
        position: absolute;
        left: 100%;
        top: 0;
        margin: 0;
        padding: 0;
        z-index: 3
    }

    .pure-menu-horizontal .pure-menu-children {
        left: 0;
        top: auto;
        width: inherit
    }

    .pure-menu-active > .pure-menu-children, .pure-menu-allow-hover:hover > .pure-menu-children {
        display: block;
        position: absolute
    }

    .pure-menu-has-children > .pure-menu-link:after {
        padding-left: .5em;
        content: "\25B8";
        font-size: small
    }

    .pure-menu-horizontal .pure-menu-has-children > .pure-menu-link:after {
        content: "\25BE"
    }

    .pure-menu-scrollable {
        overflow-y: scroll;
        overflow-x: hidden
    }

    .pure-menu-scrollable .pure-menu-list {
        display: block
    }

    .pure-menu-horizontal.pure-menu-scrollable .pure-menu-list {
        display: inline-block
    }

    .pure-menu-horizontal.pure-menu-scrollable {
        white-space: nowrap;
        overflow-y: hidden;
        overflow-x: auto;
        -webkit-overflow-scrolling: touch;
        padding: .5em 0
    }

    .pure-menu-horizontal.pure-menu-scrollable::-webkit-scrollbar {
        display: none
    }

    .pure-menu-horizontal .pure-menu-children .pure-menu-separator, .pure-menu-separator {
        background-color: #ccc;
        height: 1px;
        margin: .3em 0
    }

    .pure-menu-horizontal .pure-menu-separator {
        width: 1px;
        height: 1.3em;
        margin: 0 .3em
    }

    .pure-menu-horizontal .pure-menu-children .pure-menu-separator {
        display: block;
        width: auto
    }

    .pure-menu-heading {
        text-transform: uppercase;
        color: #565d64
    }

    .pure-menu-link {
        color: #777
    }

    .pure-menu-children {
        background-color: #fff
    }

    .pure-menu-disabled, .pure-menu-heading, .pure-menu-link {
        padding: .5em 1em
    }

    .pure-menu-disabled {
        opacity: .5
    }

    .pure-menu-disabled .pure-menu-link:hover {
        background-color: transparent
    }

    .pure-menu-active > .pure-menu-link, .pure-menu-link:focus, .pure-menu-link:hover {
        background-color: #eee
    }

    .pure-menu-selected .pure-menu-link, .pure-menu-selected .pure-menu-link:visited {
        color: #000
    }

    .pure-table {
        border-collapse: collapse;
        border-spacing: 0;
        empty-cells: show;
        border: 1px solid #cbcbcb
    }

    .pure-table caption {
        color: #000;
        font: italic 85%/1 arial, sans-serif;
        padding: 1em 0;
        text-align: center
    }

    .pure-table td, .pure-table th {
        border-left: 1px solid #cbcbcb;
        border-width: 0 0 0 1px;
        font-size: inherit;
        margin: 0;
        overflow: visible;
        padding: .5em 1em
    }

    .pure-table td:first-child, .pure-table th:first-child {
        border-left-width: 0
    }

    .pure-table thead {
        background-color: #e0e0e0;
        color: #000;
        text-align: left;
        vertical-align: bottom
    }

    .pure-table td {
        background-color: transparent
    }

    .pure-table-odd td {
        background-color: #f2f2f2
    }

    .pure-table-striped tr:nth-child(2n-1) td {
        background-color: #f2f2f2
    }

    .pure-table-bordered td {
        border-bottom: 1px solid #cbcbcb
    }

    .pure-table-bordered tbody > tr:last-child > td {
        border-bottom-width: 0
    }

    .pure-table-horizontal td, .pure-table-horizontal th {
        border-width: 0 0 1px 0;
        border-bottom: 1px solid #cbcbcb
    }

    .pure-table-horizontal tbody > tr:last-child > td {
        border-bottom-width: 0
    }

    //Pure grids
    @media screen and (min-width: 64em) {
        .pure-u-lg-1, .pure-u-lg-1-1, .pure-u-lg-1-12, .pure-u-lg-1-2, .pure-u-lg-1-24, .pure-u-lg-1-3, .pure-u-lg-1-4, .pure-u-lg-1-5, .pure-u-lg-1-6, .pure-u-lg-1-8, .pure-u-lg-10-24, .pure-u-lg-11-12, .pure-u-lg-11-24, .pure-u-lg-12-24, .pure-u-lg-13-24, .pure-u-lg-14-24, .pure-u-lg-15-24, .pure-u-lg-16-24, .pure-u-lg-17-24, .pure-u-lg-18-24, .pure-u-lg-19-24, .pure-u-lg-2-24, .pure-u-lg-2-3, .pure-u-lg-2-5, .pure-u-lg-20-24, .pure-u-lg-21-24, .pure-u-lg-22-24, .pure-u-lg-23-24, .pure-u-lg-24-24, .pure-u-lg-3-24, .pure-u-lg-3-4, .pure-u-lg-3-5, .pure-u-lg-3-8, .pure-u-lg-4-24, .pure-u-lg-4-5, .pure-u-lg-5-12, .pure-u-lg-5-24, .pure-u-lg-5-5, .pure-u-lg-5-6, .pure-u-lg-5-8, .pure-u-lg-6-24, .pure-u-lg-7-12, .pure-u-lg-7-24, .pure-u-lg-7-8, .pure-u-lg-8-24, .pure-u-lg-9-24 {
            display: inline-block;
            zoom: 1;
            letter-spacing: normal;
            word-spacing: normal;
            vertical-align: top;
            text-rendering: auto
        }

        .pure-u-lg-1-24 {
            width: 4.1667%
        }

        .pure-u-lg-1-12, .pure-u-lg-2-24 {
            width: 8.3333%
        }

        .pure-u-lg-1-8, .pure-u-lg-3-24 {
            width: 12.5%
        }

        .pure-u-lg-1-6, .pure-u-lg-4-24 {
            width: 16.6667%
        }

        .pure-u-lg-1-5 {
            width: 20%
        }

        .pure-u-lg-5-24 {
            width: 20.8333%
        }

        .pure-u-lg-1-4, .pure-u-lg-6-24 {
            width: 25%
        }

        .pure-u-lg-7-24 {
            width: 29.1667%
        }

        .pure-u-lg-1-3, .pure-u-lg-8-24 {
            width: 33.3333%
        }

        .pure-u-lg-3-8, .pure-u-lg-9-24 {
            width: 37.5%
        }

        .pure-u-lg-2-5 {
            width: 40%
        }

        .pure-u-lg-10-24, .pure-u-lg-5-12 {
            width: 41.6667%
        }

        .pure-u-lg-11-24 {
            width: 45.8333%
        }

        .pure-u-lg-1-2, .pure-u-lg-12-24 {
            width: 50%
        }

        .pure-u-lg-13-24 {
            width: 54.1667%
        }

        .pure-u-lg-14-24, .pure-u-lg-7-12 {
            width: 58.3333%
        }

        .pure-u-lg-3-5 {
            width: 60%
        }

        .pure-u-lg-15-24, .pure-u-lg-5-8 {
            width: 62.5%
        }

        .pure-u-lg-16-24, .pure-u-lg-2-3 {
            width: 66.6667%
        }

        .pure-u-lg-17-24 {
            width: 70.8333%
        }

        .pure-u-lg-18-24, .pure-u-lg-3-4 {
            width: 75%
        }

        .pure-u-lg-19-24 {
            width: 79.1667%
        }

        .pure-u-lg-4-5 {
            width: 80%
        }

        .pure-u-lg-20-24, .pure-u-lg-5-6 {
            width: 83.3333%
        }

        .pure-u-lg-21-24, .pure-u-lg-7-8 {
            width: 87.5%
        }

        .pure-u-lg-11-12, .pure-u-lg-22-24 {
            width: 91.6667%
        }

        .pure-u-lg-23-24 {
            width: 95.8333%
        }

        .pure-u-lg-1, .pure-u-lg-1-1, .pure-u-lg-24-24, .pure-u-lg-5-5 {
            width: 100%
        }
    }
</style>
