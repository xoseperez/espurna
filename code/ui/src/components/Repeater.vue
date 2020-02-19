<template>
    <div>
        <!--transition-group name="flip-list" tag="div"-->
        <RepeaterRow v-for="(row, i) in values"
                     :key="row.key"
                     :value="row.value"
                     @input="(v) => onInput(v, i)">
            <slot name="default" :value="row.value"
                  :k="i"
                  :row="row"
                  :remove="() => onRemove(row.key)"></slot>
            <template v-if="!locked" #append>
                <slot name="btnRemove" :click="() => onRemove(row.key)"
                      :row="row"
                      :value="row.value"
                      :k="i">
                    <Btn color="danger" @click="() => onRemove(row.key)">
                        Remove
                    </Btn>
                </slot>
            </template>
        </RepeaterRow>
        <!--/transition-group-->
        <slot v-if="!locked && canAdd" name="btnAdd" :click="onAdd">
            <Btn @click="onAdd">
                Add
            </Btn>
        </slot>
    </div>
</template>

<script>
    import Btn from "./Button";
    import RepeaterRow from "./RepeaterRow";


    const filterMap = function (arr, callback) {
        let out = [];
        for (let key in arr) {
            if (arr.hasOwnProperty(key)) {
                let v = callback(arr[key], key);
                if (v !== undefined) {
                    out.push(v);
                }
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
                type: String,
                required: false
            },
            locked: {
                type: Boolean,
                default: false
            },
            value: {
                type: Array,
                required: false
            },
        },
        data() {
            return {
                key: 0,
                values: []
            };
        },
        computed: {
            form() {
                return this.$form ? this.$form() : false;
            },
            canAdd() {
                return !this.locked && (!this.max || this.values.length < this.max);
            },
        },
        mounted() {
            this.setValues();
        },
        inject: {$form: {name: "$form", default: false}},
        methods: {
            setValues() {
                let values = [];

                if (this.value) {
                    this.value.forEach((v) => {
                        let row = {key: this.key++, value: v};
                        this.$emit("created", {row});
                        values.push(row);
                    });
                } else if (this.name && this.form.values[this.name]) {
                    this.form.values[this.name].forEach((v) => {
                        let row = {key: this.key++, value: v};
                        this.$emit("created", {row});
                        values.push(row);
                    });
                }
                this.values = values;
            },
            getValues() {
                return this.values.map((v) => {
                    return v.value;
                });
            },
            onAdd(val) {
                val = val instanceof Event ? undefined : val; // If called from a click handler, ignore the property
                if (this.canAdd) {
                    let row = {
                        key: this.key++,
                        value: val !== undefined ? val : {}
                    };
                    this.$emit("created", {row});
                    this.values.push(row);
                }
            },
            onRemove(key) {
                if (!this.locked) {
                    this.values = filterMap(this.values, (val) => {
                        if (val.key === key) {
                            return undefined;
                        }
                        return val;
                    });
                }
            },
            onInput(val, i) {
                this.$set(this.values[i], "value", val);
                this.$emit("input", this.getValues());
            }
        },

    };
</script>

<style lang="less">
    .append-wrapper {
        text-align: right;
    }
</style>
