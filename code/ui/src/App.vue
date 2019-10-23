<template>
    <!-- TODO if process.env.VUE_APP_FORCE_PASS_CHANGE -->
    <PwdChange v-if="!webmode"/>
    <Form v-else ref="formSettings">
        <Menu id="layout" :tabs="tabs" class="webmode">
            <template #header>
                <div class="heading">
                    <h1 class="hostname">{{data.device.hostname}}</h1>
                    <h2 class="desc">{{data.device.desc}}</h2>
                </div>
            </template>

            <template #footer>
                <div class="main-buttons">
                    <Btn name="update" @click="save">Save</Btn>
                    <Btn name="reconnect" color="primary" @click="reconnect">Reconnect</Btn>
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
                    GPLv3 license<br><br>
                    Version: {{data.version.app_version}}
                </div>
            </template>

            <template #status>
                <Status v-bind="data"/>
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

            <!-- #if process.env.VUE_APP_ALEXA === 'true' -->
            <template #alexa>
                <Alexa/>
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
    </Form>
</template>

<script>

    import Socket from './common/websocket';

    import PwdChange from './tabs/common/PassChange';
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

    // #if process.env.VUE_APP_ALEXA === 'true'
    import Alexa from "./tabs/integrations/Alexa";

    components.Alexa = Alexa;
    tabs.push({k: "alexa", l: "Alexa"});
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
    import {mergeDeep} from "./common/merge-deep";

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
                    version: {
                        app_version: process.env.VUE_APP_VERSION
                    },
                    device: {
                        hostname: 'ESPURNA',
                        desc: '',
                        now: Math.floor(Date.now() / 1000),
                        uptime: 0,
                        lastUpdate: 0
                    },
                },
                settings: {},
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
                this.data.device.now++;
                this.data.device.lastUpdate++;
                this.data.device.uptime++
            }, 1000);

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
                let data = evt.data;
                if (typeof data === 'string') {
                    try {
                        data = JSON.parse(evt.data.replace(/\n/g, "\\n").replace(/\r/g, "\\r").replace(/\t/g, "\\t"));
                    } catch (e) {
                        console.log('Invalid data received', evt.data);
                    }
                }
                if (typeof data === 'object') {
                    this.data = mergeDeep(this.data, data);
                    this.$set(this.device, "lastUpdate", 0);
                }
            },
            prepareData(data) {
                Object.keys(data).forEach((k) => {
                    let val = data[k];
                    if (typeof val === "object") {
                        if (Array.isArray(val.list) && "schema" in val) {
                            let objs = [];
                            val.list.forEach((v) => {
                                if (Array.isArray(v)) {
                                    let i = 0;
                                    let obj = {};
                                    v.forEach((prop) => {
                                        obj[val.schema[i++]] = prop;
                                    });
                                    objs.push(obj);
                                }
                            });
                            val.list = objs;
                        }
                    }
                });

            }
        },
        provide() {
            return {
                $app: {
                    ws: this.ws,
                    node: this
                }
            }
        }
    };

</script>

<style lang="less">
    @import "assets/Colors";

    /* -----------------------------------------------------------------------------
    General
   -------------------------------------------------------------------------- */


    @media screen and (max-width: 32em) {
        .header > h1 {
            line-height: 100%;
            font-size: 2em;
        }
    }

    .hostname {
        font-size: 1em;
    }

    .desc {
        font-size: .8em;
        font-weight: normal;
    }

    .page {
        margin-top: 10px;
    }

    .hint {
        color: #ccc;
        font-size: 80%;
        margin: -10px 0 10px 0;
        display: none;
    }

    .hint a {
        color: inherit;
    }

    .template {
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

    .right {
        text-align: right;
    }

    textarea.terminal, div.terminal, pre.terminal {
        font-family: 'Courier New', monospace;
        font-size: 80%;
        line-height: 100%;
        background-color: #333;
        color: #52ff5f;
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


    // Side menu.css
    body {
        color: #777;
    }

    /*
    The content `<div>` is where all your content goes.
    */
    .header {
        color: #333;
        text-align: left;
        /*position: sticky;*/
        border-bottom: 1px solid #eee;
        /*top: 0;*/
        //background: desaturate(@secondary, 50%);
        width: 100%;
        padding: 2em 0 1em;
        z-index: 79;
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

    html {
        font-family: sans-serif;
        -webkit-text-size-adjust: 100%
    }

    body, html, body, body > form {
        width: 100%;
        height: 100%;
    }

    body * {
        box-sizing: border-box;
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


    @media screen and (max-width: 48em) {
        .header {
            text-align: center;
            padding: 2.5em 2em 0;
        }
    }
</style>
