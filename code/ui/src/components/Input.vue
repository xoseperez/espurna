<template>
    <span v-if="type === 'select'" class="select">
        <select v-if="!multiple" ref="input" v-model="val" v-bind="$attrs" @change="(evt) => $emit('change', evt.target.value)">
            <option v-if="$attrs.placeholder" value="" disabled :selected="!val">{{$attrs.placeholder}}</option>
            <option v-for="{l, k} in _options" :key="k" :value="k">{{l}}</option>
        </select>
        <MultiSelect v-else v-model="val"
                     v-bind="$attrs" :options="_options"/>
        &nbsp;<span v-if="unit" class="unit">{{unit}}</span>
    </span>
    <span v-else-if="type === 'switch'" class="switch">
        <input :id="'switch-'+_uid" ref="input" v-model="val"
               type="checkbox" v-bind="$attrs"
               @change="(evt) => $emit('change', evt.target.value)">
        <span class="layer"></span>
        <label :for="'switch-'+_uid"><span class="on">{{on}}</span><span class="off">{{off}}</span></label>
    </span>
    <span v-else-if="type === 'password'" class="password">
        <input ref="input" v-model="val" :type="passType"
               v-bind="$attrs"
               @change="(evt) => $emit('change', evt.target.value)">
        <span class="no-select password-reveal" @click="togglePass">👁</span>
    </span>
    <span v-else-if="type === 'file'" class="file">
        <input ref="input"
               :type="type" v-bind="$attrs"
               :multiple="multiple"
               @change="(evt) => $emit('change', multiple ? evt.target.files : evt.target.files[0])">
    </span>
    <span v-else :class="'input input-'+type">
        <input ref="input" v-model="val" :type="type"
               v-bind="$attrs"
               @change="(evt) => $emit('change', evt.target.value)">
        &nbsp;<span v-if="unit" class="unit">{{unit}}</span>
    </span>
</template>

<script>
    import MultiSelect from "./MultiSelect";

    export default {
        components: {
            MultiSelect
        },
        inheritAttrs: false,
        props: {
            type: {
                type: String,
                default: "text"
            },
            name: {
                type: String,
                required: false
            },
            value: {
                type: [String, Number, Array],
                required: false
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
                type: [String, Number, Array],
                default: null
            },
            unit: {
                type: String,
                required: false
            },
            multiple: {
                type: Boolean,
                default: false
            }
        },
        data() {
            return {
                passType: this.type
            }
        },
        computed: {
            val: {
                get() {
                    return this.value ? this.value : (this.form && this.form.values && this.name in this.form.values ? this.form.values[this.name] : this.default);
                },
                set(v) {
                    if (this.name)
                        this.$set(this.form.values, this.name, v);
                    this.$emit('input', v);
                }
            },
            form() {
                return this.$form ? this.$form() : false;
            },
            _options() {
                let options = [];

                this.options.forEach((v, k) => {
                    options.push({k: this.key(k, v), l: this.label(k, v)})
                });

                return options;
            },
        },
        mounted() {
            if (this.val === undefined || this.val === null) {
                if (this.default !== null) {
                    this.val = this.default;
                } else if (this.type === 'select' && !this.placeholder) {
                    this.val = 0;
                } else if (this.type === 'number' && "min" in this.$attrs) {
                    this.val = this.$attrs.min;
                }
            }
        },
        inject: {$form: {name: '$form', default: false}},
        methods: {
            key(k, l) {
                return typeof l === 'object' ? l.k : k
            },
            label(k, l) {
                return typeof l === 'object' ? l.l : l
            },
            togglePass() {
                this.passType = (this.passType === 'text' ? 'password' : 'text');
            },
            setVisible() {
                this.passType = 'text';
            }
        },
    };
</script>

<style lang="less">
    @import "../assets/Colors";

    .unit {
        position: absolute;
        top: 50%;
        line-height: 1em;
        color: #aaa;
        right: 10px;
        font-size: 1.1em;
        transform: translateY(-50%);
    }


    .input, .password {
        position: relative;
        width: 100%;
    }

    .input-number input:hover + .unit,
    .input-number input:focus + .unit {
        right: 30px;
    }

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

            &[disabled] {
                cursor: not-allowed;

                ~ label {
                    background-color: desaturate(@error, 60%);
                }

                &:checked ~ label {
                    background-color: desaturate(@secondary, 60%);
                }

                + .layer {
                    background-color: desaturate(lighten(@error, 30%), 100%);
                }

                &:checked + .layer {
                    background-color: desaturate(lighten(@secondary, 30%), 100%);
                }
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

    .file {
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

    select {
        height: 2.25em;
        border: 1px solid #ccc;
        background-color: #fff;

        &[multiple] {
            height: auto
        }
    }

    .password-reveal {
        font-family: EmojiSymbols, Segoe UI Symbol;
        top: 50%;
        transform: translateY(-50%);
        line-height: 1em;
        position: absolute;
        right: 12px;
        font-size: 1.2em;
        user-select: none;
        cursor: pointer;
        color: #ccc;

        &:after {
            content: "";
            display: block;
            height: 2px;
            width: 100%;
            background: #ccc;
            transform: rotate(45deg);
            top: 44%;
            position: absolute;
        }
    }

    input[type="text"] + .password-reveal {
        color: @secondary;

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
