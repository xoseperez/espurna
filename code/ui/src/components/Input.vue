<template>
    <select v-if="type === 'select'" v-model="value">
        <option v-for="(l, k) in options" :key="key(k,l)" :value="key(k,l)">{{label(k,l)}}</option>
    </select>
    <span v-else-if="type === 'switch'" class="switch">
        <input :id="'switch-'+_uid" v-model="value" type="checkbox">
        <label :for="'switch-'+_uid"><i class="on">{{on}}</i><i class="off">{{off}}</i></label>
    </span>
    <input v-else v-model="value" :type="type">
</template>

<script>
    export default {
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
        inject: {$form: {name: '$form', default: false}},
        computed: {
            value: {
                get() {
                    return this.form && this.form.values ? this.form.values[this.name] : this.default;
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


    input:not([type=checkbox]):not([type=file]):not([type=radio]), select, textarea {
        padding: .5em .6em;
        display: inline-block;
        border: 1px solid #ccc;
        box-shadow: inset 0 1px 3px #ddd;
        border-radius: 4px;
        vertical-align: middle;
        box-sizing: border-box;
        margin-bottom: 10px;

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

    input[type=checkbox]:focus, input[type=file]:focus, input[type=radio]:focus {
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


</style>
