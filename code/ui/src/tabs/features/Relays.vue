<template>
    <section>
        <div class="header">
            <h1>SWITCHES</h1>
            <h2>Switch / relay configuration</h2>
        </div>

        <div class="page form">
            <fieldset>
                <legend class="module module-multirelay">General</legend>

                <div class="pure-g module module-multirelay">
                    <label class="pure-u-1 pure-u-lg-1-4">Switch sync mode</label>
                    <select name="relaySync">
                        <option value="0">No synchronisation</option>
                        <option value="1">Zero or one switches active</option>
                        <option value="2">One and just one switch active</option>
                        <option value="3">All synchronised</option>
                        <option value="4">Switch #0 controls other switches</option>
                    </select>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Define how the different switches should be
                        synchronized.
                    </Hint>
                </div>

                <div id="relayConfig"></div>
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
                    <template v-if="modules.mqtt">
                        <div class="pure-g module module-mqtt">
                            <div class="pure-u-1 pure-u-lg-1-4"><label>MQTT group</label></div>
                            <div class="pure-u-1 pure-u-lg-3-4">
                                <Inpt name="mqttGroup"
                                      class="pure-u-1"
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
                            <Inpt type="select" class="pure-u-1 pure-u-lg-3-4" name="relayOnDisc"
                                  :options="[
                                      'Don\'t change',
                                      'Turn the switch OFF',
                                      'Turn the switch ON'
                                  ]"/>
                        </div>
                    </template>
                </div>
            </fieldset>
            <fieldset v-if="modules.sch">
                <legend>SCHEDULES</legend>
                <div>Turn switches ON and OFF based on the current time.</div>
                <Repeater v-model="schedule.list" :max="schedule.max">
                    <template #default="tpl">
                        <Row>
                            <C><label>When time is</label></C>
                            <C no-wrap>
                                <Inpt name="hour"
                                      type="number"
                                      min="0"
                                      step="1"
                                      max="23"
                                      :default="0" unit="h"/>
                                <Inpt name="minute"
                                      type="number"
                                      min="0"
                                      step="1"
                                      max="59"
                                      :default="0" unit="m"/>
                            </C>

                            <C><label>Use UTC time</label></C>
                            <C>
                                <Inpt type="switch" name="UTC"/>
                            </C>

                            <C><label>And weekday is one of</label></C>
                            <C>
                                <Inpt type="select"
                                      multiple
                                      name="weekdays"
                                      maxlength="15"
                                      :default="[1,2,3,4,5,6,7]"
                                      :options="daysOfWeek"/>
                            </C>

                            <C><label>Action</label></C>
                            <C no-wrap>
                                <Inpt type="select" name="action" :options="['Turn OFF', 'Turn ON', 'Toggle']"/>
                                <Inpt type="select" name="relay" :options="relayOptions"/>
                                <Inpt type="hidden" name="type" value="1"/>
                            </C>

                            <C><label>Enabled</label></C>
                            <C>
                                <Inpt type="switch" name="enabled"/>
                            </C>
                        </Row>
                    </template>
                    <template #btnRemove="tpl">
                        <Btn name="del-schedule" color="danger" @click="tpl.click">Delete schedule</Btn>
                    </template>
                    <template #btnAdd="tpl">
                        <Btn name="add-switch-schedule" @click="tpl.click">Add switch schedule</Btn>
                    </template>
                </Repeater>
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';
    import Btn from "../../components/Button";
    import Repeater from "../../components/Repeater";
    import Hint from "../../components/Hint";
    import C from "../../layout/Col";
    import Row from "../../layout/Row";

    export default {
        components: {
            Row,
            C,
            Hint,
            Repeater,
            Inpt,
            Btn
        },
        inheritAttrs: false,
        props: {
            relay: {
                type: Object,
                default: () => ({})
            },
            modules: {
                type: Object,
                default: () => ({})
            },
            schedule: {
                type: Object,
                default: () => ({})
            }
        },
        computed: {
            relayOptions() {
                let options = [];
                if (this.relay.config) {
                    for (let i = 0; i < this.relay.config.list.length; ++i) {
                        options.push("Switch #" + i);
                    }
                }
                return options;
            },
            daysOfWeek() {
                let days = [];

                let d = new Date();
                for (let i = 0; i < 7; i++) {
                    d.setDate(i);
                    let s = d.toLocaleString(navigator.language, {weekday: 'long'});
                    days.push({k: i + 1, l: s});
                }

                return days;
            }
        }
    }
</script>

<style lang="less">

</style>