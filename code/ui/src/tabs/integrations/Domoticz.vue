<template>
    <section>
        <div class="header">
            <h1>DOMOTICZ</h1>
            <h2>
                Configure the connection to your Domoticz server.
            </h2>
        </div>

        <div class="page">
            <fieldset>
                <legend>General</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Enable Domoticz</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="dczEnabled"
                              tabindex="30"/>
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Domoticz IN Topic</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4" name="dczTopicIn" type="text" tabindex="31"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Domoticz OUT Topic</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4"
                          name="dczTopicOut"
                          type="text"
                          action="reconnect"
                          tabindex="32"/>
                </div>

                <legend>Sensors &amp; actuators</legend>

                <div class="pure-g">
                    <Hint>
                        Set IDX to 0 to disable notifications from that component.
                    </Hint>
                </div>

                <div id="dczRelays"></div>
                <div id="dczRelayTemplate" class="template">
                    <div class="pure-g">
                        <label class="pure-u-1 pure-u-lg-1-4">Switch</label>
                        <div class="pure-u-1 pure-u-lg-1-4">
                            <Inpt class="pure-u-1 pure-u-lg-23-24 dczRelayIdx"
                                  name="dczRelayIdx"
                                  type="number"
                                  min="0"
                                  tabindex="0"
                                  data="0"/>
                        </div>
                    </div>
                </div>

                <!-- removeIf(!sensor) -->
                <div id="dczMagnitudes"></div>
                <div id="dczMagnitudeTemplate" class="template">
                    <div class="pure-g">
                        <label class="pure-u-1 pure-u-lg-1-4">Magnitude</label>
                        <div class="pure-u-1 pure-u-lg-1-4">
                            <Inpt class="pure-u-1 pure-u-lg-23-24 center"
                                  name="dczMagnitude"
                                  type="number"
                                  min="0"
                                  tabindex="0"
                                  data="0"/>
                        </div>
                        <div class="pure-u-1 pure-u-lg-1-2 hint center"></div>
                    </div>
                </div>
                <!-- endRemoveIf(!sensor) -->
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';
    import Hint from "../../components/Hint";

    export default {
        components: {
            Hint,
            Inpt
        },
        inheritAttrs: false,
    }
</script>

<style lang="less">

</style>