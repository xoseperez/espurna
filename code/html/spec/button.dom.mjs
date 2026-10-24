// shared DOM for button.spec.mjs & button-order.spec.mjs (not a spec by itself)
import { init as initRelay } from '../src/relay.mjs';
import { init as initButton } from '../src/button.mjs';

export const RELAY_SCHEMA = ['relayName', 'relayBtnGpio', 'relayTopicSub'];
export const BUTTON_SCHEMA = ['btnGpio', 'btnFreq', 'btnMinPulses', 'btnMinDuty', 'btnAcConfirm', 'btnDebDel', 'btnRelay'];

export function setupButtonDom() {
    document.body.innerHTML += `
    <form id="form-relay">
        <div id="relayConfig" class="settings-group"></div>
        <fieldset id="buttonOther" hidden>
            <div id="buttonConfig"></div>
        </fieldset>
        <fieldset id="buttonAcGlobal" hidden>
            <select name="btnAcFreq">
                <option value="0">Any</option>
                <option value="50">50 Hz</option>
                <option value="60">60 Hz</option>
            </select>
            <input type="checkbox" name="btnAcDbg">
        </fieldset>
    </form>
    <template id="template-relay-config">
        <div>
            <fieldset>
                <div class="pure-control-group">
                    <input name="relayName" type="text">
                </div>
                <div class="pure-control-group relay-btn-gpio">
                    <select name="relayBtnGpio">
                        <option value="2">2</option>
                        <option value="12">12</option>
                        <option value="13">13</option>
                        <option value="153">NONE</option>
                    </select>
                </div>
                <div class="pure-control-group relay-after">
                    <input name="relayTopicSub" type="text">
                </div>
            </fieldset>
        </div>
    </template>
    <template id="template-button-config">
        <div class="button-config">
            <select name="btnFreq" data-action="reboot">
                <option value="0">Button</option>
                <option value="1">AC mains switch</option>
            </select>
            <div class="button-ac-filter" hidden>
                <input name="btnMinPulses" type="number">
                <input name="btnMinDuty" type="number">
                <input name="btnAcConfirm" type="number">
                <input name="btnDebDel" type="number">
            </div>
        </div>
    </template>
    <template id="template-button-fallback">
        <div>
            <fieldset>
                <legend>Button #<span class="button-id"></span> (GPIO<span class="button-gpio"></span>)</legend>
            </fieldset>
        </div>
    </template>
    `;

    initRelay();
    initButton();
}
