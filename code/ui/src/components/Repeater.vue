<template>
    <div>
        <transition-group type="transition" name="flip-list">
            <RepeaterRow v-for="(row, i) in values"
                         :key="row.key"
                         :value="row.value"
                         :row="row.key"
                         @input="(val) => $emit('input', [...values].splice(i,1,val))">
                <slot name="default" :value="row.value" :row="row" :remove="() => onRemove(row.key)"></slot>
                <template #append>
                    <slot name="btnRemove" :click="() => onRemove(row.key)" :row="row">
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