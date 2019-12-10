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
                    <el-input v-model="scanIp" placeholder="IP range to scan for devices"/>
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
                scanIp: "192.168.1.1-255",
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
            setInterval(this.updateDevices, 3000); //Update wifi info every 3 second
        },
        methods: {
            updateDevices() {
                Object.keys(this.devices).forEach((ip) => {
                    fetch('http://' + ip + '/discover').then(response => {
                        if (response.ok) {
                            return response.json();
                        } else {
                            throw new Error('Not an espurna device');
                        }
                    }).then(data => {
                        this.devices[ip] = {...this.devices[ip], ...data};
                    }).catch((error) => {
                        return Promise.reject(error);
                        //Do nothing
                    }).catch(() => {
                        this.$set(this.device[ip], 'rssi', null);
                        this.$set(this.device[ip], 'status', 'ko');
                    });
                })
            },
            retrieveDevices() {
                let parts = this.scanIp.split('.');
                this.rangeIterate(parts, (ip) => {
                    ip = ip.join('.');
                    fetch('http://' + ip + '/discover').then(response => {
                        if (response.ok) {
                            return response.json();
                        } else {
                            throw new Error('Not an espurna device');
                        }
                    }).then(data => {
                        this.$set(this.devices, ip, data);
                    }).catch((error) => {
                        return Promise.reject(error);
                        //Do nothing
                    }).catch((error) => {
                        console.log(error);
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

    .el-button-group button {
        border-bottom: none;
        border-top: none;
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