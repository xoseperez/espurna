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
    .btn {
        display: inline-block;
        line-height: normal;
        white-space: nowrap;
        vertical-align: middle;
        text-align: center;
        cursor: pointer;
        user-select: none;
        box-sizing: border-box;
        border: none;
        text-decoration: none;
        border-radius: 4px;
        color: white;
        letter-spacing: 0;
        margin-bottom: 10px;
        text-shadow: 0 1px 1px rgba(0, 0, 0, 0.2);
        padding: 8px 8px;

        &:focus, &:hover {
            outline: 0;
            background-image: linear-gradient(transparent, rgba(0, 0, 0, .05) 40%, rgba(0, 0, 0, .1))
        }
        &:active {
            box-shadow: 0 0 0 1px rgba(0, 0, 0, .15) inset, 0 0 6px rgba(0, 0, 0, .2) inset
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


    .pure-btn-primary {
        background-color: #0078e7;
        color: #fff
    }

    .main-buttons {
        margin: 20px auto;
        text-align: center;
    }

    .main-buttons button {
        width: 100px;
    }

    .btn-del-schedule {
        margin-top: 15px;
    }

    .btn-danger {
        background: #C03420; /* redish */
    }

    .btn-success {
        background: #58C04E;
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

    .btn-add-switch-schedule,
    .btn-add-light-schedule {
        display: none;
    }

    .btn-accent {
        background: #FF8000;
    }

    .btn-info {
        background: #42B8DD;
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

    .btn-upgrade-browse,
    .btn-clear-filters,
    .btn-clear-messages,
    .btn-clear-counts,
    .btn-dbgcmd,
    .btn-ha-add,
    .btn-apikey,
    .btn-upgrade {
        margin-left: 5px;
    }
</style>