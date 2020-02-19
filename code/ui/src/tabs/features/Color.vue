<template>
    <section>
        <div class="header">
            <h1>LIGHTS</h1>
            <h2>Lights configuration</h2>
        </div>

        <Group v-model="color" class="page form">
            <fieldset>
                <Row>
                    <C><label>Use color</label></C>
                    <C>
                        <Inpt type="switch"
                              name="useColor"
                              action="reload"
                              tabindex="1"/>
                        <Hint>
                            Use the first three channels as RGB channels.
                            This will also enable the color picker in the web UI. Will only work if the device
                            has at least 3 dimmable channels.<br>Reload the page to update the web interface.
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>Use RGB picker</label></C>
                    <C>
                        <Inpt type="switch"
                              name="useRGB"
                              action="reload"
                              tabindex="2"/>
                        <Hint>
                            Use RGB color picker if enabled (plus
                            brightness), otherwise use HSV (hue-saturation-value) style
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>Use white channel</label></C>

                    <C>
                        <Inpt type="switch"
                              name="useWhite"
                              action="reload"
                              tabindex="3"/>
                        <Hint>
                            For 2 channels warm white and cold white lights
                            or color lights to use forth dimmable channel as (cold) white light calculated
                            out
                            of the RGB values.<br>Will only work if the device has at least 4 dimmable
                            channels.<br>Enabling this will render useless the "Channel 4" slider in the
                            status
                            page.<br>Reload the page to update the web interface.
                        </Hint>
                    </C>
                </Row>


                <Row>
                    <C><label>Use white color temperature</label></C>

                    <C>
                        <Inpt type="switch"
                              name="useCCT"
                              action="reload"
                              tabindex="4"/>
                        <Hint>
                            Use a dimmable channel as warm white light and
                            another dimmable channel as cold white light.<br>On devices with two dimmable
                            channels the first use used for warm white light and the second for cold white
                            light.<br>On color lights the fifth use used for warm white light and the fourth
                            for cold white light.<br>Will only work if the device has exactly 2 dimmable
                            channels or at least 5 dimmable channels and "white channel" above is also
                            ON.<br>Enabling
                            this will render useless the "Channel 5" slider in the status page.<br>Reload
                            the
                            page to update the web interface.
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>Use gamma correction</label></C>

                    <C>
                        <Inpt type="switch"
                              name="useGamma"
                              tabindex="5"/>
                        <Hint>
                            Use gamma correction for RGB channels.<br>Will
                            only work if "use colorpicker" above is also ON.
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>Use CSS style</label></C>

                    <C>
                        <Inpt type="switch" name="useCSS" tabindex="12"/>
                        <Hint>
                            Use CSS style to report colors to MQTT and REST
                            API. <br>Red will be reported as "#FF0000" if ON, otherwise "255,0,0"
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>Color transitions</label></C>

                    <C>
                        <Inpt type="switch"
                              name="useTransitions"
                              tabindex="6"/>
                        <Hint>
                            If enabled color changes will be smoothed.
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>Transition time</label></C>

                    <C>
                        <Inpt type="number"
                              name="lightTime"
                              min="10"
                              max="5000"
                              tabindex="7"/>
                        <Hint>
                            Time in milliseconds to transition from one
                            color to another.
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>MQTT group</label></C>
                    <C>
                        <Inpt name="mqttGroupColor"
                              tabindex="8"
                              action="reconnect"/>
                        <Hint>Sync color between different lights.</Hint>
                    </C>
                </Row>
            </fieldset>
            <fieldset>
                <legend>SCHEDULE</legend>
                <h2>Turn switches ON and OFF based on the current time.</h2>
                <Repeater v-model="color.schedules">
                    <template #btnAdd="tpl">
                        <Btn name="add-light-schedule" @click="tpl.click">
                            Add channel schedule
                        </Btn>
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
    import Row from "../../layout/Row";
    import C from "../../layout/Col";
    import Group from "../../components/Group";

    export default {
        components: {
            Group,
            C,
            Row,
            Hint,
            Repeater,
            Btn,
            Inpt
        },
        inheritAttrs: false,
        props: {
            color: {
                type: Object,
                default: () => ({})
            }
        }
    };
</script>

<style lang="less">

</style>