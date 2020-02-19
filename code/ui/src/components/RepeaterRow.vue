<template>
    <div class="repeater-row">
        <div class="repeater-row-inner">
            <slot name="default"></slot>
        </div>
        <div class="append-wrapper">
            <slot name="append"></slot>
        </div>
    </div>
</template>

<script>
    export default {
        props: {
            value: Object,
        },
        provide() {
            return {
                $form: () => ({
                    values: this.value
                })
            };
        },
        watch: {
            value: {
                deep: true,
                handler() {
                    this.$emit("input", this.value);
                }
            }
        }
    };
</script>

<style lang="less">

</style>
