<template>
    <section v-loading="true">
        <div class="header">
            <h1>THERMOSTAT</h1>
            <h2>Thermostat configuration</h2>
        </div>

        <Group v-model="thermostat" class="page form" #default>
            <fieldset>
                <Row>
                    <C><label>Enable Thermostat</label></C>
                    <C>
                        <Inpt type="switch"
                              name="enabled"
                              tabindex="1"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Thermostat Mode</label></C>
                    <C>
                        <Inpt type="switch"
                              name="mode"
                              off="Heater"
                              on="Cooler"
                              tabindex="2"/>
                    </C>
                </Row>

                <Row>
                    <C><label for="operationMode">Operation mode</label></C>
                    <C>
                        <Inpt id="operationMode" name="operationMode" type="text"
                              readonly/>
                    </C>
                </Row>
            </fieldset>

            <fieldset>
                <legend>Temperature range</legend>

                <Row>
                    <C><label for="tempRangeMax">Max</label></C>
                    <C>
                        <Inpt id="tempRangeMax"
                              name="tempRangeMax"
                              type="number"
                              :unit="tempUnit"
                              :min="thermostat.tempRangeMin + 1"
                              max="100"
                              :default="20"/>
                    </C>
                </Row>

                <Row>
                    <C><label for="tempRangeMin">Min</label></C>
                    <C>
                        <Inpt id="tempRangeMin"
                              name="tempRangeMin"
                              type="number"
                              :unit="tempUnit"
                              min="0"
                              :max="thermostat.tempRangeMax - 1"
                              :default="10"/>
                    </C>
                </Row>
            </fieldset>

            <fieldset>
                <legend>Remote sensor</legend>

                <Row>
                    <C><label for="remoteSensorName">Remote sensor name</label></C>
                    <C>
                        <Inpt id="remoteSensorName" name="remoteSensorName" type="text"/>
                    </C>
                </Row>

                <Row>
                    <C><label for="remoteTmp">Remote temperature (<span class="tmpUnit"></span>)</label></C>
                    <C>
                        <Inpt id="remoteTmp" name="remoteTmp" type="text" readonly/>
                    </C>
                </Row>

                <Row>
                    <C><label for="remoteTempMaxWait">Remote temperature waiting (s)</label></C>
                    <C>
                        <Inpt id="remoteTempMaxWait" name="remoteTempMaxWait"
                              type="number"
                              min="0"
                              max="1800"
                              data="120"/>
                    </C>
                </Row>
            </fieldset>

            <fieldset>
                <legend>Operation mode</legend>
                <Row>
                    <C><label for="maxOnTime">Max heating time</label></C>
                    <C>
                        <Inpt id="maxOnTime" name="maxOnTime"
                              type="number"
                              unit="minutes"
                              min="0"
                              max="180"
                              data="30"/>
                    </C>
                </Row>

                <Row>
                    <C><label for="minOffTime">Min rest time</label></C>
                    <C>
                        <Inpt id="minOffTime"
                              name="minOffTime"
                              type="number"
                              unit="minutes"
                              min="0"
                              max="60"
                              data="10"/>
                    </C>
                </Row>
            </fieldset>

            <fieldset>
                <legend>Autonomous mode</legend>

                <Row>
                    <C><label for="aloneOnTime">Heating time</label></C>
                    <C>
                        <Inpt id="aloneOnTime"
                              name="aloneOnTime"
                              type="number"
                              unit="minutes"
                              min="0"
                              max="180"
                              data="5"/>
                    </C>
                </Row>

                <Row>
                    <C><label for="aloneOffTime">Rest time</label></C>
                    <C>
                        <Inpt id="aloneOffTime"
                              name="aloneOffTime"
                              type="number"
                              unit="minutes"
                              min="0"
                              max="180"
                              data="55"/>
                    </C>
                </Row>
            </fieldset>

            <fieldset>
                <legend>Time worked</legend>

                <Row>
                    <C><label for="burnToday">Today</label></C>
                    <C>
                        <Inpt id="burnToday" type="text" name="burnToday" readonly/>
                    </C>
                </Row>

                <Row>
                    <C><label for="burnYesterday">Yesterday</label></C>
                    <C>
                        <Inpt id="burnYesterday" type="text" name="burnYesterday" readonly/>
                    </C>
                </Row>

                <Row>
                    <C><label for="burnThisMonth">Current month</label></C>
                    <C>
                        <Inpt id="burnThisMonth" type="text" name="burnThisMonth" readonly/>
                    </C>
                </Row>

                <Row>
                    <C><label for="burnPrevMonth">Previous month</label></C>
                    <C>
                        <Inpt id="burnPrevMonth" type="text" name="burnPrevMonth" readonly/>
                    </C>
                </Row>

                <Row>
                    <C><label for="burnTotal">Total</label></C>
                    <C>
                        <Inpt id="burnTotal" type="text" name="burnTotal" readonly/>
                    </C>
                </Row>

                <Row>
                    <Btn name="thermostat-reset-counters" color="danger">Reset counters</Btn>
                </Row>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Group from "../../components/Group";
    import Row from "../../layout/Row";
    import C from "../../layout/Col";
    import Btn from "../../components/Button";

    export default {
        components: {
            Btn,
            C,
            Row,
            Group,
            Inpt
        },
        inheritAttrs: false,
        props: {
            sns: Object,
            thermostat: Object
        },
        computed: {
            tempUnit() {
                return this.sns && this.sns.tmpUnits ? "°F" : "°C";
            }
        },
    };
</script>

<style lang="less">

</style>
