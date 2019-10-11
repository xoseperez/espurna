<template>
    <section>
        <div class="header">
            <h1>HOME ASSISTANT</h1>
            <h2>
                Add this device to your Home Assistant.
            </h2>
        </div>

        <div class="page">
            <fieldset>
                <legend>Discover</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Discover</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="haEnabled"
                              tabindex="14"/>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-2"></div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        Home Assistant auto-discovery feature. Enable and save to add the device to your HA console.
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Prefix</label>
                    <Inpt class="pure-u-1 pure-u-lg-1-4" name="haPrefix" type="text" tabindex="15"/>
                </div>

                <legend>Configuration</legend>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Configuration</label>
                    <div class="pure-u-1-4 pure-u-lg-3-4">
                        <button type="button" class="pure-button button-ha-config pure-u-1-3">
                            Show
                        </button>
                    </div>
                    <div class="pure-u-0 pure-u-lg-1-4"></div>
                    <div class="pure-u-1 pure-u-lg-3-4 hint">
                        These are the settings you should copy to your Home Assistant "configuration.yaml" file. If any
                        of the sections below (switch, light, sensor) already exists, do not duplicate it, simply copy
                        the contents of the section below the ones already present.
                    </div>
                </div>
                <div class="pure-g">
                    <textarea id="haConfig"
                              class="pure-u-1 terminal"
                              name="haConfig"
                              wrap="soft"
                              readonly></textarea>
                </div>
            </fieldset>
        </div>
    </section>
</template>

<script>
    import Inpt from './../../components/Input';

    export default {
        components: {
            Inpt
        }
    }
</script>

<style scoped>

</style>