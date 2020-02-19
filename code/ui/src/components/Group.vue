<template>
    <div class="group"><slot></slot></div>
</template>

<script>
    export default {
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
            };
        }
    };
</script>