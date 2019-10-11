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
                        <Inpt type="checkbox"
                              name="useColor"
                              action="reload"
                              tabindex="8"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Use the first three channels as RGB channels.
                        This will also enable the color picker in the web UI. Will only work if the
                        device
                        has at least 3 dimmable channels.<br>Reload the page to update the web
                        interface.
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use RGB picker</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="useRGB"
                              action="reload"
                              tabindex="11"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Use RGB color picker if enabled (plus
                        brightness), otherwise use HSV (hue-saturation-value) style
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use white channel</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="useWhite"
                              action="reload"
                              tabindex="9"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        For 2 channels warm white and cold white lights
                        or color lights to use forth dimmable channel as (cold) white light calculated
                        out
                        of the RGB values.<br>Will only work if the device has at least 4 dimmable
                        channels.<br>Enabling this will render useless the "Channel 4" slider in the
                        status
                        page.<br>Reload the page to update the web interface.
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use white color temperature</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="useCCT"
                              action="reload"
                              tabindex="10"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
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
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use gamma correction</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="useGamma"
                              tabindex="11"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Use gamma correction for RGB channels.<br>Will
                        only work if "use colorpicker" above is also ON.
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Use CSS style</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox" name="useCSS" tabindex="12"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Use CSS style to report colors to MQTT and REST
                        API. <br>Red will be reported as "#FF0000" if ON, otherwise "255,0,0"
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Color transitions</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="useTransitions"
                              tabindex="13"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        If enabled color changes will be smoothed.
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Transition time</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt class="pure-u-1"
                              type="number"
                              name="lightTime"
                              min="10"
                              max="5000"
                              tabindex="14"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Time in millisecons to transition from one
                        color to another.
                    </div>
                </div>

                <div class="pure-g">
                    <div class="pure-u-1 pure-u-lg-1-4"><label>MQTT group</label></div>
                    <div class="pure-u-1 pure-u-lg-3-4">
                        <Inpt name="mqttGroupColor"
                              class="pure-u-1"
                              tabindex="15"
                              action="reconnect"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">Sync color between different lights.</div>
                </div>
            </fieldset>
            <fieldset>
                <legend>SCHEDULE</legend>
                <h2>Turn switches ON and OFF based on the current time.</h2>
                <div id="color-schedules"></div>

                <button type="button" class="pure-button button-add-light-schedule module module-color">
                    Add channel schedule
                </button>
            </fieldset>
        </div>

        <div id="lightActionTemplate" class="template">
            <label class="pure-u-1 pure-u-lg-1-4">Brightness</label>
            <div class="pure-u-1 pure-u-lg-1-5">
                <Inpt class="pure-u-2-3"
                      name="schAction"
                      type="number"
                      min="0"
                      step="1"
                      max="255"
                      value="0"/>
            </div>
            <Inpt class="pure-u-1 pure-u-lg-1-5 islight" name="schSwitch" :options="lightOptions"></Inpt>
            <Inpt type="hidden" name="schType" value="2"/>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';

    export default {
        name: "color",
        components: {
            Inpt
        }
    }
</script>

<style scoped>

</style>