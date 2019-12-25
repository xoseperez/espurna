<template>
    <section>
        <div class="header">
            <h1>RADIO FREQUENCY</h1>
            <h2>
                Sonoff 433 RF Bridge &amp; RF Link Configuration<br>
            </h2>
        </div>

        <div>
            This page allows you to configure the RF codes for the Sonoff RFBridge 433 and also for a basic RF receiver.
            <br><br>
            To learn a new code click <strong>LEARN</strong>
            (the Sonoff RFBridge will beep) then press a button on the remote, the new code should show up (and the
            RFBridge will double beep). If the device double beeps but the code does not update it has not been
            properly learnt. Keep trying.
            <br><br>
            Modify or create new codes manually and then click
            <strong>SAVE</strong> to store them in the device memory. If your controlled device uses the same code
            to switch ON and OFF, learn the code with the ON button and copy paste it to the OFF input box, then
            click SAVE on the last one to store the value.
            <br><br>
            Delete any code clicking the
            <strong>FORGET</strong> button.
            <br><br>
            You can also specify any RAW code. For reference see
            <A href="https://github.com/Portisch/RF-Bridge-EFM8BB1/wiki/Commands">possible commands for Sonoff RF Bridge
                EFM8BB1</A>
            (original firmware supports codes from <strong>0xA0</strong> to <strong>0xA5</strong>).
        </div>

        <Group v-model="rfb" class="page form">
            <fieldset>
                <legend>RF Codes</legend>
                <Repeater name="list">
                    <template #default="tpl">
                        <label>Switch #{{tpl.i}}</label>
                        <Row>
                            <C><label>Switch ON</label></C>
                            <C>
                                <Inpt type="text"
                                      maxlength="116"
                                      name="on"/>
                            </C>
                            <C>
                                <Btn name="rfb-learn">LEARN</Btn>
                            </C>
                            <C>
                                <Btn name="rfb-send">SAVE</Btn>
                            </C>
                            <C>
                                <Btn name="rfb-forget">FORGET</Btn>
                            </C>
                        </Row>

                        <Row>
                            <div><label>Switch OFF</label></div>
                            <Inpt type="text"
                                  maxlength="116"
                                  name="off"/>
                            <C>
                                <Btn name="rfb-learn">LEARN</Btn>
                            </C>
                            <C>
                                <Btn name="rfb-send">SAVE</Btn>
                            </C>
                            <C>
                                <Btn name="rfb-forget">FORGET</Btn>
                            </C>
                        </Row>
                    </template>
                </Repeater>

                <legend>Settings</legend>
                <Row>
                    <C><label>Repeats</label></C>
                    <C>
                        <Inpt name="repeat"
                              type="number"
                              min="1"
                              tabindex="1"/>
                        <Hint>
                            Number of times to repeat transmission
                        </Hint>
                    </C>
                </Row>

                <template v-if="modules.rfbdirect">
                    <legend>GPIO</legend>
                    <Row>
                        <Hint>
                            Pins used by the receiver (RX) and transmitter
                            (TX). Set to <strong>NONE</strong> to disable
                        </Hint>

                        <label>RX Pin</label>
                        <Inpt type="select" name="RX" :options="gpios" tabindex="2"/>

                        <label>TX Pin</label>
                        <Inpt type="select" name="TX" :options="gpios" tabindex="3"/>
                    </Row>
                </template>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Btn from "../../components/Button";
    import Hint from "../../components/Hint";
    import Row from "../../layout/Row";
    import C from "../../layout/Col";
    import A from "../../components/ExtLink";
    import Repeater from "../../components/Repeater";
    import Group from "../../components/Group";

    export default {
        components: {
            Group,
            Repeater,
            A,
            C,
            Row,
            Hint,
            Btn,
            Inpt
        },
        inheritAttrs: false,
        props: {
            modules: {
                type: Object,
                default: () => ({})
            }
        },
        computed: {
            gpios() {
                // TODO: cross-check used GPIOs
                // TODO: support 9 & 10 with esp8285 variant
                return [
                    {k: 153, l: "NONE"},
                    {k: 0, l: "0"},
                    {k: 1, l: "1 (U0TXD)"},
                    {k: 2, l: "2 (U1TXD)"},
                    {k: 3, l: "3 (U0RXD)"},
                    {k: 4, l: "4"},
                    {k: 5, l: "5"},
                    {k: 12, l: "12 (MTDI)"},
                    {k: 13, l: "13 (MTCK)"},
                    {k: 14, l: "14 (MTMS)"},
                    {k: 15, l: "15 (MTDO)"},
                ]
            }
        }
    }
</script>

<style>
    /* -----------------------------------------------------------------------------
        RF Bridge panel
       -------------------------------------------------------------------------- */

    #panel-rfb fieldset {
        margin: 10px 2px;
        padding: 20px;
    }

    #panel-rfb input {
        margin-right: 5px;
    }

    #panel-rfb label {
        padding-top: 5px;
    }

    #panel-rfb input {
        text-align: center;
    }

    /* -----------------------------------------------------------------------------
        Table
       -------------------------------------------------------------------------- */


    table.dataTable.display tbody td {
        text-align: center;
    }

    .filtered {
        color: rgb(202, 60, 60);
    }

</style>
