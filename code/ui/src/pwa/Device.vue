<template>
    <div class="device">
        <div class="name">
            {{hostname}}
        </div>
        <div class="ip">
            {{ip}}
        </div>
        <div class="version">
            {{version}}
        </div>
        <div class="board">
            {{device}}
        </div>
        <div class="description">
            {{description}}
        </div>
        <div class="buttons">
            <Button @click="$emit('upgrade')">Upgrade</Button>
            <Button @click="$emit('configure')">Configure</Button>
        </div>
    </div>
</template>

<script>
    import Button from "../components/Button";
    import compareVersions from "compare-version";

    export default {
        components: {Button},
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
            width: 100%;
            line-height: 2em;
            font-size: 1.2em;
        }
        .ip {
            width: 50%;
        }
        .version {
            width: 50%;
            text-align: right;
        }
        .description {
            padding-top: 10px;
        }
    }
</style>