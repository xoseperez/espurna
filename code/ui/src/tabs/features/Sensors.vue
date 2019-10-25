<template>
    <section>
        <div class="header">
            <h1>SENSOR CONFIGURATION</h1>
            <h2>
                Configure and calibrate your device sensors.
            </h2>
        </div>

        <div class="page">
            <fieldset>
                <legend>General</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Read interval</label>
                    <select class="pure-u-1 pure-u-lg-1-4" name="snsRead">
                        <option value="1">1 second</option>
                        <option value="6">6 seconds</option>
                        <option value="10">10 seconds</option>
                        <option value="15">15 seconds</option>
                        <option value="30">30 seconds</option>
                        <option value="60">1 minute</option>
                        <option value="300">5 minutes</option>
                        <option value="600">10 minutes</option>
                        <option value="900">15 minutes</option>
                        <option value="1800">30 minutes</option>
                        <option value="3600">60 minutes</option>
                    </select>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Select the interval between readings. These will be filtered and averaged for the report.
                        Please mind some sensors do not have fast refresh intervals. Check the sensor datasheet to know
                        the minimum read interval. The default and recommended value is 6 seconds.
                    </Hint>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Report every</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt name="snsReport"
                              class="pure-u-1"
                              type="number"
                              min="1"
                              step="1"
                              max="60"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Select the number of readings to average and report
                    </Hint>
                </div>

                <div class="pure-g module module-pwr">
                    <label class="pure-u-1 pure-u-lg-1-4">Save every</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt name="snsSave"
                              class="pure-u-1"
                              type="number"
                              min="0"
                              step="1"
                              max="200"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Save aggregated data to EEPROM after these many reports. At the moment this only applies to
                        total energy readings.
                        Please mind: saving data to EEPROM too often will wear out the flash memory
                        quickly. Set it to 0 to disable this feature (default value).
                    </Hint>
                </div>

                <div class="pure-g module module-pwr">
                    <label class="pure-u-1 pure-u-lg-1-4">Power units</label>
                    <select name="pwrUnits" tabindex="16" class="pure-u-1 pure-u-lg-1-4">
                        <option value="0">Watts (W)</option>
                        <option value="1">Kilowatts (kW)</option>
                    </select>
                </div>

                <div class="pure-g module module-pwr">
                    <label class="pure-u-1 pure-u-lg-1-4">Energy units</label>
                    <select name="eneUnits" tabindex="16" class="pure-u-1 pure-u-lg-1-4">
                        <option value="0">Joules (J)</option>
                        <option value="1">Kilowatts·hour (kWh)</option>
                    </select>
                </div>

                <div class="pure-g module module-temperature">
                    <label class="pure-u-1 pure-u-lg-1-4">Temperature units</label>
                    <select name="tmpUnits" tabindex="16" class="pure-u-1 pure-u-lg-1-4">
                        <option value="0">Celsius (&deg;C)</option>
                        <option value="1">Fahrenheit (&deg;F)</option>
                    </select>
                </div>

                <div class="pure-g module module-temperature">
                    <label class="pure-u-1 pure-u-lg-1-4">Temperature correction</label>
                    <Inpt name="tmpCorrection"
                          class="pure-u-1 pure-u-lg-1-4"
                          type="number"
                          action="reboot"
                          min="-100"
                          step="0.1"
                          max="100"
                          tabindex="18"/>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Temperature correction value is added to the measured value which may be
                        inaccurate
                        due to many factors. The value can be negative.
                    </Hint>
                </div>

                <div class="pure-g module module-humidity">
                    <label class="pure-u-1 pure-u-lg-1-4">Humidity correction</label>
                    <Inpt name="humCorrection"
                          class="pure-u-1 pure-u-lg-1-4"
                          type="number"
                          action="reboot"
                          min="-100"
                          step="0.1"
                          max="100"
                          tabindex="18"/>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Humidity correction value is added to the measured value which may be inaccurate
                        due
                        to many factors. The value can be negative.
                    </Hint>
                </div>

                <div class="pure-g module module-mics">
                    <label class="pure-u-1 pure-u-lg-1-4">Calibrate gas sensor</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="snsResetCalibration"
                              tabindex="55"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        Move this switch to ON and press "Save" to
                        reset gas sensor calibration. Check the sensor datasheet for calibration
                        conditions.
                    </Hint>
                </div>

                <legend class="module module-hlw module-cse module-emon">Energy monitor</legend>

                <div class="pure-g module module-emon">
                    <label class="pure-u-1 pure-u-lg-1-4">Voltage</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4" name="pwrVoltage" type="text" tabindex="51"/>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>Mains voltage in your system (in V).</Hint>
                </div>

                <div class="pure-g module module-hlw module-cse">
                    <label class="pure-u-1 pure-u-lg-1-4">Expected Current</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4 pwrExpected"
                          name="pwrExpectedC"
                          type="text"
                          tabindex="52"
                          placeholder="0"/>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        In Amperes (A). If you are using a pure resistive load like a bulb, this will be the ratio
                        between the two previous values, i.e. power / voltage. You can also use a current clamp around
                        one of the power wires to get this value.
                    </Hint>
                </div>

                <div class="pure-g module module-hlw module-cse">
                    <label class="pure-u-1 pure-u-lg-1-4">Expected Voltage</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4 pwrExpected"
                          name="pwrExpectedV"
                          type="text"
                          tabindex="53"
                          placeholder="0"/>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        In Volts (V). Enter your the nominal AC voltage
                        for your household or facility, or use multimeter to get this value.
                    </Hint>
                </div>

                <div class="pure-g module module-hlw module-cse module-emon">
                    <label class="pure-u-1 pure-u-lg-1-4">Expected Power</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4 pwrExpected"
                          name="pwrExpectedP"
                          type="text"
                          tabindex="54"
                          placeholder="0"/>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>
                        In Watts (W). Calibrate your sensor connecting
                        a pure resistive load (like a bulb) and enter here its nominal power or use a
                        multimeter.
                    </Hint>
                </div>

                <div class="pure-g module module-pm">
                    <label class="pure-u-1 pure-u-lg-1-4">Energy Ratio</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4"
                          name="pwrRatioE"
                          type="text"
                          tabindex="55"
                          placeholder="0"/>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>Energy ratio in pulses/kWh.</Hint>
                </div>

                <div class="pure-g module module-hlw module-cse module-emon">
                    <label class="pure-u-1 pure-u-lg-1-4">Reset calibration</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="pwrResetCalibration"
                              tabindex="56"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Move this switch to ON and press "Save" to
                        revert to factory calibration values.
                    </div>
                </div>

                <div class="pure-g module module-hlw module-cse module-emon module-pm">
                    <label class="pure-u-1 pure-u-lg-1-4">Reset energy</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch"
                              name="pwrResetE"
                              tabindex="57"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Move this switch to ON and press "Save" to set
                        energy count to 0.
                    </div>
                </div>
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