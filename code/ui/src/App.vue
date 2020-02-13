<template>
    <!-- TODO if process.env.VUE_APP_FORCE_PASS_CHANGE -->
    <Setup v-if="!webmode"/>
    <Form v-else ref="formSettings" v-model="data">
        <Menu id="layout" :tabs="tabs" class="webmode">
            <template #header>
                <div class="heading">
                    <template>
                        <i v-if="close" class="back" @click="close">🡠</i>
                        <div class="clearfix"></div>
                    </template>
                    <Icon class="icon"/>
                    <h1 class="hostname">{{data.device.hostname}}</h1>
                    <h2 class="desc">{{data.device.desc}}</h2>
                    <div class="clearfix"></div>
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
                <General v-bind="data"/>
            </template>

            <template #admin>
                <Admin v-bind="data"/>
            </template>

            <template v-if="data.modules.mqtt" #mqtt>
                <Mqtt v-bind="data"/>
            </template>

            <!-- #!if THERMOSTAT === true -->
            <template v-if="data.modules.thermostat" #thermostat>
                <Tstat v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if LED === true -->
            <template v-if="data.modules.led" #led>
                <Led v-bind="data" :relay-options="relayOptions"/>
            </template>
            <!-- #!endif -->

            <!-- #!if LIGHT === true -->
            <template v-if="data.modules.color" #color>
                <Color v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if RFM69 === true -->
            <template v-if="data.modules.rfm69" #rfm69>
                <Rfm69 v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if RFBRIDGE === true -->
            <template v-if="data.modules.rfb" #rfb>
                <Rfb v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if SENSOR === true -->
            <template v-if="data.modules.sns" #sns>
                <Sns v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if RELAYS === true -->
            <template v-if="data.modules.relay" #relays>
                <Relays v-bind="data" :relay-options="relayOptions"/>
            </template>
            <!-- #!endif -->

            <!-- #!if LIGHTFOX === true -->
            <template v-if="data.modules.lightfox" #lightfox>
                <Lfox v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if DCZ === true -->
            <template v-if="data.modules.dcz" #dcz>
                <Dcz v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if HA === true -->
            <template v-if="data.modules.ha" #ha>
                <Ha v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if ALEXA === true -->
            <template v-if="data.modules.alexa" #alexa>
                <Alexa v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if THINGSPEAK === true -->
            <template v-if="data.modules.tspk" #thingspeak>
                <Tspk v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if IDB === true -->
            <template v-if="data.modules.idb" #idb>
                <Idb v-bind="data"/>
            </template>
            <!-- #!endif -->

            <!-- #!if NOFUSS === true -->
            <template v-if="data.modules.nofuss" #nofuss>
                <Nfss v-bind="data"/>
            </template>
            <!-- #!endif -->
        </Menu>
        <iframe id="downloader"></iframe>
    </Form>
</template>

