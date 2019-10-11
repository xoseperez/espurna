<template>
    <section>
        <div class="header">
            <h1>THERMOSTAT</h1>
            <h2>Thermostat configuration</h2>
        </div>

        <div class="page">
            <div class="pure-g">
                <label class="pure-u-1 pure-u-lg-1-4">Enable Thermostat</label>
                <div class="pure-u-1 pure-u-lg-1-4">
                    <Inpt type="checkbox"
                          name="thermostatEnabled"
                          tabindex="30"/>
                </div>
            </div>

            <div class="pure-g">
                <label class="pure-u-1 pure-u-lg-1-4">Thermostat Mode</label>
                <div class="pure-u-1 pure-u-lg-1-4">
                    <Inpt type="checkbox"
                          name="thermostatMode"
                          tabindex="30"/>
                </div>
            </div>

            <div class="pure-g">
                <label class="pure-u-1 pure-u-lg-1-4" for="thermostatOperationMode">Operation
                    mode</label>
                <Inpt class="pure-u-1 pure-u-lg-1-4" name="thermostatOperationMode" type="text"
                      readonly/>
            </div>

            <fieldset>
                <legend>Temperature range</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Max (<span class="tmpUnit"></span>)</label>
                    <Inpt id="tempRangeMaxInput"
                          class="pure-u-1 pure-u-lg-1-4"
                          name="tempRangeMax"
                          type="number"
                          min="1"
                          max="100"
                          tabindex="32"
                          data="20"
                          onchange="checkTempRangeMax()"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Min (<span class="tmpUnit"></span>)</label>
                    <Inpt id="tempRangeMinInput"
                          class="pure-u-1 pure-u-lg-1-4"
                          name="tempRangeMin"
                          type="number"
                          min="0"
                          max="99"
                          tabindex="31"
                          data="10"
                          onchange="checkTempRangeMin()"/>
                </div>
            </fieldset>

            <fieldset>
                <legend>Remote sensor</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="remoteSensorName">Remote sensor
                        name</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4" name="remoteSensorName" type="text"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="remoteTmp">Remote temperature (<span
                            class="tmpUnit"></span>)</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4" name="remoteTmp" type="text" readonly/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="remoteTempMaxWait">Remote temperature
                        waiting
                        (s)</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4"
                          name="remoteTempMaxWait"
                          type="number"
                          min="0"
                          max="1800"
                          tabindex="33"
                          data="120"/>
                </div>
            </fieldset>

            <fieldset>
                <legend>Operation mode</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="maxOnTime">Max heating time (m)</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4"
                          name="maxOnTime"
                          type="number"
                          min="0"
                          max="180"
                          tabindex="34"
                          data="30"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="minOffTime">Min rest time (m)</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4"
                          name="minOffTime"
                          type="number"
                          min="0"
                          max="60"
                          tabindex="35"
                          data="10"/>
                </div>
            </fieldset>

            <fieldset>
                <legend>Autonomous mode</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="aloneOnTime">Heating time (m)</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4"
                          name="aloneOnTime"
                          type="number"
                          min="0"
                          max="180"
                          tabindex="36"
                          data="5"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="aloneOffTime">Rest time (m)</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4"
                          name="aloneOffTime"
                          type="number"
                          min="0"
                          max="180"
                          tabindex="37"
                          data="55"/>
                </div>
            </fieldset>

            <fieldset>
                <legend>Time worked</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="burnToday">Today</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4" type="text" name="burnToday" readonly/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="burnYesterday">Yesterday</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4" type="text" name="burnYesterday" readonly/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="burnThisMonth">Current month</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4" type="text" name="burnThisMonth" readonly/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="burnPrevMonth">Previous month</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4" type="text" name="burnPrevMonth" readonly/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4" for="burnTotal">Total</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4" type="text" name="burnTotal" readonly/>
                </div>

                <div class="pure-g">
                    <button type="button" class="pure-button button-thermostat-reset-counters">
                        Reset
                        counters
                    </button>
                </div>
            </fieldset>
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