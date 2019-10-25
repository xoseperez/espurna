<template>
    <select v-if="type === 'select'" v-model="value" v-bind="$attrs">
        <option v-for="(l, k) in options" :key="key(k,l)" :value="key(k,l)">{{label(k,l)}}</option>
    </select>
    <span v-else-if="type === 'switch'" class="switch">
        <input :id="'switch-'+_uid" v-model="value" type="checkbox" v-bind="$attrs">
        <span class="layer"></span>
        <label :for="'switch-'+_uid"><span class="on">{{on}}</span><span class="off">{{off}}</span></label>
    </span>
    <span v-else-if="type === 'password'" class="password">
        <input v-model="value" :type="passType" v-bind="$attrs">
        <span class="no-select password-reveal" @click="togglePass">👁</span>
    </span>
    <input v-else-if="type === 'file'" :type="type"
           v-bind="$attrs" @change="(evt) => $emit('change', $attrs.multiple ? evt.target.files : evt.target.files[0])">
    <input v-else v-model="value" :type="type" v-bind="$attrs" @change="() => $emit('change')">
</template>

<script>
    export default {
        inheritAttrs: false,
        props: {
            type: {
                type: String,
                default: "text"
            },
            name: {
                type: String,
                required: true
            },
            on: {
                type: String,
                default: "Yes"
            },
            off: {
                type: String,
                default: "No"
            },
            options: {
                type: Array
            },
            default: {
                type: undefined,
                default: null
            }
        },
        data() {
            return {
                passType: this.type
            }
        },
        inject: {$form: {name: '$form', default: false}},
        computed: {
            value: {
                get() {
                    return this.form && this.form.values && this.name in this.form.values ? this.form.values[this.name] : this.default;
                },
                set(v) {
                    this.$set(this.form.values, this.name, v);
                }
            },
            form() {
                return this.$form ? this.$form() : false;
            }
        },
        methods: {
            key(k, l) {
                return typeof l === 'object' ? l.k : k
            },
            label(k, l) {
                return typeof l === 'object' ? l.l : l
            },
            togglePass() {
                console.log('reveal clicked');
                this.passType = (this.passType === 'text' ? 'password' : 'text');
            }
        },
    };
</script>

<style lang="less">
    @import "../assets/Colors";

    .switch {
        vertical-align: middle;
        display: inline-block;
        position: relative;
        width: 74px;
        height: 37px;
        overflow: hidden;
        border-radius: 4px;
        margin: 5px 0;
        border: 3px solid #fff;
        box-shadow: 0 0 0 1px #ccc;
        box-sizing: content-box;

        label, .layer {
            position: absolute;
            top: 0;
            right: 0;
            bottom: 0;
            left: 0;
        }

        input {
            position: relative;
            width: 100%;
            height: 100%;
            padding: 0;
            margin: 0;
            opacity: 0;
            cursor: pointer;
            z-index: 3;

            &:active ~ label {
                width: 56px;
            }

            &:checked:active ~ label {
                margin-left: -21px;
            }

            &:checked ~ label {
                left: 35px;
                background-color: @secondary;
                .off {
                    display: none;
                }
                .on {
                    display: inline;
                }
            }

            &:checked + .layer {
                background-color: lighten(@secondary, 40%);
                box-shadow: inset 0 0 2px 0 @secondary;
            }
        }

        .layer {
            width: 100%;
            background-color: lighten(@error, 40%);
            transition: all .3s ease;
            box-shadow: inset 0 0 2px 0 @error;
        }

        label {
            font-family: Arial, Helvetica, sans-serif;
            position: absolute;
            top: -3px;
            left: 4px;
            width: 35px;
            height: 29px;
            color: #fff;
            font-size: 13px;
            font-weight: bold;
            text-align: center;
            line-height: 18px;
            padding: 6px 3px;
            background-color: @error;
            border-radius: 2px;
            transition: all .3s ease, left .3s cubic-bezier(0.18, 0.89, 0.35, 1.15);

            .on {
                display: none;
            }
        }

    }

    input[type=file] {
        opacity: 0;
        position: absolute;
    }


    input:not([type=checkbox]):not([type=radio]), select, textarea {
        padding: .5em .6em;
        display: inline-block;
        border: 1px solid #ccc;
        box-shadow: inset 0 1px 3px #ddd;
        border-radius: 4px;
        vertical-align: middle;
        margin: 5px 0;
        width: 100%;

        @media only screen and (max-width: 480px) {
            margin-bottom: .3em;
            display: block
        }

        &:focus {
            outline: 0;
            border-color: #129fea;

            &:invalid {
                color: #b94a48;
                border-color: #e9322d
            }
        }

        &[disabled] {
            cursor: not-allowed;
            background-color: #eaeded;
            color: #777777;
        }

        &[readonly] {
            background-color: #eee;
            color: #777;
            border-color: #ccc
        }
    }

    input[type=color] {
        padding: .2em .5em
    }

    input[type=checkbox]:focus, input[type=radio]:focus {
        outline: 1px auto #129fea;

        &:invalid {
            outline-color: #e9322d
        }
    }

    select {
        height: 2.25em;
        border: 1px solid #ccc;
        background-color: #fff;

        &[multiple] {
            height: auto
        }
    }

    span.password {
        width: 100%;
    }

    .password-reveal {
        font-family: EmojiSymbols, Segoe UI Symbol;
        display: inline-block;
        float: right;
        z-index: 50;
        margin-top: 6px;
        margin-right: 10px;
        margin-left: -30px;
        vertical-align: middle;
        font-size: 1.2em;
        height: 100%;
        user-select: none;
        cursor: pointer;
        color: #ccc;
        position: relative;

        &:after {
            content: "";
            display: block;
            height: 2px;
            width: 100%;
            background: #ccc;
            transform: rotate(45deg);
            top: 46%;
            position: absolute;
        }
    }

    input[type="text"] + .password-reveal {
        color: rgba(66, 184, 221, 0.8);

        &:after {
            display: none;
        }
    }


    /* -----------------------------------------------------------------------------
        Password input controls
       -------------------------------------------------------------------------- */

    input::-ms-clear,
    input::-ms-reveal {
        display: none;
    }

    /* css minifier must not combine these.
     * style will not apply otherwise */
    input::-ms-input-placeholder {
        color: #ccd;
    }

    input::placeholder {
        color: #ccc;
    }
</style>
