<template>
    <div>
        <App v-if="singleAddress" :address="singleAddress" @close="() => singleAddress = null"/>
        <div v-else class="pwa-content">
            <div class="devices">
                <Device v-for="(device, ip) in devices" :key="ip" :ip="ip" v-bind="device"
                        @configure="() => singleAddress = ip"/>
            </div>
            <Row class="options" no-wrap>
                <Input v-model="scanIp" placeholder="192.168.1.1-255"/>
                <Button color="primary" @click="retrieveDevices">Scan for devices</Button>
                <!-- <Button>Add device manually</Button>-->
            </Row>
        </div>
    </div>
</template>

<script>
    import getUserIp from './get-user-ip';
    import App from "../App";
    import Device from "./Device";
    import Button from "../components/Button";
    import Input from "../components/Input";
    import Row from "../layout/Row";

    export default {
        components: {Row, Input, Button, Device, App},
        data() {
            return {
                scanIp: null,
                userIp: null,
                singleAddress: null,
                devices: {}
            }
        },
        watch: {
            userIp(newIp) {
                //We are connected to an espurna device, show single device interface
                if (newIp === '192.168.4.1' || newIp === '192.168.241.1') {
                    this.singleAddress = newIp;
                } else {
                    this.scanIp = newIp.split('.');
                    this.scanIp.pop();
                    this.scanIp.push('1-255');
                    this.scanIp = this.scanIp.join('.');
                }
            }
        },
        mounted() {
            //this.devices = localStorage.devices;
            getUserIp((ip) => {
                this.userIp = ip;
            });
            setInterval(this.retrieveDevices, 60000); //Rescan every minute
        },
        methods: {
            retrieveDevices() {
                let parts = this.scanIp.split('.');
                this.rangeIterate(parts, (ip) => {
                    ip = ip.join('.');
                    fetch('http://' + ip + '/discover').then(response => {
                        response.json().then(data => {
                            this.$set(this.devices, ip, data);
                        });
                    }).catch(() => {
                        //Do nothing
                    });
                });
            },
            rangeIterate(parts, cb, r) {
                r = r || 0;

                if (r === parts.length) {
                    cb(parts);
                } else {
                    let range = parts[r].split('-', 2);
                    if (range.length > 1) {
                        let i;

                        let ranged_parts = [...parts];

                        for (i = parseInt(range[0]); i <= parseInt(range[1]); ++i) {
                            ranged_parts[r] = i;
                            this.rangeIterate(ranged_parts, cb, r + 1);
                        }
                    } else {
                        this.rangeIterate(parts, cb, r + 1);
                    }
                }
            },

        }
    }
</script>

<style lang="less">
    .pwa-content {
        transform: translate(-50%, -50%);
        position: absolute;
        top: 50%;
        left: 50%;
        max-width: 1000px;

        .devices {
            display: flex;
            flex-wrap: wrap;
            margin: 0 auto;
            font-family: Roboto, sans-serif;

            .device {
                width: 32%;
                margin: 1%;
            }
        }
    }
</style>