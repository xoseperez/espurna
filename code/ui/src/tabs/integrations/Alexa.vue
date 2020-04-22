<template>
    <section v-loading="!alexa">
        <div class="header">
            <h1>Alexa</h1>
            <h2>
                Configure the alexa integration.
            </h2>
        </div>

        <Group v-model="alexa" class="page form" #default>
            <fieldset>
                <Row>
                    <C>
                        <label>Alexa integration</label>
                    </C>
                    <C>
                        <Inpt type="switch" name="enabled"
                              tabindex="1"/>
                    </C>
                </Row>
                <Row>
                    <C>
                        <label>Alexa device name</label>
                    </C>
                    <C>
                        <Inpt name="name"
                              maxlength="31"
                              type="text"
                              action="reboot"
                              tabindex="2"/>
                        <Hint>
                            This name will be used in Alexa integration.
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


    //TODO alexa name by switch
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
            alexa: Object
        }
    };
</script>

<style scoped>

</style>