<script>

    import Socket from './common/websocket';
    import Icon from '../public/icons/icon.svg';
    import capitalize from './common/capitalize';

    import Setup from './tabs/common/Setup';
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
    let components = {A, Icon, Inpt, Menu, Form, Mqtt, Admin, General, Status, Btn, Setup};

    //Board Features

    // #!if THERMOSTAT === true
    import Tstat from "./tabs/features/Thermostat";

    components.Tstat = Tstat;
    tabs.push({k: "thermostat", l: "Thermostat"});
    // #!endif

    // #!if LED === true
    import Led from "./tabs/features/Led";

    components.Led = Led;
    tabs.push({k: "led", l: "LED"});
    // #!endif

    // #!if LIGHT === true
    import Color from "./tabs/features/Color";

    components.Color = Color;
    tabs.push({k: "color", l: "Lights"}); //Moved color schedules here
    // #!endif

    // #!if RFM69 === true
    import Rfm69 from "./tabs/features/Rfm69";

    components.Rfm69 = Rfm69;
    tabs.push({k: "rfm69", l: "RFM69 Mapping"}); //Moved messages and mapping here
    // #!endif

    // #!if RFBRIDGE === true
    import Rfb from "./tabs/features/Rfb";

    components.Rfb = Rfb;
    tabs.push({k: "rfb", l: "RF Bridge"});
    // #!endif

    // #!if SENSOR === true
    import Sns from "./tabs/features/Sensors";

    components.Sns = Sns;
    tabs.push({k: "sns", l: "Sensors"});
    // #!endif

    // #!if RELAYS === true
    import Relays from "./tabs/features/Relays";

    components.Relays = Relays;
    tabs.push({k: "relays", l: "Switches"}); //Moved schedules to switches
    // #!endif

    tabs.push({k: "separator"});

    //Integrations
    // #!if HA === true
    import Ha from "./tabs/integrations/HomeAssistant";

    components.Ha = Ha;
    tabs.push({k: "ha", l: "Home Assistant"});
    // #!endif

    // #!if ALEXA === true
    import Alexa from "./tabs/integrations/Alexa";

    components.Alexa = Alexa;
    tabs.push({k: "alexa", l: "Alexa"});
    // #!endif

    // #!if LIGHTFOX === true
    import Lfox from "./tabs/features/LightFox";

    components.Lfox = Lfox;
    tabs.push({k: "lightfox", l: "LightFox"});
    // #!endif

    // #!if DCZ === true
    import Dcz from "./tabs/integrations/Domoticz";

    components.Dcz = Dcz;
    tabs.push({k: "dcz", l: "Domoticz"});
    // #!endif

    // #!if NOFUSS === true
    import Nfss from "./tabs/integrations/NoFuss";

    components.Nfss = Nfss;
    tabs.push({k: "nofuss", l: "NoFuss"});
    // #!endif

    // #!if THINGSPEAK === true
    import Tspk from "./tabs/integrations/ThingSpeak";

    components.Tspk = Tspk;
    tabs.push({k: "thingspeak", l: "ThingSpeak"});
    // #!endif

    // #!if IDB === true
    import Idb from "./tabs/integrations/InfluxDB";
    import {detailedDiff} from "deep-object-diff";

    components.Idb = Idb;
    tabs.push({k: "idb", l: "InfluxDB"});
    // #!endif

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

    function isObject(item) {
        return (item && typeof item === 'object' && !Array.isArray(item));
    }

    export default {
        components,
        props: {
            address: {
                type: String,
                required: false
            },
            close: {
                type: Function,
                required: false
            }
        },
        data() {
            return {
                webmode: true,
                ws: new Socket,
                original: {},
                data: {
                    modules: {},
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
                    relays: {}
                },
                settings: {},
                tabs: tabs
            }
        },
        computed: {
            lightOptions() {
                let options = [];
                if ("light" in this.data) {
                    for (let i = 0; i < this.data.light.num_channel; ++i) {
                        options.push({k: i, l: "Channel #" + i});
                    }
                }
                return options;
            },
            relayOptions() {
                let options = [];
                if (this.data.relays.config) {
                    for (let i = 0; i < this.data.relays.config.list.length; ++i) {
                        options.push({k: i, l: "Switch #" + i});
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

            if (!this.address) {
                // Check host param in query string
                const search = new URLSearchParams(window.location.search),
                    host = search.get("host");
                this.ws.connect(host ? host : window.location.host, this.receiveMessage);
            } else {
                this.ws.connect(this.address, this.receiveMessage);
            }
        },
        methods: {
            save() {
                if (this.$refs.formSettings.reportValidity()) {
                    const diff = detailedDiff(this.original, this.data);

                    let config = {};

                    Object.keys(diff).forEach((k) => {
                        Object.assign(config, this.flatten(diff[k]));
                    });

                    this.sendConfig(config);
                }
            },
            flatten(obj, key, flat) {
                flat = typeof flat === 'undefined' ? {} : flat;

                key = key || '';

                Object.keys(obj).forEach((k) => {
                    const v = obj[k];
                    k = capitalize(k);
                    if (typeof v === 'object') {
                        this.flatten(v, key + k, flat);
                    } else {
                        flat[key + k] = v;
                    }
                });
                return flat;
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
                if (data && typeof data === 'object') {
                    this.prepareData(this.data, data);
                    this.data.device.lastUpdate = 0;
                }
            },
            prepareData(target, source) {
                Object.keys(source).forEach((k) => {
                    let val = source[k];

                    if (isObject(val)) {
                        if (!target[k]) {
                            this.$set(target, k, {});
                        }

                        if (val.schema && Array.isArray(val.list)) {
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

                            if (val.start) {
                                val.list = [...target.list];
                                val.list.splice(val.start, objs.length, ...objs);
                            } else {
                                val.list = objs;
                            }
                        }

                        this.prepareData(target[k], val);
                    } else {
                        this.$set(target, k, val);
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
    @import "assets/Loaders";

    /* -----------------------------------------------------------------------------
    General
   -------------------------------------------------------------------------- */
    .icon {
        width: 30px;
        margin: 7px 10px 7px 0;
        height: auto;
        float: left;

        path {
            fill: #fff;
        }
    }

    .clearfix {
        clear: both;
    }

    @media screen and (max-width: 32em) {
        .header > h1 {
            line-height: 100%;
            font-size: 2em;
        }
    }

    .align-right {
        text-align: right;

        &.pd {
            padding-right: 5px;
        }
    }

    .align-left {
        text-align: left;

        &.pd {
            padding-left: 5px;
        }
    }

    .center {
        text-align: center;

        &.pd {
            padding-right: 5px;
            padding-left: 5px;
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

    .align-right {
        text-align: right;
    }

    textarea.terminal, div.terminal, pre.terminal {
        font-family: 'Courier New', monospace;
        font-size: 80%;
        background-color: #333;
        color: #52ff5f;
        line-height: 1.2em;
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
        border-bottom: 1px solid #eee;
        width: 100%;
        padding: 2em 0 1em;
        z-index: 79;
        text-align: center;
    }

    .page + .header {
        margin-top: 40px;
        border-top: 1px solid #eee;
    }

    .header h1 {
        margin: 0.2em 0;
        font-size: 2.6em;
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
        overflow: auto;
        width: 100% !important;
        height: 200px;
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

    i.back {
        cursor: pointer;
        font-style: normal;
        font-size: 1.5em;
        position: absolute;
        display: block;
        color: white;
        top: 0;
        right: 0;
        width: 40px;
        height: 40px;
        text-align: center;
        line-height: 38px;
        border-color: darken(@primary, 15%);
        border-style: solid;
        border-width: 0 0 3px 0;
        border-radius: 0 0 0 5px;
        background: @primary;
    }

    @media screen and (max-width: 48em) {
        .header {
            text-align: center;
            padding: 2.5em 2em 0;
        }
    }
</style>
