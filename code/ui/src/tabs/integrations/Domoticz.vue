<template>
    <section>
        <div class="header">
            <h1>DOMOTICZ</h1>
            <h2>
                Configure the connection to your Domoticz server.
            </h2>
        </div>

        <Group v-model="dcz" class="page form">
            <fieldset>
                <legend>General</legend>

                <Row>
                    <C><label>Enable Domoticz</label></C>
                    <C>
                        <Inpt type="switch"
                              name="enabled"
                              tabindex="1"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Domoticz IN Topic</label></C>
                    <C>
                        <Inpt name="topicIn" type="text" tabindex="2"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Domoticz OUT Topic</label></C>
                    <C>
                        <Inpt name="topicOut"
                              type="text"
                              action="reconnect"
                              tabindex="3"/>
                    </C>
                </Row>

                <legend>Sensors & actuators</legend>

                <Row>
                    <Hint>
                        Set IDX to 0 to disable notifications from that component.
                    </Hint>
                </Row>

                <!-- #!if RELAYS === true -->
                <Repeater v-model="relays.config.list" locked>
                    <template #default="tpl">
                        <Row>
                            <C><label>Switch #{{tpl.k}}</label></C>
                            <C>
                                <Inpt name="relayIdx"
                                      type="number"
                                      min="0"/>
                            </C>
                        </Row>
                    </template>
                </Repeater>
                <!-- #!endif -->

                <!-- #!if SENSOR === true -->
                <Repeater v-model="sns.magnitudes.list" locked>
                    <template #default="tpl">
                        <Row>
                            <C><label>Magnitude {{tpl.value.name}}</label></C>
                            <C>
                                <Inpt name="magnitude"
                                      type="number"
                                      min="0"/>
                                <Hint>{{tpl.value.description}}</Hint>
                            </C>
                        </Row>
                    </template>
                </Repeater>
                <!-- #!endif -->
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Hint from "../../components/Hint";
    import Row from "../../layout/Row";
    import C from "../../layout/Col";
    import Repeater from "../../components/Repeater";
    import Group from "../../components/Group";

    export default {
        components: {
            Group,
            Repeater,
            C,
            Row,
            Hint,
            Inpt
        },
        inheritAttrs: false,
        props: {
            dcz: {
                type: Object,
                default: () => ({})
            },
            relays: {
                type: Object,
                default: () => ({config:{}})
            },
            sns: {
                type: Object,
                default: () => ({magnitudes:{}})
            },
        }
    }
</script>

<style lang="less">

</style>
