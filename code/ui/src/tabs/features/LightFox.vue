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

        <Group class="page form">
            <fieldset>
                <Row>
                    <C><label>RF Actions</label></C>
                    <C>
                        <Btn name="learn">
                            Learn
                        </Btn>
                    </C>
                    <C>
                        <Btn name="clear" color="danger">
                            Clear
                        </Btn>
                    </C>
                </Row>

                <Row v-for="b in lightfox.buttons" :key="b.id">
                    <C><label>Button #{{b.id}}</label></C>
                    <C>
                        <Inpt type="select" :name="'btnRelay'+b.id" :options="relayOptions"
                              @change="$emit('reboot')"/>
                    </C>
                </Row>
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

    export default {
        components: {
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
        }
    }
</script>

<style lang="less">

</style>