<template>
    <div>
        <transition-group type="transition" name="flip-list">
            <RepeaterRow v-for="(row, i) in values"
                         :key="row.key"
                         :value="row.value"
                         :row="row.key"
                         @input="(val) => onInput(i, val)">
                <template #default="props">
                    <slot name="default" :value="row.value" :row="row"></slot>
                </template>
                <template #append="props">
                    <slot name="btnRemove" :click="() => onRemove(row.key)">
                        <Btn color="danger" @click="() => onRemove(row.key)">
                            Remove
                        </Btn>
                    </slot>
                </template>
            </RepeaterRow>
        </transition-group>
        <slot name="btnAdd" :click="onAdd">
            <Btn @click="onAdd">
                Add
            </Btn>
        </slot>
    </div>
</template>

<script>
    import Btn from './Button'
    import RepeaterRow from "./RepeaterRow";


    const filterMap = function (arr, callback) {
        let out = [];
        for (let key in arr) {
            if (arr.hasOwnProperty(key)) {
                let v = callback(arr[key], key);
                if (v !== undefined)
                    out.push(v);
            }
        }
        return out;
    };

    export default {
        components: {
            RepeaterRow,
            Btn
        },
        props: {
            max: {
                type: Number,
                default: 8
            },
            name: {
                type: String
            }
        },
        data() {
            return {
                key: 0
            }
        },
        computed: {
            form() {
                return this.$form ? this.$form() : false;
            },
            canAdd() {
                return (!this.max || this.values.length < this.max)
            },
            values: {
                get() {
                    return this.form.values[this.name];
                },
                set(v) {
                    this.$set(this.form.values, this.name, v);
                }
            }
        },
        mounted() {
            if (!this.values)
                this.values = [];
        },
        inject: {$form: {name: '$form', default: false}},
        methods: {
            onAdd(val) {
                if (this.canAdd) {
                    this.values.push({
                        key: this.key++,
                        value: typeof val !== 'undefined' ? val : {}
                    });
                }
            },
            onRemove(key) {
                this.values = filterMap(this.values, (val) => {
                    if (val.key === key) {
                        return undefined;
                    }
                    return val;
                });
            }
        },

    }
</script>

<style lang="less">

</style>