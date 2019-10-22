<template>
    <section>
        <div class="header">
            <h1>NoFUSS</h1>
            <h2>
                Automatically upgrade the firmware (see <a href="https://bitbucket.org/xoseperez/nofuss">xoseperez/nofuss</a>
                for details)
            </h2>
        </div>

        <div class="page">
            <fieldset>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Enable</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="switch" name="nofussEnabled"/>
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Server</label>
                    <Inpt name="nofussServer" class="pure-u-1 pure-u-lg-3-4" type="text" tabindex="15"/>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <Hint>Address of the NoFUSS server</Hint>
                </div>
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';
    import Hint from "../../components/Hint";

    export default {
        components: {
            Hint,
            Inpt
        }
    }
</script>

<style lang="less">

</style>