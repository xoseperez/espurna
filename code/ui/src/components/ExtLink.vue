<template>
    <a rel="noopener" target="_blank"><slot></slot></a>
</template>

<script>
    export default {
    };
</script>

<style>
    a {
        background-color: transparent;
        padding: 0;
        text-decoration: none;
    }

    a:active, a:hover {
        outline: 0
    }
</style>