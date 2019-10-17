<template>
    <div>
        <!-- TODO if process.env.VUE_APP_FORCE_PASS_CHANGE -->
        <PwdChange v-if="!webmode"/>
        <Form v-else ref="formSettings">
            <Menu id="layout" :tabs="tabs" class="webmode">
                <template #header>
                    <div class="heading">
                        <span class="hostname">{{data.hostname}}</span>
                        <span class="small title">ESPurna {{data.version.app_version}}</span>
                        <span class="small desc">{{data.desc}}</span>
                    </div>
                </template>

                <template #footer>
                    <div class="main-buttons">
                        <Btn name="update" @click="save">Save</Btn>
                        <Btn name="reconnect" color="accent" @click="reconnect">Reconnect</Btn>
                        <Btn name="reboot" color="danger" @click="reboot">Reboot</Btn>
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
                    <Status :status="status"/>
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
                    <Led :leds="{}"/>
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
                    <Lfox :buttons="{}" :relay-options="relayOptions"/>
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
                    <Inpt type="switch" name="schUTC"/>
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
                    <Inpt type="switch" name="schEnabled"/>
                </div>

                <div class="pure-u-1 pure-u-lg-1-2"></div>
                <Btn name="del-schedule" color="danger">Delete schedule</Btn>
            </div>

            <iframe id="downloader"></iframe>
            <input id="uploader" type="file"/>
        </div>
    </div>
</template>

<script>

    import Socket from './common/websocket';

    import PwdChange from './components/PassChange';
    import Inpt from './components/Input';
    import Menu from './components/Menu';
    import Form from './components/Form';
    import A from './components/ExtLink';
    import Btn from './components/Button';


    import Mqtt from "./tabs/common/Mqtt";
    import Admin from "./tabs/common/Admin";
    import General from "./tabs/common/General";
    import Status from "./tabs/common/Status";

    let tabs = [
        //Basic settings
        {k: "status", l: "Status"}, //Move debug to status
        {k: "general", l: "General"}, //Move wifi to general
        {k: "admin", l: "Admin"}, //Move ntp to admin
        {k: "mqtt", l: "MQTT"},
        {k: "separator"}
    ];
    let components = {A, Inpt, Menu, Form, Mqtt, Admin, General, Status, Btn, PwdChange};

    //Board Features

    // #if process.env.VUE_APP_THERMOSTAT === 'true'
    import Tstat from "./tabs/features/Thermostat";

    components.Tstat = Tstat;
    tabs.push({k: "thermostat", l: "Thermostat"});
    // #endif

    // #if process.env.VUE_APP_LED === 'true'
    import Led from "./tabs/features/Led";

    components.Led = Led;
    tabs.push({k: "led", l: "LED"});
    // #endif

    // #if process.env.VUE_APP_LIGHT === 'true'
    import Color from "./tabs/features/Color";

    components.Color = Color;
    tabs.push({k: "color", l: "Lights"}); //Moved color schedules here
    // #endif

    // #if process.env.VUE_APP_RFM69 === 'true'
    import Rfm69 from "./tabs/features/Rfm69";

    components.Rfm69 = Rfm69;
    tabs.push({k: "rfm69", l: "RFM69 Mapping"}); //Moved messages and mapping here
    // #endif

    // #if process.env.VUE_APP_RFBRIDGE === 'true'
    import Rfb from "./tabs/features/Rfb";

    components.Rfb = Rfb;
    tabs.push({k: "rfb", l: "RF Bridge"});
    // #endif

    // #if process.env.VUE_APP_SENSOR === 'true'
    import Sns from "./tabs/features/Sensors";

    components.Sns = Sns;
    tabs.push({k: "sns", l: "Sensors"});
    // #endif

    // #if process.env.VUE_APP_RELAYS === 'true'
    import Relays from "./tabs/features/Relays";

    components.Relays = Relays;
    tabs.push({k: "relays", l: "Switches"}); //Moved schedules to switches
    // #endif

    tabs.push({k: "separator"});

    //Integrations
    // #if process.env.VUE_APP_HA === 'true'
    import Ha from "./tabs/integrations/HomeAssistant";

    components.Ha = Ha;
    tabs.push({k: "ha", l: "Home Assistant"});
    // #endif

    // #if process.env.VUE_APP_LIGHTFOX === 'true'
    import Lfox from "./tabs/features/LightFox";

    components.Lfox = Lfox;
    tabs.push({k: "lightfox", l: "LightFox"});
    // #endif

    // #if process.env.VUE_APP_DCZ === 'true'
    import Dcz from "./tabs/integrations/Domoticz";

    components.Dcz = Dcz;
    tabs.push({k: "dcz", l: "Domoticz"});
    // #endif

    // #if process.env.VUE_APP_NOFUSS === 'true'
    import Nfss from "./tabs/integrations/NoFuss";

    components.Nfss = Nfss;
    tabs.push({k: "nofuss", l: "NoFuss"});
    // #endif

    // #if process.env.VUE_APP_THINGSPEAK === 'true'
    import Tspk from "./tabs/integrations/ThingSpeak";

    components.Tspk = Tspk;
    tabs.push({k: "thingspeak", l: "ThingSpeak"});
    // #endif

    // #if process.env.VUE_APP_IDB === 'true'
    import Idb from "./tabs/integrations/InfluxDB";

    components.Idb = Idb;
    tabs.push({k: "idb", l: "InfluxDB"});
    // #endif

    let messages = ["",
        "Remote update started",
        "OTA update started",
        "Error parsing data!",
        "The file does not look like a valid configuration backup or is corrupted",
        "Changes saved. You should reboot your board now",
        "Passwords do not match!",
        "Changes saved",
        "No changes detected",
        "Session expired, please reload page..."
    ];


    export default {
        components,
        data() {
            return {
                webmode: true,
                ws: new Socket,
                data: {
                    version: {},
                    relays: {},
                    wifi: {},
                    device: {},
                },
                tabs: tabs
            }
        },
        computed: {
            relayOptions() {
                let options = [];
                if ("relays" in this.data) {
                    for (let i = 0; i < this.data.relays.length; ++i) {
                        options.push("Switch #" + i);
                    }
                }
                return options;
            },
            lightOptions() {
                let options = [];
                if ("light" in this.data) {
                    for (let i = 0; i < this.data.light.num_channel; ++i) {
                        options.push("Channel #" + i);
                    }
                }
                return options;
            },
        },
        mounted() {
            setInterval(() => {
                this.data.now++;
                this.data.ago++;
                this.data.uptime++
            }, 999);

            // Check host param in query string
            var search = new URLSearchParams(window.location.search),
                host = search.get("host");

            this.ws.connect(host ? host : window.location.host, this.receiveMessage);
        },
        methods: {
            save() {
                if (this.$refs.formSettings.reportValidity()) {
                    this.sendConfig(this.$refs.formSettings.values);
                }
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
            },
            receiveMessage(evt) {
                try {
                    let data = JSON.parse(evt.data.replace(/\n/g, "\\n").replace(/\r/g, "\\r").replace(/\t/g, "\\t"));
                    if (data) {
                        this.data = {...this.data, ...data};
                    }
                } catch (e) {
                    console.log('Invalid data received', evt.data);
                }
            }
        },
        provide() {
            return {
                $app: {
                    ws: this.ws,
                }
            }
        }
    };

