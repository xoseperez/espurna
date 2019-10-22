<template>
    <section>
        <div class="header">
            <h1>SWITCHES</h1>
            <h2>Switch / relay configuration</h2>
        </div>

        <div class="page">
            <fieldset>
                <legend class="module module-multirelay">General</legend>

                <div class="pure-g module module-multirelay">
                    <label class="pure-u-1 pure-u-lg-1-4">Switch sync mode</label>
                    <select name="relaySync" class="pure-u-1 pure-u-lg-3-4" tabindex="3">
                        <option value="0">No synchronisation</option>
                        <option value="1">Zero or one switches active</option>
                        <option value="2">One and just one switch active</option>
                        <option value="3">All synchronised</option>
                        <option value="4">Switch #0 controls other switches</option>
                    </select>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Define how the different switches should be
                        synchronized.
                    </Hint>
                </div>

                <div id="relayConfig"></div>
                <div id="relayConfigTemplate" class="template">
                    <legend>Switch #<span class="id"></span> (<span class="gpio"></span>)</legend>
                    <div class="pure-g">
                        <div class="pure-u-1 pure-u-lg-1-4"><label>Boot mode</label></div>
                        <select class="pure-u-1 pure-u-lg-3-4" name="relayBoot">
                            <option value="0">Always OFF</option>
                            <option value="1">Always ON</option>
                            <option value="2">Same as before</option>
                            <option value="3">Toggle before</option>
                            <option value="4">Locked OFF</option>
                            <option value="5">Locked ON</option>
                        </select>
                    </div>
                    <div class="pure-g">
                        <div class="pure-u-1 pure-u-lg-1-4"><label>Pulse mode</label></div>
                        <select class="pure-u-1 pure-u-lg-3-4" name="relayPulse">
                            <option value="0">Don't pulse</option>
                            <option value="1">Normally OFF</option>
                            <option value="2">Normally ON</option>
                        </select>
                    </div>
                    <div class="pure-g">
                        <div class="pure-u-1 pure-u-lg-1-4"><label>Pulse time (s)</label></div>
                        <div class="pure-u-1 pure-u-lg-1-4">
                            <Inpt name="relayTime"
                                  class="pure-u-1"
                                  type="number"
                                  min="0"
                                  step="0.1"
                                  max="3600"/>
                        </div>
                    </div>
                    <div class="pure-g module module-mqtt">
                        <div class="pure-u-1 pure-u-lg-1-4"><label>MQTT group</label></div>
                        <div class="pure-u-1 pure-u-lg-3-4">
                            <Inpt name="mqttGroup"
                                  class="pure-u-1"
                                  tabindex="0"
                                  data="0"
                                  action="reconnect"/>
                        </div>
                    </div>
                    <div class="pure-g module module-mqtt">
                        <div class="pure-u-1 pure-u-lg-1-4"><label>MQTT group sync</label></div>
                        <select class="pure-u-1 pure-u-lg-3-4" name="mqttGroupSync">
                            <option value="0">Same</option>
                            <option value="1">Inverse</option>
                            <option value="2">Receive Only</option>
                        </select>
                    </div>
                    <div class="pure-g module module-mqtt">
                        <div class="pure-u-1 pure-u-lg-1-4"><label>On MQTT disconnect</label></div>
                        <Inpt type="select" class="pure-u-1 pure-u-lg-3-4" name="relayOnDisc"
                              :options="[
                                  'Don\'t change',
                                  'Turn the switch OFF',
                                  'Turn the switch ON'
                              ]"/>
                    </div>
                </div>
            </fieldset>
            <fieldset>
                <legend>SCHEDULE</legend>
                <h2>Turn switches ON and OFF based on the current time.</h2>
                <Repeater>
                    <template #btnAdd="tpl">
                        <Btn name="add-switch-schedule" class="module module-relay" @click="tpl.click">
                            Add switch schedule
                        </Btn>
                    </template>
                </Repeater>
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';
    import Btn from "../../components/Button";
    import Repeater from "../../components/Repeater";
    import Hint from "../../components/Hint";

    export default {
        components: {
            Hint,
            Repeater,
            Inpt,
            Btn
        }
    }
</script>

<style lang="less">

</style>