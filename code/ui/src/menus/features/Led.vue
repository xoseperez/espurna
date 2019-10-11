<template>
    <section>
        <div class="header">
            <h1>LED</h1>
            <h2>Notification LED configuration</h2>
        </div>

        <div class="page">
            <fieldset>
                <div class="pure-g module module-led">
                    <div class="pure-u-0 pure-u-lg-1-4">Modes</div>
                    <ul class="pure-u-1 pure-u-lg-3-4 hint">
                        <li>
                            <strong>WiFi status</strong> will blink at 1Hz when trying to connect. If
                            successfully connected it will briefly blink every 5 seconds if in STA mode
                            or
                            every second if in AP mode.
                        </li>
                        <li>
                            <strong>Follow switch</strong> will force the LED to follow the status of a
                            given switch (you must define which switch to follow in the side field).
                        </li>
                        <li>
                            <strong>Inverse switch</strong> will force the LED to not-follow the status
                            of a
                            given switch (you must define which switch to follow in the side field).
                        </li>
                        <li>
                            <strong>Find me</strong> will turn the LED ON when all switches are OFF.
                            This is
                            meant to locate switches at night.
                        </li>
                        <li>
                            <strong>Find me &amp; WiFi</strong> will follow the WiFi status but will
                            stay
                            mostly on when switches are OFF, and mostly OFF when any of them is ON.
                        </li>
                        <li>
                            <strong>Switches status</strong> will turn the LED ON whenever any switch is
                            ON,
                            and OFF otherwise. This is global status notification.
                        </li>
                        <li>
                            <strong>Switches status &amp; WiFi</strong> will follow the WiFi status but
                            will
                            stay mostly off when switches are OFF, and mostly ON when any of them is ON.
                        </li>
                        <li>
                            <strong>MQTT managed</strong> will let you manage the LED status via MQTT by
                            sending a message to "&lt;base_topic&gt;/led/0/set" with a payload of 0, 1
                            or 2
                            (to toggle it).
                        </li>
                        <li>
                            <strong>Always ON</strong> and <strong>Always OFF</strong> modes are
                            self-explanatory.
                        </li>
                    </ul>
                </div>

                <div id="ledConfig"></div>
            </fieldset>
        </div>

        <!-- Templates -->

        <div id="ledConfigTemplate" class="template">
            <div class="pure-g">
                <label class="pure-u-1 pure-u-lg-1-4">LED #<span class="id"></span> mode</label>
                <select name="ledMode" class="pure-u-1-4">
                    <option value="1">WiFi status</option>
                    <option value="2">Follow switch #</option>
                    <option value="3">Inverse switch #</option>
                    <option value="4">Find me</option>
                    <option value="5">Find me &amp; WiFi</option>
                    <option value="8">Switches status</option>
                    <option value="9">Switches &amp; WiFi</option>
                    <option value="0">MQTT managed</option>
                    <option value="6">Always ON</option>
                    <option value="7">Always OFF</option>
                </select>
                &nbsp;
                <div class="pure-u-1-4">
                    <Inpt class="pure-u-23-24" name="ledRelay" type="number" min="0" data="0"/>
                </div>
            </div>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';

    export default {
        components: {
            Inpt
        }
    }
</script>

<style scoped>

</style>