<template>
    <section>
        <div class="header">
            <h1>THINGSPEAK</h1>
            <h2>
                Send your sensors data to <A href="https://thingspeak.com/">Thingspeak</A>.
            </h2>
        </div>

        <div class="page">
            <fieldset>
                <legend>General</legend>

                <Row>
                    <C><label>Enable Thingspeak</label></C>
                    <C>
                        <Inpt type="switch"
                              name="enabled"
                              tabindex="1"/>
                    </C>
                </Row>

                <Row>
                    <label>Clear cache</label>
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
                    <C><label>Thingspeak API Key</label></C>
                    <C>
                        <Inpt name="tspkKey" type="text" tabindex="3"/>
                    </C>
                </Row>

                <legend>Sensors & actuators</legend>

                <Row>
                    <Hint>
                        Enter the field number to send each data to, 0 disable notifications from that component.
                    </Hint>
                </Row>

                <!-- #if process.env.VUE_APP_RELAYS === 'true' -->
                <Repeater v-model="relays.list" locked>
                    <template #default="tpl">
                        <Row>
                            <C><label>Switch</label></C>
                            <C>
                                <Inpt name="relay"
                                      type="number"
                                      min="0"
                                      max="8"/>
                            </C>
                        </Row>
                    </template>
                </Repeater>
                <!-- #endif -->

                <!-- #if process.env.VUE_APP_SENSOR === 'true' -->
                <Repeater v-model="tspk.magnitudes.list" locked>
                    <template #default="tpl">
                        <Row>
                            <C><label>Magnitude</label></C>
                            <C>
                                <Inpt name="magnitude"
                                      type="number"
                                      min="0"
                                      max="8"
                                      tabindex="0"
                                      data="0"/>
                                <Hint>{{tpl.row.description}}</Hint>
                            </C>
                        </Row>
                    </template>
                </Repeater>
                <!-- #endif -->
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Hint from "../../components/Hint";
    import Row from "../../layout/Row";
    import C from "../../layout/Col";
    import Repeater from "../../components/Repeater";

    export default {
        components: {
            Repeater,
            C,
            Row,
            Hint,
            Inpt
        },
        inheritAttrs: false,
    }
</script>

<style lang="less">

</style>