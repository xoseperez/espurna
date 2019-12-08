<template>
    <div class="content">
        <App v-if="singleAddress" :address="singleAddress" @close="() => singleAddress = null"/>
        <template v-else>
            <div class="devices">
                <Device v-for="(device, ip) in devices" :key="ip" v-bind="device"
                        @configure="() => singleAddress = ip"/>
            </div>
            <div class="options">
                <Input v-model="scanIp"/>
                <Button @click="retrieveDevices">Scan for devices</Button>
                <Button>Add device manually</Button>
            </div>
        </template>
    </div>
</template>

<script>
    import getUserIp from './get-user-ip';
    import App from "../App";
    import Device from "./Device";
    import Button from "../components/Button";
    import Input from "../components/Input";

    export default {
        components: {Input, Button, Device, App},
        data() {
            return {
                scanIp: null,
                userIp: null,
                singleAddress: null,
                devices: {}
            }
        },
        watch: {
            userIp(newIp, oldIp) {
                //We are connected to an espurna device, show single device interface
                if (newIp === '192.168.4.1' || newIp === '192.168.241.1') {
                    this.singleAddress = newIp;
                }
            }
        },
        mounted() {
            this.devices = localStorage.devices;
            getUserIp((ip) => {
                this.userIp = ip;
            });
            setInterval(this.retrieveDevices, 60000); //Rescan every minute
        },
        methods: {
            retrieveDevices() {
                let parts = this.ip.split('.');
                parts.pop();
                parts = parts.join('.') + '.';
                let i;
                for (i = 0; i < 256; ++i) {
                    const iptoCheck = parts + i;
                    //TODO api call on ip
                }
            }
        }
    }
</script>

<style lang="less">
    .devices {
        display: flex;
        flex-wrap: wrap;
        max-width: 1400px;
        position: absolute;
        top: 50%;
        transform: translateY(-50%);
        margin: 0 auto;
        font-family: Roboto, sans-serif;

        .device {
            width: 23%;
            margin: 1%;
        }
    }
</style>