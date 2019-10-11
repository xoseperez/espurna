<template>
    <section>
        <div class="header">
            <h1>MAPPING</h1>
            <h2>
                Configure the map between nodeID/key and MQTT topic. Messages from the given nodeID with
                the
                given key will be forwarded to the specified topic.
                You can also configure a default topic using {nodeid} and {key} as placeholders, if the
                default topic is empty messages without defined map will be discarded.
            </h2>
        </div>

        <div class="page">
            <fieldset>
                <legend>Default topic</legend>

                <div class="pure-g">
                    <Inpt name="rfm69Topic"
                          type="text"
                          class="pure-u-23-24"
                          value=""
                          size="8"
                          tabindex="41"
                          placeholder="Default MQTT Topic (use {nodeid} and {key} as placeholders)"/>
                </div>

                <legend>Specific topics</legend>

                <div id="mapping"></div>
                <div id="nodeTemplate" class="template">
                    <div class="pure-g">
                        <div class="pure-u-md-1-6 pure-u-1-2">
                            <Inpt name="node"
                                  type="text"
                                  class="pure-u-11-12"
                                  value=""
                                  size="8"
                                  tabindex="0"
                                  placeholder="Node ID"
                                  autocomplete="false"/>
                        </div>
                        <div class="pure-u-md-1-6 pure-u-1-2">
                            <Inpt name="key"
                                  type="text"
                                  class="pure-u-11-12"
                                  value=""
                                  size="8"
                                  tabindex="0"
                                  placeholder="Key"/>
                        </div>
                        <div class="pure-u-md-1-2 pure-u-3-4">
                            <Inpt name="topic"
                                  type="text"
                                  class="pure-md-11-12 pure-u-23-24"
                                  value=""
                                  size="8"
                                  tabindex="0"
                                  placeholder="MQTT Topic"/>
                        </div>
                        <div class="pure-u-md-1-6 pure-u-1-4">
                            <button type="button" class="pure-button button-del-mapping pure-u-5-6 pure-u-md-5-6">Del
                            </button>
                        </div>
                    </div>
                </div>

                <button type="button" class="pure-button button-add-mapping">Add</button>
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

                <table id="packets" class="display" cellspacing="0">
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

                <button type="button" class="pure-button button-clear-filters">Clear filters</button>
                <button type="button" class="pure-button button-clear-messages">Clear messages</button>
                <button type="button" class="pure-button button-clear-counts">Clear counts</button>
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';

    export default {
        components: {
            Inpt
        }
    }
</script>

<style scoped>

</style>