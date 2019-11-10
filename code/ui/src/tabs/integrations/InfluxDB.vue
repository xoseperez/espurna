<template>
    <section>
        <div class="header">
            <h1>INFLUXDB</h1>
            <h2>
                Configure the connection to your InfluxDB server. Leave the host field empty to disable InfluxDB
                connection.
            </h2>
        </div>

        <Group v-model="idb" class="page form">
            <fieldset>
                <Row>
                    <C><label>Enable InfluxDB</label></C>
                    <C>
                        <Inpt type="switch"
                              name="enabled"
                              tabindex="1"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Host</label></C>
                    <C>
                        <Inpt name="host" type="text" tabindex="2"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Port</label></C>
                    <C>
                        <Inpt name="port" type="text" tabindex="3"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Database</label></C>
                    <C>
                        <Inpt name="database" type="text"
                              tabindex="4"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Username</label></C>
                    <C>
                        <Inpt name="username"
                              type="text"
                              tabindex="5"
                              autocomplete="off"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Password</label></C>
                    <C>
                        <Inpt name="password"
                              type="password"
                              tabindex="6"
                              autocomplete="off"
                              spellcheck="false"/>
                    </C>
                </Row>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Row from "../../layout/Row";
    import C from "../../layout/Col";
    import Group from "../../components/Group";

    export default {
        components: {
            Group,
            C,
            Row,
            Inpt
        },
        inheritAttrs: false,
        props: {
            idb: {
                type: Object,
                default: () => ({})
            },
        },
    }
</script>

<style lang="less">

</style>