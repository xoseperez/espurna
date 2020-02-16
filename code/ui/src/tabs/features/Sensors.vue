<template>
    <section>
        <div class="header">
            <h1>SENSOR CONFIGURATION</h1>
            <h2>
                Configure and calibrate your device sensors.
            </h2>
        </div>

        <Group v-model="sensors" class="page form">
            <fieldset>
                <legend>General</legend>
                <Row>
                    <C><label>Read interval</label></C>
                    <C>
                        <Inpt type="select" name="read" :options="intervalOptions" :default="6"/>
                        <Hint>
                            Select the interval between readings. These will be filtered and averaged for the report.
                            Please mind some sensors do not have fast refresh intervals. Check the sensor datasheet to
                            know the minimum read interval. The default and recommended value is 6 seconds.
                        </Hint>
                    </C>
                </Row>

                <Row>
                    <C><label>Report every</label></C>
                    <C>
                        <Inpt name="report"
                              type="number"
                              unit="readings"
                              min="1"
                              step="1"
                              max="60"/>
                        <Hint>
                            Select the number of readings to average and report
                        </Hint>
                    </C>
                </Row>

                <template v-if="modules.pwr">
                    <Row>
                        <C><label>Save every</label></C>
                        <C>
                            <Inpt name="save"
                                  type="number"
                                  unit="reports"
                                  min="0"
                                  step="1"
                                  max="200"/>
                            <Hint>
                                Save aggregated data to EEPROM after these many reports. At the moment this only applies
                                to total energy readings.
                                Please mind: saving data to EEPROM too often will wear out the flash memory quickly.
                                Set it to 0 to disable this feature (default value).
                            </Hint>
                        </C>
                    </Row>

                    <Row>
                        <C><label>Power units</label></C>
                        <C>
                            <Inpt type="select" name="pwrUnits" :options="['Watts (W)', 'Kilowatts (kW)']"/>
                        </C>
                    </Row>

                    <Row>
                        <C><label>Energy units</label></C>
                        <C>
                            <Inpt type="select" name="eneUnits" :options="['Joules (J)', 'Kilowatts·hour (kWh)']"/>
                        </C>
                    </Row>
                </template>

                <template v-if="modules.temperature">
                    <Row>
                        <C><label>Temperature units</label></C>
                        <C>
                            <Inpt type="select" name="tmpUnits" :options="['Celsius (°C)', 'Fahrenheit (°F)']"/>
                        </C>
                    </Row>

                    <Row>
                        <C><label>Temperature correction</label></C>
                        <C>
                            <Inpt name="tmpCorrection"
                                  type="number"
                                  action="reboot"
                                  min="-100"
                                  step="0.1"
                                  max="100"/>
                            <Hint>
                                Temperature correction value is added to the measured value which may be
                                inaccurate
                                due to many factors. The value can be negative.
                            </Hint>
                        </C>
                    </Row>
                </template>

                <Row v-if="modules.humidity">
                    <C><label>Humidity correction</label></C>
                    <C>
                        <Inpt name="humCorrection"

                              type="number"
                              action="reboot"
                              min="-100"
                              step="0.1"
                              max="100"/>
                        <Hint>
                            Humidity correction value is added to the measured value which may be inaccurate
                            due to many factors. The value can be negative.
                        </Hint>
                    </C>
                </Row>

                <Row v-if="modules.mics">
                    <C><label>Calibrate gas sensor</label></C>
                    <C>
                        <Inpt type="switch"
                              name="resetCalibration"/>
                        <Hint>
                            Move this switch to ON and press "Save" to reset gas sensor calibration.
                            Check the sensor datasheet for calibration conditions.
                        </Hint>
                    </C>
                </Row>

                <template v-if="modules.emon || modules.cse || modules.hlw">
                    <legend>Energy monitor</legend>

                    <Row v-if="modules.emon">
                        <C><label>Voltage</label></C>
                        <C>
                            <Inpt name="pwrVoltage" type="text"/>
                            <Hint>Mains voltage in your system (in V).</Hint>
                        </C>
                    </Row>

                    <template v-if="modules.cse || modules.hlw">
                        <Row>
                            <C><label>Expected Current</label></C>
                            <C>
                                <Inpt name="pwrExpectedC"
                                      type="text"
                                      placeholder="0"/>
                                <Hint>
                                    In Amperes (A). If you are using a pure resistive load like a bulb, this will be the
                                    ratio between the two previous values, i.e. power / voltage. You can also use a
                                    current clamp around one of the power wires to get this value.
                                </Hint>
                            </C>
                        </Row>

                        <Row>
                            <C><label>Expected Voltage</label></C>
                            <C>
                                <Inpt name="pwrExpectedV"
                                      type="text"
                                      placeholder="0"/>
                                <Hint>
                                    In Volts (V). Enter your the nominal AC voltage for your household or facility, or
                                    use multimeter to get this value.
                                </Hint>
                            </C>
                        </Row>
                    </template>

                    <Row>
                        <C><label>Expected Power</label></C>
                        <C>
                            <Inpt name="pwrExpectedP"
                                  type="text"
                                  placeholder="0"/>
                            <Hint>
                                In Watts (W). Calibrate your sensor connecting a pure resistive load (like a bulb) and
                                enter here its nominal power or use a multimeter.
                            </Hint>
                        </C>
                    </Row>
                </template>

                <Row v-if="modules.pm">
                    <C><label>Energy Ratio</label></C>
                    <C>
                        <Inpt name="pwrRatioE"
                              type="text"
                              placeholder="0"/>
                        <Hint>Energy ratio in pulses/kWh.</Hint>
                    </C>
                </Row>

                <Row v-if="modules.pm || modules.emon || modules.cse">
                    <C><label>Reset calibration</label></C>
                    <C>
                        <Inpt type="switch"
                              name="pwrResetCalibration"/>
                        <Hint>
                            Move this switch to ON and press "Save" to revert to factory calibration values.
                        </Hint>
                    </C>
                </Row>
                <Row v-if="modules.pm || modules.emon || modules.cse || modules.hlw">
                    <C><label>Reset energy</label></C>
                    <C>
                        <Inpt type="switch"
                              name="pwrResetE"/>
                        <Hint>
                            Move this switch to ON and press "Save" to set energy count to 0.
                        </Hint>
                    </C>
                </Row>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
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
            Inpt
        },
        inheritAttrs: false,
        props: {
            modules: Object,
            sensors: {
                type: Object,
                default: () => ({})
            }
        },
        computed: {
            intervalOptions() {
                let opts = [1, 2, 3, 4, 5, 6, 10, 15, 30, 45, 60, 120, 180, 240, 300, 600, 900, 1800, 3600];

                return opts.map((v) => {
                    let u = 'second';
                    let uv = v;
                    if (v > 60) {
                        u = 'minute';
                        uv = v / 60;
                    }
                    if (uv > 1) {
                        u += 's';
                    }
                    return {k: v, l: uv + ' ' + u};
                });
            }
        }
    }
</script>

<style lang="less">

</style>
