<template>
    <section>
        <div class="header">
            <h1>HOME ASSISTANT</h1>
            <h2>
                Add this device to your Home Assistant.
            </h2>
        </div>

        <Group v-model="ha" class="page form">
            <fieldset>
                <legend>Discover</legend>

                <Row>
                    <C><label>Discover</label></C>
                    <C>
                        <Inpt type="switch"
                              name="enabled"
                              tabindex="1"/>
                        <Hint v-if="modules.mqtt || modules.light">
                            <div v-if="modules.mqtt">
                                <strong>WARNING! </strong>Incompatible with Home Assistant MQTT integration
                            </div>
                            <div v-if="modules.light">
                                Use CSS style to report colors to MQTT and REST API. <br> Red will be reported as
                                "#FF0000" if ON, otherwise "255,0,0"
                            </div>
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>Prefix</label></C>
                    <C>
                        <Inpt name="prefix" type="text" tabindex="2"/>
                    </C>
                </Row>

                <legend>Configuration</legend>

                <p>
                    These are the settings you should copy to your Home Assistant "configuration.yaml" file. If any of
                    the sections below (switch, light, sensor) already exists, do not duplicate it, simply copy the
                    contents of the section below the ones already present.
                </p>
                <Row>
                    <textarea :value="config" class="terminal"
                              wrap="soft"
                              readonly></textarea>
                </Row>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Hint from "../../components/Hint";
    import Row from "../../layout/Row";
    import C from "../../layout/Col";
    import Group from "../../components/Group";

    export default {
        components: {
            Group,
            C,
            Row,
            Hint,
            Inpt
        },
        inheritAttrs: false,
        props: {
            ha: {
                type: Object,
                default: () => ({})
            },
            relays: {
                type: Object,
                default: () => ({list: []})
            },
            sns: {
                type: Object,
                default: () => ({})
            },
        },
        computed: {
            config() {
                let s = '';

                if (this.relays.config && this.relays.config.list.length) {
                    s += 'switch:\n';

                    this.relays.config.list.forEach((v, i) => {
                        s += '  - name: ' + v.name + '\n' +
                            '    platform: mqtt\n' +
                            '    state_topic: ' + this.topic + '/relay/' + i + '\n' +
                            '    command_topic: ' + this.topic + '/relay/' + i + '/set\n' +
                            '    payload_on: 1\n' +
                            '    payload_off: 0\n' +
                            '    availability_topic: ' + this.topic + '/status\n' +
                            '    payload_available: 1\n' +
                            '    payload_not_available: 0\n\n';
                    });
                }

                if (this.sns.magnitudes && this.sns.magnitudes.list.length) {
                    s += 'sensor:\n';
                    this.sns.magnitudes.list.forEach((v) => {
                        s += '  - name: ' + this.topic + v.name + '\n' +
                            '    platform: mqtt\n' +
                            '    state_topic: ' + this.topic + '/' + v.name + '\n' +
                            '    unit_of_measurement: ' + v.unit + '\n\n'
                    });

                }
                return s;
            }
        }
    }
</script>

<style lang="less">

</style>
