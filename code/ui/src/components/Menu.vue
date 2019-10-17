<template>
    <div>
        <div class="menu">
            <a class="menu-link">
                <span></span>
            </a>
            <slot name="header"></slot>
            <ul class="list">
                <li v-for="(v, i) in tabs" :key="i" :class="{current: currentPanel === v.k}">
                    <a v-if="v.k !== 'separator'" href="#" @click="currentPanel = v.k">{{v.l}}</a>
                    <span v-else class="separator"></span>
                </li>
            </ul>
            <slot name="footer"></slot>
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
                currentPanel: this.tabs[0].k
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

    .menu {
        box-sizing: border-box;
        margin-left: -160px; /* ".menu" width */
        width: 160px;
        position: fixed;
        top: 0;
        left: 0;
        bottom: 0;
        z-index: 1000; /* so the menu or its navicon stays above all content */
        background: #191818;
        overflow-y: auto;
        -webkit-overflow-scrolling: touch;

        .list a {
            border: 0;
            color: #999;
            padding: .6em 0 .6em .6em;
            display: block;
            text-decoration: none;
            white-space: nowrap;
            background: transparent;

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
            margin: 0;
            padding: 0;
            background: transparent;
        }

        li {
            padding: 0;
            margin: 0;
            height: 100%;
            display: block;
        }

        .heading {
            display: block;
            text-decoration: none;
            border-bottom: 1px solid #333;
            padding: .5em .5em;
            white-space: normal;
            text-transform: initial;
            font-size: 110%;
            color: #fff;
            margin: 0;
        }

        .footer {
            border-top: 1px solid #333;
        }

        /*
        All anchors inside the menu should be styled like this.
        */

        .footer a {
            color: #999;
            border: none;
            padding: 0.6em 0 0.6em 0.6em;
        }

        /*
        Add that light border to separate items into groups.
        */

        .separator {
            display: block;
            width: 100%;
            height: 1px;
            background: #333;
        }

        .current,
        .heading {
            background: #479fd6;
        }

    }

    .menu-link {
        position: fixed;
        display: block; /* show this only on small screens */
        top: 0;
        left: 0; /* ".menu width" */
        background: rgba(0, 0, 0, 0.7);
        font-size: 10px; /* change this value to increase/decrease button size */
        z-index: 10;
        width: 2em;
        height: auto;
        padding: 2.1em 1.6em;

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
    @media (min-width: 48em) {
        .header,
        .content {
            padding-left: 2em;
            padding-right: 2em;
        }

        .menu {
            left: 160px;
        }

        .menu .menu-link {
            position: fixed;
            left: 160px;
            display: none;
        }
    }

</style>