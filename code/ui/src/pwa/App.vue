<template>
    <div>
        <App v-if="singleAddress" :address="singleAddress" :close="() => singleAddress = null"/>
        <div v-else class="pwa-content">
            <div class="pwa-icon">
                <Icon/>
                <h1>Espurna</h1>
            </div>
            <el-row :gutter="10" class="options">
                <el-col :span="16">
                    <el-input v-model="scanIp" placeholder="192.168.1.1-255"/>
                </el-col>
                <el-col :span="8">
                    <el-button type="primary" @click="retrieveDevices">Scan for devices</el-button>
                </el-col>
                <!-- <Button>Add device manually</Button>-->
            </el-row>
            <el-row :gutter="12" class="devices">
                <el-col v-for="(device, ip) in devices" :key="ip" :span="8">
                    <Device :ip="ip"
                            v-bind="device"
                            @configure="() => singleAddress = ip"/>
                </el-col>
            </el-row>
        </div>
    </div>
</template>

<script>
    import Icon from '../../public/icons/icon.svg';
    import getUserIp from './get-user-ip';
    import App from "../App";
    import Device from "./Device";

    export default {
        components: {Icon, Device, App},
        data() {
            return {
                scanIp: "",
                userIp: "",
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
                console.log(ip);
                this.userIp = ip;
            });
            // setInterval(this.retrieveDevices, 60000); //Rescan every minute
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
    .el-row {
        margin-top: 10px;
        margin-bottom: 10px;
        display: flex;
        flex-wrap: wrap;
    }
    .el-input input.el-input__inner {
        margin: 0;
    }

    .el-col > .el-button {
        width: 100%;
    }

    .pwa-content {
        transform: translate(-50%, -50%);
        position: absolute;
        top: 45%;
        left: 50%;
        max-width: 1000px;
        width: 100%;
        font-family: Roboto, sans-serif;

        .el-card > * {
            width: 100%;
        }

        .pwa-icon {
            width: 20vh;
            text-align: center;
            margin: auto;
            font-size: 2vh;

            h1 {
                margin-top: -.2em;
                text-transform: uppercase;
            }
        }

        .devices {
            /*display: flex;
            flex-wrap: wrap;
            margin: 0 auto;

            .device {
                width: 32%;
                margin: 1%;
            }*/
        }
    }
</style>