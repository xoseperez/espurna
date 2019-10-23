<template>
    <select v-if="type === 'select'" v-model="value" v-bind="$attrs">
        <option v-for="(l, k) in options" :key="key(k,l)" :value="key(k,l)">{{label(k,l)}}</option>
    </select>
    <span v-else-if="type === 'switch'" class="switch">
        <input :id="'switch-'+_uid" v-model="value" type="checkbox" v-bind="$attrs">
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
    .switch {
        /*
        overflow: hidden;
        width: auto;
        height: 30px;
        margin: 0 0 10px 0;
        padding: 0;
        border-radius: 4px;
        box-shadow: inset 1px 1px #CCC;*/

        input {
            opacity: 0;
            position: absolute;
            margin-top: 26px;
            z-index: -9;
        }

        label {
            position: relative;
            display: inline-block;
            min-width: 112px;
            cursor: pointer;
            font-weight: 500;
            text-align: left;
            padding: 4px 0 4px 44px;

            &:before, &:after {
                content: "";
                position: absolute;
                margin: 0;
                outline: 0;
                top: 50%;
                transform: translate(0, -50%);
                transition: all 0.3s ease;
            }

            &:before {
                left: 1px;
                width: 34px;
                height: 14px;
                background-color: #999;
                border-radius: 8px;
            }

            &:after {
                left: 0;
                width: 20px;
                height: 20px;
                background-color: #FAFAFA;
                border-radius: 50%;
                box-shadow: 0 3px 1px -2px rgba(0, 0, 0, 0.14), 0 2px 2px 0 rgba(0, 0, 0, 0.098), 0 1px 5px 0 rgba(0, 0, 0, 0.084);
            }
        }

        label .on,
        input:checked + label .off {
            display: none;
        }

        label .off,
        input:checked + label .on {
            display: inline-block;
        }

        input:checked + label:before {
            background-color: #5bc0de;
        }

        input:checked + label:after {
            background-color: #31b0d5;
            transform: translate(80%, -50%);
        }

        input[disabled] + label {
            &:before {
                background-color: #666;
            }

            &:after {
                background-color: #bbb;
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
        margin-bottom: 10px;
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

    input {
        margin-bottom: 10px;
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
