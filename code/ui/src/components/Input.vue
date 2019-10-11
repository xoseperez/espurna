<template>
    <select v-if="type === 'select'" :value="value" @input="onInput">
        <option v-for="(k, l) in options" :key="k" :value="k">{{l}}</option>
    </select>
    <input v-else :type="type" :value="value" @input="onInput">
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
            }
        },
        inject: {$form: {name: '$form', default: false}},
        computed: {
            value() {
                return this.form && this.form.values ? this.form.values[this.name] : null;
            },
            form() {
                return this.$form ? this.$form() : false;
            }
        },
        methods: {
            onInput(ev) {
                this.$set(this.form.values, this.name, ev.target.value);
            }
        },
    };
</script>

<style></style>
