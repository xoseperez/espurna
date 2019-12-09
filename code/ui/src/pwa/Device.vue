<template>
    <el-card class="device" shadow="hover">
        <div slot="header" class="clearfix">
            <el-button-group class="buttons">
                <el-button type="danger" title="Upgrade" icon="el-icon-upload" size="small" @click="$emit('upgrade')"/>
                <el-button type="primary" title="Configure" icon="el-icon-setting"
                           size="small"
                           @click="$emit('configure')"/>
            </el-button-group>
            <div class="name">
                {{hostname}}
            </div>
            <div v-if="description" class="description">
                {{description}}
            </div>
        </div>
        <el-row>
            <el-col :span="12" class="ip">
                {{ip}}
            </el-col>
            <el-col :span="12" class="version">
                {{version}}
            </el-col>
        </el-row>
        <el-row>
            <el-col class="board">
                {{device}}
            </el-col>
        </el-row>
    </el-card>
</template>

<script>
    import compareVersions from "compare-version";

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
            spaceAvailable: {
                type: Number,
                default: 0
            }
        },
        methods: {
            compareVersions(a, b) {
                return compareVersions(a, b);
            }
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

        .name {
            color: #333;
            float: left;
            line-height: 2em;
            font-size: 1.1em;
        }

        .version {
            text-align: right;
        }

        .description {
            padding-top: 10px;
        }

        .buttons {
            float: right;
        }
    }
</style>