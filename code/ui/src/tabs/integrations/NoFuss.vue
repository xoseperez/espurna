<template>
    <section v-loading="!nofuss">
        <div class="header">
            <h1>NoFUSS</h1>
            <h2>
                Automatically upgrade the firmware (see <A href="https://bitbucket.org/xoseperez/nofuss">xoseperez/nofuss</A>
                for details)
            </h2>
        </div>

        <Group v-model="nofuss" class="page form" #default>
            <fieldset>
                <Row>
                    <C><label>Enable</label></C>
                    <C>
                        <Inpt type="switch" name="enabled"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Server</label></C>
                    <C>
                        <Inpt name="server" type="text"/>
                        <Hint>Address of the NoFUSS server</Hint>
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
    import A from "../../components/ExtLink";
    import Group from "../../components/Group";

    export default {
        components: {
            Group,
            A,
            C,
            Row,
            Hint,
            Inpt
        },
        inheritAttrs: false,
        props: {
            nofuss: Object
        }
    };
</script>

<style lang="less">

</style>
