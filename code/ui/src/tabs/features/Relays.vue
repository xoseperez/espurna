<template>
    <section>
        <div class="header">
            <h1>SWITCHES</h1>
            <h2>Switch / relay configuration</h2>
        </div>

        <Group v-model="relay.config" class="page">
            <fieldset>
                <template v-if="relay.config.list.length > 1">
                    <legend>General</legend>

                    <Row>
                        <C><label>Switch sync mode</label></C>
                        <C>
                            <Inpt type="select" name="sync" :options="[
                                'No synchronisation',
                                'Zero or one switches active',
                                'One and just one switch active',
                                'All synchronised',
                                'Switch #0 controls all other switches'
                            ]"/>
                            <Hint>
                                Define how the different switches should be synchronized.
                            </Hint>
                        </C>
                    </Row>
                </template>

                <Repeater v-model="relay.config.list" locked class="switches">
                    <template #default="tpl">
                        <legend>Switch #{{tpl.k}} ({{tpl.value.gpio}})</legend>
                        <Row>
                            <C>
                                <Row>
                                    <C><label>Switch name</label></C>
                                    <C>
                                        <Inpt type="text" name="name"/>
                                        <Hint>
                                            A name to distinguish the different relays
                                        </Hint>
                                    </C>
                                </Row>
                                <Row>
                                    <C><label>Boot mode</label></C>
                                    <C>
                                        <Inpt type="select" name="boot"
                                              :options="['Always OFF', 'Always ON', 'Same as before', 'Toggle before', 'Locked OFF', 'Locked ON']"/>
                                    </C>
                                </Row>
                            </C>
                            <C>
                                <Row>
                                    <C><label>Pulse mode</label></C>
                                    <C>
                                        <Inpt type="select" name="pulse"
                                              :options="['Don\'t pulse', 'Normally OFF', 'Normally ON']"/>
                                    </C>
                                </Row>
                                <Row>
                                    <C><label>Pulse time</label></C>
                                    <C>
                                        <Inpt name="time"
                                              type="number"
                                              min="0"
                                              step="0.1"
                                              max="3600"
                                              unit="s"/>
                                    </C>
                                </Row>
                            </C>
                        </Row>

                        <Row>
                            <C>
                                <Row>
                                    <C><label>Restore last schedules</label></C>
                                    <C>
                                        <Inpt name="lastSch"
                                              type="switch"/>
                                    </C>
                                </Row>
                            </C>
                            <C>
                                <Row v-if="modules.mqtt">
                                    <C><label>On MQTT disconnect</label></C>
                                    <C>
                                        <Inpt type="select" name="on_disc"
                                              :options="[
                                                  'Don\'t change',
                                                  'Turn the switch OFF',
                                                  'Turn the switch ON'
                                              ]"/>
                                    </C>
                                </Row>
                            </C>
                        </Row>
                        <template v-if="modules.mqtt">
                            <Row>
                                <C>
                                    <Row>
                                        <C><label>MQTT group</label></C>
                                        <C>
                                            <Inpt name="group"
                                                  data="0"
                                                  action="reconnect"/>
                                        </C>
                                    </Row>
                                </C>
                                <C>
                                    <Row v-if="tpl.value.group">
                                        <C><label>MQTT group sync</label></C>
                                        <C>
                                            <Inpt type="select" name="groupSync"
                                                  :options="['Same','Inverse', 'Receive Only']"/>
                                        </C>
                                    </Row>
                                </C>
                            </Row>
                        </template>
                    </template>
                </Repeater>
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
                                      :default="12" unit="h"/>
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
                            <C no-wrap stretch>
                                <Inpt type="select" name="action" :options="['Turn OFF', 'Turn ON', 'Toggle']"
                                      placeholder="Select an action"/>
                                <Inpt type="select" name="relay" :options="relayOptions" placeholder="Select a switch"/>
                                <Inpt type="hidden" name="type" value="1"/>
                            </C>

                            <C><label>Enabled</label></C>
                            <C>
                                <Inpt type="switch" name="enabled" :default="true"/>
                            </C>
                        </Row>
                    </template>
                    <template #btnRemove="tpl">
                        <Btn name="del-schedule" color="danger" @click="tpl.click">
                            Delete schedule
                        </Btn>
                    </template>
                    <template #btnAdd="tpl">
                        <Btn name="add-switch-schedule" @click="tpl.click">Add switch schedule</Btn>
                    </template>
                </Repeater>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Btn from "../../components/Button";
    import Repeater from "../../components/Repeater";
    import Hint from "../../components/Hint";
    import C from "../../layout/Col";
    import Row from "../../layout/Row";
    import Group from "../../components/Group";
    import capitalize from "../../common/capitalize";

    export default {
        components: {
            Group,
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
            modules: Object,
            schedule: {
                type: Object,
                default: () => ({})
            },
            relayOptions: {
                type: Array
            }
        },
        computed: {
            daysOfWeek() {
                let days = [];

                let d = new Date(0);
                for (let i = 0; i < 7; ++i) {
                    d.setDate(i + 5);
                    let s = d.toLocaleString(navigator.language, {weekday: "long"});
                    s = capitalize(s);
                    days.push({k: i + 1, l: s});
                }

                return days;
            }
        }
    };
</script>

<style lang="less">
    .switches .col .col:nth-of-type(odd) {
        text-align: right;
    }
</style>
