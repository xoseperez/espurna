<template>
    <section>
        <div class="header">
            <h1>Alexa</h1>
            <h2>
                Configure the alexa integration.
            </h2>
        </div>

        <div class="page">
            <fieldset>
                <Row>
                    <C>
                        <label>Alexa integration</label>
                    </C>
                    <C>
                        <Inpt type="switch" name="alexaEnabled"
                              tabindex="1"/>
                    </C>
                </Row>
                <Row>
                    <C><label>Alexa device name</label></C>
                    <C>
                        <Inpt name="alexaName"
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
        </div>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Hint from "../../components/Hint";
    import Row from "../../layout/Row";
    import C from "../../layout/Col";


    //TODO alexa name by switch
    export default {
        components: {
            C,
            Row,
            Hint,
            Inpt
        },
        inheritAttrs: false,
    }
</script>

<style scoped>

</style>