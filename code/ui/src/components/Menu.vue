<template>
    <div>
        <div :class="['menu', {open}]">
            <span class="menu-link" @click="toggleMenu">
                <span></span>
            </span>
            <div class="inner">
                <slot name="header"></slot>
                <ul class="list">
                    <!-- eslint-disable vue/no-use-v-if-with-v-for || because this is one of the only valid use case -->
                    <li v-for="(v, i) in tabs" v-if="v.k === 'separator' || $scopedSlots[v.k]" :key="i"
                        :class="{current: currentPanel === v.k}">
                        <!-- eslint-enable -->
                        <a v-if="v.k !== 'separator'" href="#" @click="currentPanel = v.k">{{v.l}}</a>
                        <span v-else class="separator"></span>
                    </li>
                </ul>
                <slot name="footer"></slot>
            </div>
        </div>
        <div class="content">
            <slot :id="'panel-'+currentPanel" :name="currentPanel"></slot>
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
                currentPanel: this.tabs[0].k,
                open: false
            }
        },
        methods: {
            toggleMenu() {
                this.open = !this.open;
            }
        }
    }
</script>

<style lang="less">
    @import "../assets/Colors";

    @menu-width: 190px;
    @menu-toggle-size: 44px;

    @padded-menu-width: (@menu-width + @menu-toggle-size);


    /*
    This is the parent `<div>` that contains the menu and the content area.
    */
    #layout {
        position: relative;
        min-height: 100%;
        display: flex;
    }

    .main-buttons, .footer, .separator {
        margin: 0;
        padding: 20px 12px;
        border-top: 1px solid #555;
    }

    .main-buttons {
        display: flex;
        flex-direction: column;
        align-items: stretch;
    }


    .content {
        padding: 0 40px 40px;
        max-width: 1400px;
        margin: 0 0 0 -10px;
        line-height: 1.6em;
        width: 100%;
        overflow: hidden;
    }

    .menu {
        width: @padded-menu-width;
        padding-right: 10px; //For the shadow to be visible
        z-index: 99; /* so the menu or its navicon stays above all content */
        overflow-y: auto;
        -webkit-overflow-scrolling: touch;

        .inner {
            background: #222;
            height: 100%;
            box-shadow: 1px 0 6px rgba(0, 0, 0, .5);
        }

        .list a {
            border: 0;
            color: #999;
            padding: .6em 14px .6em .8em;
            display: block;
            text-decoration: none;
            white-space: nowrap;
            background: transparent;
            text-transform: uppercase;
            text-align: right;

            &:focus, &:hover {
                background-color: lighten(@secondary, 10%);
                color: #fff;
            }
        }

        .current {
            a, a:focus, a:hover {
                color: #fff;
                background: @secondary;
                margin-right: -4px;
                border-radius: 0 3px 3px 0;
                box-shadow: 1px 0 6px rgba(0, 0, 0, .5);
                position: relative;
                z-index: 1;
            }
        }

        li, ul {
            position: relative
        }

        ul {
            list-style: none;
            margin: 10px 0;
            padding: 0;
            background: transparent;
        }

        li {
            padding: 0;
            margin: 0;
            display: block;
        }

        .heading {
            display: block;
            text-decoration: none;
            border-bottom: 4px solid darken(@primary, 15%);
            padding: .5em .5em;
            font-size: 1.2em;
            color: #fff;
            margin: 0 -4px 0 0;
            border-radius: 0 0 10px;
            background: @primary;
        }

        /*
        All anchors inside the menu should be styled like this.
        */

        .footer {
            font-size: .9em;
            padding-bottom: 30px;

            a {
                color: @primary;
            }
        }

        /*
        Add that light border to separate items into groups.
        */

        .separator {
            display: block;
            margin: 6px 0;
            padding: 0;
        }

    }

    .menu,
    .menu-link, .content, .list a, .inner {
        transition: all .3s ease-out;
    }

    .menu-link {
        position: absolute;
        display: none; /* show this only on small screens */
        top: 0;
        right: 0;
        background: rgba(0, 0, 0, 0.7);
        font-size: 10px; /* change this value to increase/decrease button size */
        z-index: -1;
        width: @menu-toggle-size;
        height: @menu-toggle-size;
        padding: 2.1em 1.2em;

        &:hover,
        &:focus {
            background: #000;
        }

        span,
        span:before,
        span:after {
            position: relative;
            display: block;
            background-color: #fff;
            width: 100%;
            height: 0.2em;
        }

        span:before,
        span:after {
            position: absolute;
            margin-top: -0.6em;
            content: " ";
        }

        span:after {
            margin-top: 0.6em;
        }
    }

    /* -- Responsive Styles (Media Queries) ------------------------------------- */

    /*
    Hides the menu at `48em`, but modify this based on your app's needs.
    */
    @media (max-width: 48em) {
        .content {
            margin: 0;
        }

        .content > section {
            padding: 0 10px;
        }

        .menu {
            position: fixed;
            height: 100%;
            padding-right: @menu-toggle-size;

            &:not(.open) {
                overflow: hidden;
                transform: translateX(-@menu-width);

                .heading {
                    margin: 0;
                }

                + .content {
                    padding: 0;
                }

                .current {
                    a, a:focus, a:hover {
                        margin: 0;
                        box-shadow: none;
                    }
                }

                .inner {
                    box-shadow: none;
                }
            }

            &.open + .content {
                padding: 0;
                transform: translateX(@menu-width);

                &:before {
                    opacity: 1;
                    pointer-events: all;
                }
            }

            + .content:before {
                content: "";
                z-index: 9;
                width: 100%;
                height: 100%;
                background: #0000003b;
                position: absolute;
                pointer-events: none;
                margin-left: 0;
                opacity: 0;
                transition: opacity .3s ease-out;
            }
        }

        .menu-link {
            /*left: 160px;*/
            display: block;
        }
    }

</style>