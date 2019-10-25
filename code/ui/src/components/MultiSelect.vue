<template>
    <div class="multiselect">
        <div class="values">
            <span v-for="(v, i) in value" :key="v">{{find(v).l}}&nbsp;<i class="delete" @click="() => onDelete(i)">&times;</i></span>
        </div>
        <select v-bind="$attrs" @input="onSelect">
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
                    return this.options;

                let options = [...this.options];

                this.value.forEach((v) => {
                    options.splice(options.findIndex((el) => {
                        return el.k === v
                    }), 1);
                });
                return options;
            }
        },
        methods: {
            find(key) {
                return this.options.find((el) => {
                    return el.k === key
                });
            },
            onSelect(ev) {
                if (ev.target.value && this.find(ev.target.value)) {
                    this.value.push(ev.target.value);
                    this.$emit('input', this.value);
                    ev.target.value = ''
                }
            },
            onDelete(i) {
                this.$delete(this.value, i);
                this.$emit('input', this.value);
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
            padding: 5px;
            margin-right: 5px;
            font-size: .8em;
        }

        .delete {
            border-radius: 50%;
            background: desaturate(darken(@secondary, 20%), 50%);
            display: inline-block;
            width: 18px;
            height: 18px;
            padding: 1px;
            line-height: 16px;
            color: white;
            text-align: center;
        }
    }
</style>