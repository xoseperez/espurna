<template>
    <form class="pure-form">
        <slot></slot>
    </form>
</template>

<script>
    export default {
        data() {
            return {
                values: {}
            }
        },
        watch: {
            values(newVal, oldVal) {
                //console.log(newVal);
            }
        },
        provide() {
            return {
                $form: () => ({
                    values: this.values
                })
            }
        }
    }
</script>

<style lang="less">

    .pure-form {
        label {
            margin: .5em 0 .2em
        }

        fieldset {
            margin: 0;
            padding: .35em 0 .75em;
            border: 0
        }

        legend {
            display: block;
            width: 100%;
            padding: .3em 0;
            margin-bottom: .3em;
            color: #333;
            border-bottom: 1px solid #e5e5e5
        }


        .pure-input-rounded, input.pure-input-rounded {
            border-radius: 2em;
            padding: .5em 1em
        }

        .pure-input-1 {
            width: 100%
        }

        .pure-input-3-4 {
            width: 75%
        }

        .pure-input-2-3 {
            width: 66%
        }

        .pure-input-1-2 {
            width: 50%
        }

        .pure-input-1-3 {
            width: 33%
        }

        .pure-input-1-4 {
            width: 25%
        }

        .pure-help-inline, .pure-form-message-inline {
            display: inline-block;
            padding-left: .3em;
            color: #666;
            vertical-align: middle;
            font-size: .875em
        }

        legend {
            font-weight: bold;
            letter-spacing: 0;
            margin: 10px 0 1em 0;
        }

        .pure-g > label {
            margin: .4em 0 .2em;
        }
    }

    .pure-form-aligned .pure-help-inline, .pure-form-aligned input, .pure-form-aligned select, .pure-form-aligned textarea, .pure-form-message-inline {
        display: inline-block;
        vertical-align: middle
    }

    .pure-form-aligned textarea {
        vertical-align: top
    }

    .pure-form-aligned .pure-control-group {
        margin-bottom: .5em
    }

    .pure-form-aligned .pure-control-group label {
        text-align: right;
        display: inline-block;
        vertical-align: middle;
        width: 10em;
        margin: 0 1em 0 0
    }

    .pure-form-aligned .pure-controls {
        margin: 1.5em 0 0 11em
    }

    .pure-form-message {
        display: block;
        color: #666;
        font-size: .875em
    }

    @media only screen and (max-width: 480px) {
        .pure-form button[type=submit] {
            margin: .7em 0 0
        }

        .pure-form-aligned .pure-control-group label {
            margin-bottom: .3em;
            text-align: left;
            display: block;
            width: 100%
        }

        .pure-form-aligned .pure-controls {
            margin: 1.5em 0 0 0
        }

        .pure-form .pure-help-inline, .pure-form-message, .pure-form-message-inline {
            display: block;
            font-size: .75em;
            padding: .2em 0 .8em
        }
    }
</style>