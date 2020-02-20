<template>
    <Repeater v-model="list" :max="max" @created="({row})=>$set(row, 'more', false)">
        <template #default="tpl">
            <Row>
                <C><label :for="'ssid-'+tpl.row.key">Network SSID</label></C>
                <C no-wrap>
                    <Inpt :id="'ssid-'+tpl.row.key" name="ssid"
                          type="text"
                          action="reconnect"
                          tabindex="0"
                          placeholder="Network SSID"
                          autocomplete="network-ssid"/>
                    <Btn @click="() => { $set(tpl.row, 'more', !tpl.row.more) }">...</Btn>
                </C>
                <template v-if="tpl.row.more">
                    <C><label>Password</label></C>
                    <C>
                        <Inpt name="pass"
                              type="password"
                              action="reconnect"
                              tabindex="0"
                              autocomplete="network-password"
                              spellcheck="false"/>
                    </C>

                    <C><label>Static IP</label></C>
                    <C>
                        <Inpt name="ip"
                              type="text"
                              action="reconnect"
                              maxlength="15"
                              tabindex="0"
                              autocomplete="false"/>
                        <Hint>Leave empty for DHCP negotiation</Hint>
                    </C>

                    <C><label>Gateway IP</label></C>
                    <C>
                        <Inpt name="gw"
                              type="text"
                              action="reconnect"
                              maxlength="15"
                              tabindex="0"
                              autocomplete="false"/>
                        <Hint>Set when using a static IP</Hint>
                    </C>

                    <C><label>Network Mask</label></C>
                    <C>
                        <Inpt name="mask"
                              type="text"
                              action="reconnect"
                              placeholder="255.255.255.0"
                              maxlength="15"
                              tabindex="0"
                              autocomplete="false"/>
                        <Hint>Usually 255.255.255.0 for /24 networks</Hint>
                    </C>

                    <C><label>DNS IP</label></C>
                    <C>
                        <Inpt name="dns"
                              type="text"
                              action="reconnect"
                              value=""
                              maxlength="15"
                              tabindex="0"
                              autocomplete="false"/>
                        <Hint>
                            Set the Domain Name Server IP to use when using a static IP
                        </Hint>
                    </C>
                </template>
            </Row>
        </template>
        <template #btnRemove="tpl">
            <Btn name="del-network" color="danger"
                 @click="deleteWifi(tpl)">
                Delete network
            </Btn>
        </template>
        <template #btnAdd="tpl">
            <Btn name="add-network" @click="tpl.click">Add network</Btn>
        </template>
    </Repeater>
</template>

<script>
    import Repeater from "./Repeater";
    import Hint from "./Hint";
    import Inpt from "./Input";
    import Btn from "./Button";
    import Row from "../layout/Row";
    import C from "../layout/Col";

    export default {
        components: {
            Repeater, Hint, Inpt, Btn, Row, C
        },
        props: {
            value: {
                type: Array,
                default: () => ([])
            },
            max: {
                type: Number,
                default: 5
            }
        },
        data() {
            return {
                list: this.value
            };
        },
        watch: {
            list() {
                if (this.list !== this.value) {
                    this.$emit("input", this.list);
                }
            },
            value() {
                this.list = this.value;
            }
        },
        methods: {
            deleteWifi(tpl) {
                let hardcoded = false;
                for(let i = tpl.k; i < this.list.length; ++i) {
                    if (this.list[i].hardcoded) {
                        hardcoded = true;
                    }
                }
                if (hardcoded) {
                    //We cannot delete it if it was hardcoded or if there is a hardcoded value after
                    Object.keys(tpl.value).forEach((k) => {
                        if (k !== "hardcoded")
                            {tpl.value[k] = "";}
                    });
                } else {
                    tpl.click();
                }
            }
        }
    };
</script>

<style lang="less">

</style>
