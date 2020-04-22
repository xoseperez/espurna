<template>
    <section v-loading="!tspk">
        <div class="header">
            <h1>THINGSPEAK</h1>
            <h2>
                Send your sensors data to <A href="https://thingspeak.com/">ThingSpeak</A>.
            </h2>
        </div>

        <Group v-model="tspk" class="page form" #default>
            <fieldset>
                <legend>General</legend>

                <Row>
                    <C><label>Enable ThingSpeak</label></C>
                    <C>
                        <Inpt type="switch"
                              name="enabled"
                              tabindex="1"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Clear cache</label></C>
                    <C>
                        <Inpt type="switch"
                              name="clear"
                              tabindex="2"/>
                        <Hint>
                            With every POST to thingspeak.com only enqueued fields are sent. If you select to clear the
                            cache after every sending this will result in only those fields that have changed will be
                            posted. If you want all fields to be sent with every POST do not clear the cache.
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>ThingSpeak Address</label></C>
                    <C>
                        <Inpt name="address" type="text" tabindex="3"/>
                    </C>
                </Row>

                <Row>
                    <C><label>ThingSpeak API Key</label></C>
                    <C>
                        <Inpt name="key" type="text" tabindex="4"/>
                    </C>
                </Row>

                <legend>Sensors & actuators</legend>

                <Row>
                    <Hint>
                        Enter the field number to send each data to, 0 disable notifications from that component.
                    </Hint>
                </Row>

                <!-- #!if RELAYS === true -->
                <Repeater v-if="relay" v-model="relay.list" locked>
                    <template #default="tpl">
                        <Row>
                            <C><label>Switch #{{tpl.k}}</label></C>
                            <C>
                                <Inpt name="relay"
                                      type="number"
                                      min="0"
                                      max="8"/>
                            </C>
                        </Row>
                    </template>
                </Repeater>
                <!-- #!endif -->

                <!-- #!if SENSOR === true -->
                <Repeater v-if="sns" v-model="sns.magnitudes.list" locked>
                    <template #default="tpl">
                        <Row>
                            <C><label>Magnitude {{tpl.value.name}}</label></C>
                            <C>
                                <Inpt name="magnitude"
                                      type="number"
                                      min="0"
                                      max="8"
                                      tabindex="0"
                                      data="0"/>
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
            tspk: Object,
            relay: Object,
            sns: Object,
        }
    };
</script>

<style lang="less">

</style>
