<template>
    <section>
        <div class="header">
            <h1>LIGHTFOX RF</h1>
            <h2>
                LightFox RF configuration<br><br>
                This page allows you to control LightFox RF receiver options.<br><br>
                To learn a new code click <strong>LEARN</strong>, wait for 3 seconds then press a button
                on
                the remote, one of the relays will toggle. If no device relay toggles the code has not
                been
                properly learnt. Keep trying.<br><br>
                Delete all the codes by clicking the <strong>CLEAR</strong> button and wait for 10
                seconds.<br><br>
                You can also specify which RF button controls which relay using controls below.
            </h2>
        </div>

        <div class="page">
            <div class="pure-g">
                <label class="pure-u-1 pure-u-lg-1-4">RF Actions</label>
                <div class="pure-u-1-2 pure-u-lg-3-8">
                    <Btn name="lightfox-learn" class="pure-u-23-24">
                        Learn
                    </Btn>
                </div>
                <div class="pure-u-1-2 pure-u-lg-3-8">
                    <Btn name="lightfox-clear" color="danger" class="pure-u-1">
                        Clear
                    </Btn>
                </div>
            </div>

            <fieldset id="lightfoxNodes">
                <div v-for="b in buttons" :key="b.id" class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Button #{{b.id}}</label>
                    <Inpt type="select" class="pure-u-1 pure-u-lg-3-4" :name="'btnRelay'+b.id" :options="relayOptions" @change="$emit('reboot')"/>
                </div>
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';
    import Btn from "../../components/Button";

    export default {
        components: {
            Btn,
            Inpt
        },
        inheritAttrs: false,
        props: {
            buttons: {
                type: Array,
                default() {
                    return []
                }
            },
            relayOptions: {
                type: Array
            }
        }
    }
</script>

<style lang="less">

</style>