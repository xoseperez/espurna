<template>
    <div>
        <div :class="['menu', {open}]">
            <span class="menu-link" @click="toggleMenu">
                <span></span>
            </span>
            <div class="inner">
                <slot name="header"></slot>
                <ul class="list">
                    <li v-for="(v, i) in tabs" :key="i" :class="{current: currentPanel === v.k}">
                        <a v-if="v.k !== 'separator'" href="#" @click="currentPanel = v.k">{{v.l}}</a>
                        <span v-else class="separator"></span>
                    </li>
                </ul>
                <slot name="footer"></slot>
            </div>
        </div>
        <div id="content">
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

    .menu-fixed {
        position: fixed;
        left: 0;
        top: 0;
        z-index: 3
    }

    .main-buttons, .footer, .separator {
        margin: 0;
        padding: 20px 10px;
        border-top: 1px solid #555;
    }

    .main-buttons {
        display: flex;
        flex-direction: column;
        align-items: stretch;
    }


    #content {
        padding: 0 2em;
        max-width: 1000px;
        margin: 0 0 0 180px;
        line-height: 1.6em;
        width: 100%;
        box-sizing: border-box;
    }

    .menu {
        box-sizing: border-box;
        /*margin-left: -160px;
        width: 160px;*/
        position: absolute;
        top: 0;
        width: 230px;
        padding-right: 44px;
        bottom: 0;
        z-index: 99; /* so the menu or its navicon stays above all content */
        overflow-y: auto;
        -webkit-overflow-scrolling: touch;

        .inner {
            background: #191818;
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
                background-color: #333
            }
        }

        .current {
            a, a:focus, a:hover {
                color: #fff;
                background: transparent;
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
            border-bottom: 4px solid #c76f19;
            padding: .5em .5em;
            font-size: 1.2em;
            color: #fff;
            margin: 0;
            background: #ff952f;
        }

        /*
        All anchors inside the menu should be styled like this.
        */

        .footer {
            font-size: .9em;

            a {
                color: #ff952f;
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

        .current {
            background: #479fd6;
        }

    }

    .menu,
    .menu-link, #content, .list a, .inner {
        transition: all .3s ease-out;
    }

    .menu-link {
        position: absolute;
        display: none; /* show this only on small screens */
        top: 0;
        right: 0;
        background: rgba(0, 0, 0, 0.7);
        font-size: 10px; /* change this value to increase/decrease button size */
        z-index: 10;
        width: 2em;
        height: auto;
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

    .content {
        padding-left: 2em;
        padding-right: 2em;
    }

    /* -- Responsive Styles (Media Queries) ------------------------------------- */

    /*
    Hides the menu at `48em`, but modify this based on your app's needs.
    */
    @media (max-width: 48em) {
        .menu {
            position: fixed;
            &:not(.open) {
                overflow: hidden;
                transform: translateX(calc(44px - 100%));

                + #content {
                    margin: 0;
                }

                .inner {
                    box-shadow: none;
                }
            }
            &.open + #content {
                margin: 0;
                transform: translateX(190px);
                &:before {
                    opacity: 1;
                    pointer-events: all;
                }
            }
            + #content:before {
                content: "";
                width: 150%;
                height: 100%;
                background: #0000003b;
                position: absolute;
                pointer-events: none;
                margin-left: -50%;
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