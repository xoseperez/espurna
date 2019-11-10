<template>
    <section>
        <div class="header">
            <h1>LIGHTS</h1>
            <h2>Lights configuration</h2>
        </div>

        <div class="page">
            <fieldset>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use color</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="useColor"
                              action="reload"
                              tabindex="1"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Use the first three channels as RGB channels.
                        This will also enable the color picker in the web UI. Will only work if the
                        device
                        has at least 3 dimmable channels.<br>Reload the page to update the web
                        interface.
                    </Hint>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use RGB picker</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="useRGB"
                              action="reload"
                              tabindex="2"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Use RGB color picker if enabled (plus
                        brightness), otherwise use HSV (hue-saturation-value) style
                    </Hint>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use white channel</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="useWhite"
                              action="reload"
                              tabindex="3"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        For 2 channels warm white and cold white lights
                        or color lights to use forth dimmable channel as (cold) white light calculated
                        out
                        of the RGB values.<br>Will only work if the device has at least 4 dimmable
                        channels.<br>Enabling this will render useless the "Channel 4" slider in the
                        status
                        page.<br>Reload the page to update the web interface.
                    </Hint>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use white color temperature</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="useCCT"
                              action="reload"
                              tabindex="4"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
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
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use gamma correction</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="useGamma"
                              tabindex="5"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Use gamma correction for RGB channels.<br>Will
                        only work if "use colorpicker" above is also ON.
                    </Hint>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use CSS style</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch" name="useCSS" tabindex="12"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Use CSS style to report colors to MQTT and REST
                        API. <br>Red will be reported as "#FF0000" if ON, otherwise "255,0,0"
                    </Hint>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Color transitions</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="useTransitions"
                              tabindex="6"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        If enabled color changes will be smoothed.
                    </Hint>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Transition time</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt class="pure-u-1"
                              type="number"
                              name="lightTime"
                              min="10"
                              max="5000"
                              tabindex="7"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Time in milliseconds to transition from one
                        color to another.
                    </Hint>
                </div>

                <div class="pure-g">
                    <div class="pure-u-1 pure-u-lg-1-4"><label>MQTT group</label></div>
                    <div class="pure-u-1 pure-u-lg-3-4">
                        <Inpt name="mqttGroupColor"
                              class="pure-u-1"
                              tabindex="8"
                              action="reconnect"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>Sync color between different lights.</Hint>
                </div>
            </fieldset>
            <fieldset>
                <legend>SCHEDULE</legend>
                <h2>Turn switches ON and OFF based on the current time.</h2>
                <Repeater id="color-schedules" name="">
                    <template #btnAdd="tpl">
                        <Btn name="add-light-schedule" class="module module-color" @click="tpl.click">
                            Add channel schedule
                        </Btn>
                    </template>
                </Repeater>
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Btn from "../../components/Button";
    import Repeater from "../../components/Repeater";
    import Hint from "../../components/Hint";

    export default {
        components: {
            Hint,
            Repeater,
            Btn,
            Inpt
        },
        inheritAttrs: false,
    }
</script>

<style lang="less">

</style>