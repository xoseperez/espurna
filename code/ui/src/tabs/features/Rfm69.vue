<template>
    <section>
        <div class="header">
            <h1>MAPPING</h1>
            <h2>
                Configure the map between nodeID/key and MQTT topic. Messages from the given nodeID with the
                given key will be forwarded to the specified topic.
                You can also configure a default topic using {nodeid} and {key} as placeholders.
                Messages without a defined mapping will be discarded.
            </h2>
        </div>

        <Group v-model="rfm69" class="page form">
            <fieldset>
                <Row>
                    <C>
                        <label>Default topic</label>
                    </C>
                    <C>
                        <Inpt name="topic"
                              type="text"
                              size="8"
                              tabindex="1"
                              placeholder="Default MQTT Topic (use {nodeid} and {key} as placeholders)"/>
                    </C>
                </Row>

                <Row>
                    <C><label>Specific topics</label></C>
                    <Repeater>
                        <template #default="tpl">
                            <Row>
                                <C :size="3">
                                    <Inpt name="node"
                                          type="text"
                                          size="8"
                                          placeholder="Node ID"
                                          autocomplete="false"/>
                                </C>
                                <C :size="3">
                                    <Inpt name="key"
                                          type="text"
                                          size="8"
                                          placeholder="Key"/>
                                </C>
                                <C :size="4">
                                    <Inpt name="topic"
                                          type="text"
                                          size="8"
                                          placeholder="MQTT Topic"/>
                                </C>
                            </Row>
                        </template>
                        <template #btnRemove="tpl">
                            <Btn name="del-mapping" @click="tpl.click">Del</Btn>
                        </template>
                        <template #btnAdd="tpl">
                            <Btn name="add-mapping" @click="tpl.click">Add</Btn>
                        </template>
                    </Repeater>
                </Row>
            </fieldset>
            <fieldset>
                <legend>Messages</legend>
                <h2>
                    Messages being received. Previous messages are not displayed.
                    You have to keep the page open in order to keep receiving them.
                    You can filter/unfilter by clicking on the values.
                    Left click on a value to show only rows that match that value, middle click to show all
                    rows
                    but those matching that value.
                    Filtered colums have red headers.
                </h2>

                <DataTable v-model="packets" :columns="columns" unique-key="packetID"/>
                <table id="packets">
                    <thead>
                        <tr>
                            <th>Timestamp</th>
                            <th>SenderID</th>
                            <th>PacketID</th>
                            <th>TargetID</th>
                            <th>Key</th>
                            <th>Value</th>
                            <th>RSSI</th>
                            <th>Duplicates</th>
                            <th>Missing</th>
                        </tr>
                    </thead>
                    <tbody>
                    </tbody>
                </table>

                <Btn name="clear-filters">Clear filters</Btn>
                <Btn name="clear-messages">Clear messages</Btn>
                <Btn name="clear-counts">Clear counts</Btn>
            </fieldset>
        </Group>
    </section>
</template>

<script>
    import Inpt from "./../../components/Input";
    import Row from "../../layout/Row";
    import DataTable from "../../components/DataTable";
    import C from "../../layout/Col";
    import Btn from "../../components/Button";
    import Repeater from "../../components/Repeater";
    import Group from "../../components/Group";

    export default {
        components: {
            Group,
            Repeater,
            Btn,
            C,
            DataTable,
            Row,
            Inpt
        },
        inheritAttrs: false,
        data() {
            return {
                columns: [
                    {
                        key: 'packetID',
                        title: 'Packet ID'
                    },
                    {
                        key: 'senderID',
                        title: 'Sender ID'
                    },
                    {
                        key: 'targetID',
                        title: 'Target ID'
                    },
                    {
                        key: 'key',
                        title: 'Key'
                    },
                    {
                        key: 'value',
                        title: 'Value'
                    },
                    {
                        key: 'rssi',
                        title: 'RSSI'
                    },
                    {
                        key: 'duplicates',
                        title: 'Duplicates'
                    },
                    {
                        key: 'missing',
                        title: 'Missing'
                    },
                    {
                        key: 'time',
                        title: 'Time'
                    },
                ]
            }
        }
    }
</script>

<style lang="less">

</style>