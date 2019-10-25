<template>
    <button type="button" :class="classes" @click="$emit('click')">
        <slot></slot>
    </button>
</template>

<script>
    export default {
        props: {
            color: {
                type: String,
                default: "success"
            },
            name: {
                type: String,
                default: ""
            }
        },
        computed: {
            classes() {
                return "btn btn-" + this.color + (this.name ? " btn-" + this.name : "")
            }
        }

    }
</script>

<style lang="less">
    @import "../assets/Colors";

    .btn {
        display: inline-block;
        line-height: normal;
        white-space: nowrap;
        vertical-align: middle;
        text-align: center;
        cursor: pointer;
        user-select: none;
        text-decoration: none;
        text-transform: uppercase;
        border-radius: 4px;
        color: white;
        letter-spacing: 0;
        margin: 5px 0;
        text-shadow: 0 1px 1px rgba(0, 0, 0, 0.2);
        padding: 10px 14px;
        border: none;
        background-image: linear-gradient(transparent, rgba(0, 0, 0, .05) 40%, rgba(0, 0, 0, .2));

        &:focus, &:hover {
            outline: 0;
            background-image: linear-gradient(transparent, rgba(0, 0, 0, .1) 40%, rgba(0, 0, 0, .4))
        }
        &:active {
            background-image: linear-gradient(transparent, rgba(255, 255, 255, .05) 40%, rgba(255, 255, 255, .2))
        }
        &[disabled] {
            border: none;
            background-image: none;
            opacity: .4;
            cursor: not-allowed;
            box-shadow: none;
            pointer-events: none
        }
    }

    .content .btn:not(:first-child) {
        margin-left: 8px;
    }

    .btn-del-schedule {
        margin-top: 15px;
    }

    .btn-danger {
        background-color: @error; /* redish */
        /*border-color: #671c11;*/
    }

    .btn-success {
        background-color: @success;
        /*border-color: #225317;*/
    }

    /*
        .btn-reboot,
        .btn-reconnect,
        .btn-ha-del,
        .btn-rfb-forget,
        .btn-lightfox-clear,
        .btn-del-network,
        .btn-del-mapping,
        .btn-del-schedule,
        .btn-dbg-clear,
        .btn-upgrade,
        .btn-clear-filters,
        .btn-clear-messages,
        .btn-clear-counts,
        .btn-settings-factory {
            background: rgb(192, 0, 0);
        }*/
    /*
        .btn-update,
        .btn-update-password,
        .btn-add-network,
        .btn-add-mapping,
        .btn-upgrade-browse,
        .btn-rfb-learn,
        .btn-lightfox-learn,
        .btn-ha-add,
        .btn-ha-config,
        .btn-settings-backup,
        .btn-settings-restore,
        .btn-dbgcmd,
        .btn-apikey,
        .btn-add-switch-schedule,
        .btn-add-light-schedule {
            background: rgb(0, 192, 0);
        }*/

    .btn-primary {
        background-color: @primary;
        /*border-color: #844d18;*/
    }

    .btn-secondary {
        background-color: @secondary;
        /*border-color: #1e5263;*/
    }

    /*
        .btn-thermostat-reset-counters,
        .btn-more-network,
        .btn-more-schedule,
        .btn-wifi-scan,
        .btn-rfb-send {
            background: rgb(255, 128, 0);
        }

        .btn-generate-password {
            background: rgb(66, 184, 221);
        }*/

</style>