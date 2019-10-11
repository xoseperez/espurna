<template>
    <form>
        <slot></slot>
    </form>
</template>

<script>
    export default {
        data() {
            return {
                values: {}
            }
        },
        watch: {
            values(newVal, oldVal) {
                console.log(newVal);
            }
        },
        provide() {
            return {
                $form: () => ({
                    values: this.values
                })
            }
        }
    }
</script>

<style>

</style>