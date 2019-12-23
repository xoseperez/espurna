<template>
    <el-card class="device" shadow="hover">
        <el-dialog title="Authentication"
                   custom-class="authentication-dialog"
                   :visible.sync="authDialogVisible"
                   width="30%"
                   append-to-body>
            <el-row>
                <el-col>
                    <label> Username
                        <el-input v-model="username" placeholder="Username"/>
                    </label>
                </el-col>
            </el-row>
            <el-row>
                <el-col>
                    <label> Password
                        <el-input v-model="password" show-password placeholder="Password"/>
                    </label>
                </el-col>
            </el-row>
            <span slot="footer" class="dialog-footer">
                <el-button @click="authDialogVisible = false">Cancel</el-button>
                <el-button type="primary" @click="testAuth">Confirm</el-button>
            </span>
        </el-dialog>

        <el-dialog title="Upgrade"
                   :visible.sync="upgradeDialogVisible"
                   width="40%"
                   center
                   append-to-body>
            <el-upload ref="upload" :action="'http://'+ip+'/upgrade'" accept=".bin"
                       :before-upload="beforeUpload"
                       :auto-upload="false"
                       drag>
                <i class="el-icon-upload"></i>
                <div class="el-upload__text">Drop file here or <em>click to upload</em></div>
                <div slot="tip" class="el-upload__tip">.bin file with a size less than {{freeSize/1024}}Kb</div>
            </el-upload>
            <span slot="footer" class="dialog-footer">
                <el-button @click="upgradeDialogVisible = false">Cancel</el-button>
                <el-button v-if="canUpgrade" type="primary" @click="upgrade">Upgrade</el-button>
            </span>
        </el-dialog>
        <div slot="header" class="clearfix">
            <el-button-group class="buttons">
                <el-button type="success" title="Authentication" icon="el-icon-unlock"
                           size="small"
                           circle
                           @click="authDialogVisible = true"/>
                <el-button type="info" title="Configure" icon="el-icon-setting"
                           size="small"
                           circle
                           @click="configure"/>
                <el-button type="warning" title="Upgrade" circle
                           icon="el-icon-upload" size="small"
                           @click="upgradeDialogVisible = true"/>
                <el-button type="danger" circle title="Remove" icon="el-icon-close" size="small"/>
            </el-button-group>
            <div class="name" :title="hostname">
                {{hostname}}
            </div>
            <div v-if="description" class="description">
                {{description}}
            </div>
        </div>
        <el-row>
            <el-col :span="6">
                <el-progress :width="60" type="dashboard"
                             :percentage="wifiPercent"
                             :color="color"
                             :format="() => { return 'Wifi' }"
                             :status="!rssi ? 'exception' : undefined"/>
            </el-col>
            <el-col :span="18" class="vertical-center">
                <div v-if="wifi">Name: {{wifi}}</div>
                <div>IP: {{ip}}</div>
            </el-col>
        </el-row>
        <el-row>
            <el-col :span="16" class="board">
                Board: {{device}}
            </el-col>
            <el-col :span="8" class="version">
                Version: {{version}}
            </el-col>
        </el-row>
    </el-card>
</template>

<script>
    import compareVersions from "compare-version";
    import mixColors from "./mix-colors";

    // #!if ENV === 'development'
    import fetch from "./mock-device";
    // #!endif

    export default {
        components: {},
        props: {
            hostname: {
                type: String,
                default: 'Espurna'
            },
            description: {
                type: String,
                default: ""
            },
            ip: {
                type: String,
                required: true
            },
            device: {
                type: String,
                default: "Unknown board"
            },
            version: {
                type: String,
                default: process.env.VUE_APP_VERSION
            },
            freeSize: {
                type: Number,
                default: 0
            },
            wifi: {
                type: String,
                default: ""
            },
            rssi: {
                type: Number,
                default: null
            }
        },
        data() {
            return {
                authDialogVisible: false,
                upgradeDialogVisible: false,
                username: 'admin',
                password: 'fibonacci'
            }
        },
        computed: {
            canUpgrade() {
                return this.$refs && this.$refs.upload && this.$refs.upload.fileList;
            },
            wifiPercent() {
                if (!this.rssi) {
                    return 0;
                }
                let rssi = Math.abs(this.rssi);
                return Math.min(100, Math.max(0, 100 * (1 - Math.pow(rssi - 40, 2) / Math.pow(50, 2))))
            },
            color() {
                return mixColors('#479fd6', '#db3a22', this.wifiPercent);
            }
        },
        methods: {
            testAuth() {
                let loading = this.$loading({
                    target: '.authentication-dialog'
                });
                const success = () => {
                    this.$notify.success({
                        title: 'Authentication successful',
                        message: this.hostname
                    })
                };
                const fail = () => {
                    this.$notify.error({
                        title: 'Authentication failed',
                        message: this.hostname
                    })
                };
                fetch('http://' + this.ip + '/auth', {
                    headers: new Headers({
                        'Authorization': `Basic ${btoa(`${this.username}:${this.password}`)}`,
                        'Content-Type': 'application/x-www-form-urlencoded'
                    }),
                }).then((resp) => {
                    loading.close();
                    if (resp.ok) {
                        this.authDialogVisible = false;
                        success();
                    } else {
                        fail();
                    }
                }).catch(() => {
                    loading.close();
                    fail();
                });
            },
            compareVersions(a, b) {
                return compareVersions(a, b);
            },
            configure() {
                console.log(this.compareVersions(this.version, '2'));
                if (this.compareVersions(this.version, '2') >= 0) {
                    this.$emit('configure', this.username, this.password);
                } else {
                    const win = window.open('http://' + this.username + ':' + this.password + '@' + this.ip, '_blank');
                    win.focus();
                }
                this.authDialogVisible = false
            },
            beforeUpload(file) {
                console.log(file.type);

                const hasEnoughSpace = file.size < this.freeSize;

                if (!hasEnoughSpace) {
                    this.$message.error('There is not enough space on the board to upgrade the firmware OTA, consider doing a two step update');
                }
            },
            upgrade() {
                return this.$refs.upload.submit();
            },
        }
    }
</script>

<style lang="less">
    .device {
        display: flex;
        flex-wrap: wrap;
        text-align: left;
        letter-spacing: 0.0125em;
        word-break: break-all;
        color: #777;

        .vertical-center {
            display: flex;
            flex-direction: column;
            justify-content: center;
        }

        .el-card__body {
            font-size: .9em;
            padding-top: 5px;
            padding-bottom: 10px;
        }

        .name {
            color: #333;
            float: left;
            line-height: 2em;
            font-size: 1.1em;
            width: 65%;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }

        .version {
            text-align: right;
        }

        .board, .version {
            font-size: .7em;
        }

        .description {
            padding-top: 10px;
            clear: both;
        }

        .buttons {
            float: right;
        }
    }
</style>
