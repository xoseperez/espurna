<template>
    <section>
        <div class="header">
            <h1>LED</h1>
            <h2>Notification LED configuration</h2>
        </div>

        <Group v-model="led" class="page">
            <fieldset>
                <legend>Modes</legend>
                <div>
                    <ul>
                        <li>
                            <strong>WiFi status</strong> will blink at 1Hz when trying to connect. If
                            successfully connected it will briefly blink every 5 seconds if in STA mode
                            or every second if in AP mode.
                        </li>
                        <template v-if="modules.relay">
                            <li>
                                <strong>Follow switch</strong> will force the LED to follow the status of a
                                given switch (you must define which switch to follow in the side field).
                            </li>
                            <li>
                                <strong>Inverse switch</strong> will force the LED to not-follow the status
                                of a given switch (you must define which switch to follow in the side field).
                            </li>
                        </template>
                        <li>
                            <strong>Find me</strong> will turn the LED ON when all switches are OFF.
                            This is meant to locate switches at night.
                        </li>
                        <li>
                            <strong>Find me & WiFi</strong> will follow the WiFi status but will
                            stay mostly on when switches are OFF, and mostly OFF when any of them is ON.
                        </li>
                        <li>
                            <strong>Switches status</strong> will turn the LED ON whenever any switch is
                            ON, and OFF otherwise. This is global status notification.
                        </li>
                        <li>
                            <strong>Switches status & WiFi</strong> will follow the WiFi status but
                            will stay mostly off when switches are OFF, and mostly ON when any of them is ON.
                        </li>
                        <li v-if="modules.mqtt">
                            <strong>MQTT managed</strong> will let you manage the LED status via MQTT by
                            sending a message to "&lt;base_topic&gt;/led/0/set" with a payload of 0, 1
                            or 2 (to toggle it).
                        </li>
                        <li>
                            <strong>Always ON</strong> and <strong>Always OFF</strong> modes are self-explanatory.
                        </li>
                    </ul>
                </div>

                <Repeater v-model="led.list" locked>
                    <template #default="tpl">
                        <Row>
                            <C :size="2">
                                <label>LED #{{tpl.k}} mode</label>
                            </C>
                            <C :size="4">
                                <Inpt type="select" name="mode" :options="modeOptions"/>
                            </C>
                            <C :size="4">
                                <Inpt v-if="tpl.value.mode == 2 || tpl.value.mode == 3" name="relay" type="select"
                                      :options="relayOptions"/>
                            </C>
                        </Row>
                    </template>
                </Repeater>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Repeater from "../../components/Repeater";
    import C from "../../layout/Col";
    import Row from "../../layout/Row";
    import Group from "../../components/Group";

    export default {
        components: {
            Group,
            Row,
            C,
            Repeater,
            Inpt
        },
        inheritAttrs: false,
        props: {
            modules: {
                type: Object,
                default: () => ({})
            },
            led: {
                type: Object,
                default: () => ({})
            },
            relayOptions: {
                type: Array
            }
        },
        computed: {
            modeOptions() {
                let options = [
                    {k: 1, l: "WiFi status"},
                    {k: 4, l: "Find me"},
                    {k: 5, l: "Find me & WiFi"},
                    {k: 8, l: "Switches status"},
                    {k: 9, l: "Switches & WiFi"},
                    {k: 6, l: "Always ON"},
                    {k: 7, l: "Always OFF"},
                ];

                if (this.modules.relay) {
                    options.push({k: 2, l: "Follow switch"});
                    options.push({k: 3, l: "Inverse switch"})
                }

                if (this.modules.mqtt) {
                    options.push({k: 0, l: "MQTT managed"})
                }

                return options;
            }
        }
    }
</script>

<style lang="less">

</style>