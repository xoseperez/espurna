<template>
    <div class="col">
        <slot></slot>
    </div>
</template>

<script>
    export default {
        props: {
            size: {
                type: Number,
                default: 6
            },
            offset: {
                type: Number,
                default: 0
            }
        }
    }
</script>

<style lang="less">
    .col {
        flex: 1 0;
    }
</style>