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
        name: "repeater-row",
        props: {
            value: {
                type: Object,
                default: () => ({})
            },
        },
        provide() {
            return {
                $form: () => ({
                    values: this.value
                })
            }
        }
    }
</script>

<style lang="less">

</style>