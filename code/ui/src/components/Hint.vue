<template>
    <div class="hint">
        <span class="tip">?</span>
        <span class="modal"><slot></slot></span>
    </div>
</template>

<script>
    export default {
    };
</script>

<style lang="less">
    .hint {
        .tip {
            text-align: center;
            display: block;
            width: 2em;
            height: 2em;
            font-weight: bold;
            color: white;
            font-family: sans-serif;
            font-size: 1.2em;
            line-height: 2em;
            background: #479fd6;
            border: 2px solid white;
            border-radius: 50%;
            position: absolute;
            left: 20px;
        }

        .modal {
            display: none;
        }

        &:hover, &:focus, &:active {
            .modal {
                display: block;
            }
        }
    }
</style>