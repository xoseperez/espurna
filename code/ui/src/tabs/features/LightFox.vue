<template>
    <section>
        <div class="header">
            <h1>LIGHTFOX RF</h1>
            <h2>
                LightFox RF configuration<br><br>
            </h2>
        </div>

        <div>
            This page allows you to control LightFox RF receiver options.<br><br>
            To learn a new code click <strong>LEARN</strong>, wait for 3 seconds then press a button on the remote, one
            of the relays will toggle. If no device relay toggles the code has not
            been properly learnt. Keep trying.<br><br>
            Delete all the codes by clicking the <strong>CLEAR</strong> button and wait for 10 seconds.<br><br>
            You can also specify which RF button controls which relay using controls below.
        </div>

        <Group v-model="lightfox" class="page form">
            <fieldset>
                <Row>
                    <C><label>RF Actions</label></C>
                    <C>
                        <Btn name="learn" @click="sendLearn">
                            Learn
                        </Btn>
                    </C>
                    <C>
                        <Btn name="clear" color="danger" @click="sendClear">
                            Clear
                        </Btn>
                    </C>
                </Row>

                <Repeater name="buttons" locked>
                    <template #default="tpl">
                        <Row>
                            <C><label>Button #{{tpl.value.id}}</label></C>
                            <C>
                                <Inpt type="select" name="relay" :options="relayOptions"
                                      @change="$emit('reboot')"/>
                            </C>
                        </Row>
                    </template>
                </Repeater>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Btn from "../../components/Button";
    import Group from "../../components/Group";
    import Row from "../../layout/Row";
    import C from "../../layout/Col";
    import Repeater from "../../components/Repeater";
    import ws from "../../common/websocket";
    import {alertSuccess, alertError} from "../../common/notification";

    export default {
        components: {
            Repeater,
            C,
            Row,
            Group,
            Btn,
            Inpt
        },
        inheritAttrs: false,
        props: {
            buttons: {
                type: Array,
                default: () => ([])
            },
            relayOptions: {
                type: Array
            }
        },
        methods: {
            sendLearn() {
                ws.send({action: "lightfoxLearn"}, (res) => {
                    if (res.success) {
                        alertSuccess("Learned successfully");
                    } else {
                        alertError("Learn command failed");
                    }
                });
            },
            sendClear() {
                ws.send({action: "lightfoxClear"}, (res) => {
                    if (res.success) {
                        alertSuccess("Cleared successfully");
                    } else {
                        alertError("Clear command failed");
                    }
                });
            }
        }
    };
</script>

<style lang="less">

</style>
