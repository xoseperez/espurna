<template>
    <section>
        <div class="header">
            <h1>HOME ASSISTANT</h1>
            <h2>
                Add this device to your Home Assistant.
            </h2>
        </div>

        <Group v-model="ha" class="page form" #default>
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

                <template v-if="modules.mqtt">
                    <legend>Configuration</legend>

                    <p>
                        These are the settings you should copy to your Home Assistant "configuration.yaml" file. If any
                        of the sections below (switch, light, sensor) already exists, do not duplicate it, simply copy
                        the contents of the section below the ones already present.
                    </p>
                    <Row>
                        <textarea :value="config" class="terminal"
                                  wrap="soft"
                                  readonly></textarea>
                    </Row>
                </template>
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
    import ws from "../../common/websocket";
    import objectToYaml from "../../common/objectToYaml";

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
            modules: Object,
            ha: Object,
            relay: Object,
            mqtt: Object,
            sns: Object,
        },
        computed: {
            config() {
                let s = "";
                const yaml = {};
                if (this.relay && this.relay.list.length) {

                    yaml.switch = [];

                    this.relay.list.forEach((v, i) => {
                        yaml.switch.push({
                            name: v.name,
                            platform: "mqtt",
                            state_topic: this.mqttTopic(this.ha.relayTopic, i),
                            command_topic: this.mqttTopic(this.ha.relayTopic, i, true),
                            payload_on: this.yamlEscape(this.relay.payloadOn),
                            payload_off: this.yamlEscape(this.relay.payloadOff),
                            availability_topic: this.mqttTopic(this.ha.statusTopic),
                            payload_available: this.yamlEscape(this.mqtt.payloadOnline),
                            payload_not_available: this.yamlEscape(this.mqtt.payloadOffline)
                        });
                        if ()
                    });

                    /*
                    todo
                        config["brightness_state_topic"] = mqttTopic(MQTT_TOPIC_BRIGHTNESS, false);
                        config["brightness_command_topic"] = mqttTopic(MQTT_TOPIC_BRIGHTNESS, true);

                        if (lightHasColor()) {
                            config["rgb_state_topic"] = mqttTopic(MQTT_TOPIC_COLOR_RGB, false);
                            config["rgb_command_topic"] = mqttTopic(MQTT_TOPIC_COLOR_RGB, true);
                        }
                        if (lightHasColor() || lightUseCCT()) {
                            config["color_temp_command_topic"] = mqttTopic(MQTT_TOPIC_MIRED, true);
                            config["color_temp_state_topic"] = mqttTopic(MQTT_TOPIC_MIRED, false);
                        }

                        if (lightChannels() > 3) {
                            config["white_value_state_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, false);
                            config["white_value_command_topic"] = mqttTopic(MQTT_TOPIC_CHANNEL, 3, true);
                        }
                     */
                }

                if (this.sns.magnitudes && this.sns.magnitudes.list.length) {
                    s += "sensor:\n";
                    this.sns.magnitudes.list.forEach((v) => {
                        s += "  - name: " + this.topic + v.name + "\n" +
                            "    platform: mqtt\n" +
                            "    state_topic: " + this.topic + "/" + v.name + "\n" +
                            "    unit_of_measurement: " + v.unit + "\n\n";
                    });

                }
                return objectToYaml(yaml);
            }
        },
        methods: {
            mqttTopic(category, index, set) {
                let topic = this.mqtt.topic;

                if (category) {
                    topic += "/" + category;
                }

                if (index) {
                    topic += "/" + i;
                }

                if (set) {
                    topic += this.mqtt.setter;
                } else {
                    topic += this.mqtt.getter;
                }
            }
        },
    };
</script>

<style lang="less">

</style>
