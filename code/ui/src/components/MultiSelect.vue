<template>
    <div class="multiselect">
        <div class="values">
            <span v-for="(v, i) in value" :key="v">{{find(v).l}}<i class="delete" @click="() => onDelete(i)">&times;</i></span>
        </div>
        <select v-if="_options" v-bind="$attrs" @input="onSelect">
            <option value="" disabled selected>{{placeholder}}</option>
            <option v-for="{l, k} in _options" :key="k" :value="k">{{l}}</option>
        </select>
    </div>
</template>

<script>
    export default {
        inheritAttrs: false,
        props: {
            options: {
                type: Array
            },
            value: {
                type: Array
            },
            placeholder: {
                type: String,
                default: "Select an option"
            }
        },
        data() {
            return {
                val: "",
            }
        },
        computed: {
            _options() {
                if (this.value.length === this.options.length)
                    return null;

                let options = [...this.options];

                this.value.forEach((v) => {
                    v = v.toString();
                    options.splice(options.findIndex((el) => {
                        return el.k.toString() === v
                    }), 1);
                });
                return options;
            }
        },
        methods: {
            find(key) {
                key = key.toString();
                return this.options.find((el) => {
                    return el.k.toString() === key
                });
            },
            onSelect(ev) {
                if (ev.target.value && this.find(ev.target.value)) {
                    let value = [...this.value];
                    value.push(ev.target.value);
                    this.$emit('input', value);
                    ev.target.value = ''
                }
            },
            onDelete(i) {
                let value = [...this.value];
                value.splice(i, 1);
                this.$emit('input', value);
            }
        }
    }
</script>

<style lang="less">
    @import "../assets/Colors";

    .multiselect {
        .values span {
            background: desaturate(lighten(@secondary, 20%), 50%);
            border-radius: 4px;
            padding: 5px 5px 5px 10px;
            margin-right: 5px;
            font-size: .8em;
            color: white;
        }

        .delete {
            border-radius: 50%;
            background: desaturate(darken(@secondary, 10%), 50%);
            display: inline-block;
            width: 18px;
            height: 18px;
            padding: 1px;
            line-height: 16px;
            color: white;
            text-align: center;
            cursor: pointer;
            margin-left: 5px;
        }
    }
</style>