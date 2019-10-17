<template>
    <section>
        <div class="header">
            <h1>INFLUXDB</h1>
            <h2>
                Configure the connection to your InfluxDB server. Leave the host field empty to disable InfluxDB
                connection.
            </h2>
        </div>

        <div class="page">
            <fieldset>
                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Enable InfluxDB</label>
                    <div class="pure-u-1 pure-u-lg-1-4">
                        <Inpt type="checkbox"
                              name="idbEnabled"
                              tabindex="40"/>
                    </div>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Host</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4" name="idbHost" type="text" tabindex="41"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Port</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4" name="idbPort" type="text" tabindex="42"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Database</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4" name="idbDatabase" type="text"
                          tabindex="43"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Username</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4"
                          name="idbUsername"
                          type="text"
                          tabindex="44"
                          autocomplete="off"/>
                </div>

                <div class="pure-g">
                    <label class="pure-u-1 pure-u-lg-1-4">Password</label>
                    <Inpt class="pure-u-1 pure-u-lg-3-4"
                          name="idbPassword"
                          type="password"
                          tabindex="45"
                          autocomplete="new-password"
                          spellcheck="false"/>
                    <span class="no-select password-reveal"></span>
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

<style lang="less">

</style>