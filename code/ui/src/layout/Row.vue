<template>
    <div :class="['row', {noWrap}]">
        <slot></slot>
    </div>
</template>

<script>
    export default {
        name: 'row',
        props: {
            noWrap: {
                type: Boolean,
                default: false
            }
        }
    }
</script>

<style lang="less">
    .row {
        display: flex;
        flex-direction: row;
        flex-wrap: wrap;
        position: relative;
    }

    .row.noWrap {
        flex-wrap: nowrap;
    }
</style>