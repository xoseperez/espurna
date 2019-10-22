<template>
    <div id="password">
        <div class="content">
            <Form id="formPassword" ref="formPassword" class="pure-form" autocomplete="off">
                <div id="panel-password" class="panel block">
                    <div class="header">
                        <h1>SECURITY</h1>
                        <h2>
                            Before using this device you have to change the default password for the user <strong>admin</strong>.
                            This password will be used for the <strong>AP mode hotspot</strong>, the
                            <strong>web interface</strong> (where you are now) and the <strong>over-the-air updates</strong>.
                        </h2>
                    </div>

                    <div class="page">
                        <fieldset>
                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="adminPass1">New Password</label>
                                <Inpt id="adminPass1" class="pure-u-1 pure-u-lg-3-4"
                                      name="adminPass1"
                                      minlength="8"
                                      maxlength="63"
                                      type="password"
                                      tabindex="1"
                                      autocomplete="false"
                                      spellcheck="false"
                                      required/>
                            </div>

                            <div class="pure-g">
                                <label class="pure-u-1 pure-u-lg-1-4" for="adminPass2">Repeat password</label>
                                <Inpt id="adminPass2" class="pure-u-1 pure-u-lg-3-4"
                                      name="adminPass2"
                                      minlength="8"
                                      maxlength="63"
                                      type="password"
                                      tabindex="2"
                                      autocomplete="false"
                                      spellcheck="false"
                                      required/>
                            </div>
                        </fieldset>

                        <div class="pure-g">
                            <Hint>
                                Password must be <strong>8..63 characters</strong> (numbers and letters and any of
                                these special characters: _,.;:~!?@#$%^&amp;*&lt;&gt;\|(){}[]) and have at least
                                <strong>one lowercase</strong> and <strong>one uppercase</strong> or
                                <strong>one number</strong>.
                            </Hint>
                        </div>


                        <div class="pure-g">
                            <Btn name="generate-pwd" color="secondary" class="pure-u-11-24 pure-u-lg-1-4"
                                 title="Generate password based on password policy">
                                Generate
                            </Btn>
                            <div class="pure-u-2-24 pure-u-lg-1-2"></div>
                            <Btn name="update-pwd" class="pure-u-11-24 pure-u-lg-1-4"
                                 title="Save new password">
                                Save
                            </Btn>
                        </div>
                    </div>
                </div>
            </Form>
        </div>
    </div>
</template>

<script>
    import Btn from "../../components/Button";
    import Inpt from "../../components/Input";
    import Hint from "../../components/Hint";

    export default {
        components: {Hint, Btn, Inpt}
    }
</script>

<style lang="less">

</style>