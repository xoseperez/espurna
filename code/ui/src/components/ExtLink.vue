<template>
    <a rel="noopener" target="_blank"><strong v-if="strong"><slot></slot></strong><slot v-else></slot></a>
</template>

<script>
    export default {
        props: {
            strong: Boolean
        }
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