</script>

<style lang="less">
    /* -----------------------------------------------------------------------------
    General
   -------------------------------------------------------------------------- */


    .pure-g {
        margin-bottom: 0;
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

    .template {
        display: none;
    }

    input[name=upgrade] {
        display: none;
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

    .terminal {
        font-family: 'Courier New', monospace;
        font-size: 80%;
        line-height: 100%;
        background-color: #000;
        color: #0F0;
    }

    /* -----------------------------------------------------------------------------
        Buttons
       -

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
        Loading
       -------------------------------------------------------------------------- */

    .loading {
        background-image: url('~@/assets/loading.gif');
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

    .menu .footer {
        color: #999;
        font-size: 80%;
        padding: 10px;
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

    /*
    The content `<div>` is where all your content goes.
    */
    #content {
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

    b, strong {
        font-weight: 700
    }

    h1 {
        font-size: 2em;
        margin: .67em 0
    }

    img {
        border: 0
    }

    hr {
        box-sizing: content-box;
        height: 0
    }

    pre {
        overflow: auto
    }

    code, pre {
        font-family: monospace;
        font-size: 1em
    }

    button, input, select, textarea {
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

    input {
        line-height: normal
    }

    input[type=checkbox], input[type=radio] {
        box-sizing: border-box;
        padding: 0
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

    @media (min-width: 48em) {
        /* Only apply this when the window is small. Otherwise, the following
        case results in extra padding on the left:
            * Make the window small.
            * Tap the menu to trigger the active state.
            * Make the window large again.
        */
        #layout {
            padding-left: 160px;
        }
    }

    @media (max-width: 48em) {
        #layout {
            left: 160px;
        }
    }
</style>
