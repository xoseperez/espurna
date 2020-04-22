<template>
    <form ref="form">
        <slot></slot>
    </form>
</template>

<script>
    export default {
        props: {
            value: {
                type: Object,
                default: () => ({})
            }
        },
        data() {
            return {
                values: this.value
            };
        },
        watch: {
            values: {
                deep: true,
                handler(newVal) {
                    this.$emit("input", newVal);
                }
            }
        },
        provide() {
            return {
                $form: () => ({
                    values: this.values
                })
            };
        },
        methods: {
            reportValidity() {
                return this.$refs.form.reportValidity();
            }
        }
    };
</script>

<style lang="less">

    form {
        label {
            margin: .5em 0 .2em
        }

        fieldset {
            margin: 0;
            padding: .35em 0 .75em;
            border: 0
        }

        legend {
            display: block;
            width: 100%;
            padding: .3em 0;
            color: #333;
            border-bottom: 1px solid #e5e5e5;
            font-weight: bold;
            letter-spacing: 0;
            margin: 10px 0 1em 0;
        }

    }
</style>
