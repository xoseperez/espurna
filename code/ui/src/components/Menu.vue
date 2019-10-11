<template>
    <div>
        <div class="menu">
            <div class="pure-menu">
                <slot #header></slot>
                <ul class="pure-menu-list">
                    <li v-for="{k, l} in tabs" :key="k" class="pure-menu-item">
                        <a href="#" class="pure-menu-link" @click="currentPanel = k">{{l}}</a>
                    </li>
                </ul>
                <slot #footer></slot>
            </div>
        </div>
        <div id="content">
            <slot :id="'panel-'+currentPanel" class="panel block" :name="currentPanel"></slot>
        </div>
    </div>
</template>

<script>
    export default {
        props: {
            tabs: {
                type: [Array],
                required: true
            }
        },
        data() {
            return {
                currentPanel: 0
            }
        }

    }
</script>

<style scoped>

</style>