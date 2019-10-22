<template>
    <div class="row">
        <slot></slot>
    </div>
</template>

<script>
    export default {
        name: 'row'
    }
</script>

<style lang="less">
    .row {
        display: flex;
        flex-direction: row;
        flex-wrap: nowrap;
        position: relative;
    }
</style>