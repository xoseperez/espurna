<template>
    <div v-if="value" class="group"><slot></slot></div>
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
        }
    };
</script>
