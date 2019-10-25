<template>
    <section>
        <div class="header">
            <h1>THINGSPEAK</h1>
            <h2>
                Send your sensors data to Thingspeak.
            </h2>
        </div>

        <div class="page">
            <fieldset>
                <legend>General</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Enable Thingspeak</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="tspkEnabled"
                              tabindex="30"/>
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Clear cache</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="tspkClear"
                              tabindex="31"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        With every POST to thinkspeak.com only enqueued fields are sent. If you select to clear the
                        cache after every sending this will result in only those fields that have changed will be
                        posted. If you want all fields to be sent with every POST do not clear the cache.
                    </Hint>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Thingspeak API Key</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4" name="tspkKey" type="text" tabindex="32"/>
                </div>

                <legend>Sensors &amp; actuators</legend>

                <div class="pure-g">
                    <Hint>
                        Enter the field number to send each data to, 0 disable notifications from that component.
                    </Hint>
                </div>

                <div id="tspkRelays"></div>
                <div id="tspkRelayTemplate" class="template">
                    <div class="pure-g">
                        <label class="pure-u-1 pure-u-lg-1-4">Switch</label>
                        <div class="pure-u-1 pure-u-lg-1-4">
                            <Inpt class="pure-u-1 pure-u-lg-23-24"
                                  name="tspkRelay"
                                  type="number"
                                  min="0"
                                  max="8"
                                  tabindex="0"
                                  data="0"/>
                        </div>
                    </div>
                </div>

                <!-- removeIf(!sensor) -->
                <div id="tspkMagnitudes"></div>
                <div id="tspkMagnitudeTemplate" class="template">
                    <div class="pure-g">
                        <label class="pure-u-1 pure-u-lg-1-4">Magnitude</label>
                        <div class="pure-u-1 pure-u-lg-1-4">
                            <Inpt class="pure-u-1 pure-u-lg-23-24 center"
                                  name="tspkMagnitude"
                                  type="number"
                                  min="0"
                                  max="8"
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