<template>
    <div :class="['col', {stretch}]">
        <slot></slot>
    </div>
</template>

<script>
    export default {
        props: {
            /*size: {
                type: Number,
                default: 6
            },
            offset: {
                type: Number,
                default: 0
            },*/
            stretch: {
                type: Boolean,
                default: false
            },
        }
    }
</script>

<style lang="less">
    .row .col:nth-of-type(2) {
        width: 150%;
    }

    .row .col {
        width: 50%;
    }

    .col.stretch {
        display: flex;
        > * {
            width: 100%;
        }
    }
</style>