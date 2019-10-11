<template>
    <input v-if="type !== 'select'" :type="type"/>
    <select v-else>
        <option v-for="(value, label) in options" :key="value" :value="value">{{label}}</option>
    </select>
</template>

<script>
    export default {
        props: {
            type: {
                type: String,
                default: "text"
            }
        }
    };
</script>

<style scoped></style>
