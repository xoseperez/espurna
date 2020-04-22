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
            outset: {
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
                    {style += "width:" + this.size * 10 + "%;";}
                if (this.offset)
                    {style += "margin-left:" + this.offset * 10 + "%;";}
                if (this.outset)
                    {style += "margin-right:" + this.outset * 10 + "%;";}

                return style;
            }
        }
    };
</script>

<style lang="less">
    .col {
        width: 50%;
        flex-grow: 1;
        padding: 0 5px;
    }

    .form .row .col:nth-of-type(odd) {
        width: 25%;
        text-align: right;
    }

    .form .row .col:nth-of-type(even) {
        width: 75%;
    }

    .col.stretch, .col.noWrap {
        display: flex;
        flex-wrap: nowrap;
    }

    .col.noWrap > *:not(.btn) {
        margin-right: 5px;
    }
    .col.noWrap > .btn {
        margin-left: 0;
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