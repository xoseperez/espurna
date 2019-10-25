<template>
    <div :class="['col', {stretch}, {noWrap}, {responsive}]" :style="style">
        <slot></slot>
    </div>
</template>

<script>
    export default {
        props: {
            size: {
                type: Number,
                default: 5
            },
            offset: {
                type: Number,
                default: 0
            },
            stretch: {
                type: Boolean,
                default: false
            },
            noWrap: {
                type: Boolean,
                default: false
            },
            responsive: {
                type: Boolean,
                default: false
            },
        },
        computed: {
            style() {
                let style = "";
                if (this.size !== 5)
                    style += 'width:' + this.size * 10 + '%;';
                if (this.offset)
                    style += 'margin-left:' + this.offset * 10 + '%;';

                return style;
            }
        }
    }
</script>

<style lang="less">
    .col {
        width: 50%;
        flex-grow: 1;
    }

    .form .row .col:nth-of-type(odd) {
        width: 25% !important;
    }

    .form .row .col:nth-of-type(even) {
        width: 75% !important;
    }

    .col.stretch, .col.noWrap {
        display: flex;
        flex-wrap: nowrap;
    }

    .col.noWrap > * {
        margin-right: 5px;
    }

    .col.stretch > * {
        width: 100%;
    }

    @media screen and (max-width: 48em){
        .responsive > .col, .col.responsive {
            width: 100% !important;
        }
    }
</style>