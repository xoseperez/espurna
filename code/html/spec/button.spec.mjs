import { beforeAll, expect, test } from 'vitest';
import { BUTTON_SCHEMA, RELAY_SCHEMA, setupButtonDom } from './button.dom.mjs';
import { updateKeyValue } from '../src/settings.mjs';
import { getElementName } from '../src/settings/name.mjs';
import { checkAndSetElementChanged } from '../src/settings/change.mjs';
import { makeDataRequest } from '../src/settings/dataset.mjs';
import { groupCleanupElements } from '../src/settings/group.elem.mjs';

beforeAll(() => {
    setupButtonDom();
});

/** @param {string} key */
function byKey(key) {
    return /** @type {HTMLInputElement | HTMLSelectElement} */(
        document.querySelector(`[data-key="${key}"]`));
}

/**
 * @param {HTMLSelectElement} select
 * @param {string} value
 */
function choose(select, value) {
    select.value = value;
    select.dispatchEvent(new Event('change'));
    checkAndSetElementChanged(select);
}

/** @param {string} id */
function isHidden(id) {
    return /** @type {HTMLElement} */(document.getElementById(id)).hidden;
}

test('btnConfig before relayConfig renders into fallback, then moves into relay cards', () => {
    updateKeyValue('btnAcFreq', 50);
    updateKeyValue('btnConfig', {
        schema: BUTTON_SCHEMA,
        values: [
            ['13', 'true', '5', '33', '2', '100', '0'],
            ['12', 'true', '7', '33', '3', '100', '1'],
            ['2', 'false', '5', '33', '2', '50', '0'],
            ['4', 'false', '5', '0', '2', '50', '255'],
        ],
    });

    // no relay cards yet, everything is in the fallback block
    expect(document.querySelectorAll('#buttonConfig select[name="btnFreq"]')).toHaveLength(4);
    expect(isHidden('buttonOther')).toBe(false);

    updateKeyValue('relayConfig', {
        schema: RELAY_SCHEMA,
        values: [
            ['Left', '13', 'a'],
            ['Right', '12', 'b'],
        ],
    });

    const cards = /** @type {HTMLCollection} */(document.getElementById('relayConfig')?.children);
    expect(cards).toHaveLength(2);

    // relay #0 is controlled by buttons #0 and #2, button #0 matches the relay 'Button GPIO'
    const card0 = /** @type {HTMLElement} */(cards[0]);
    const card1 = /** @type {HTMLElement} */(cards[1]);
    const freq0 = /** @type {HTMLSelectElement} */(card0.querySelector('select[name="btnFreq"]'));
    const freq1 = /** @type {HTMLSelectElement} */(card1.querySelector('select[name="btnFreq"]'));
    expect(getElementName(freq0)).toEqual('btnFreq0');
    expect(getElementName(freq1)).toEqual('btnFreq1');
    expect(freq0.value).toEqual('1');
    expect(card0.querySelectorAll('select[name="btnFreq"]')).toHaveLength(1);

    // right below 'Button GPIO'
    const btnGpioGroup = card0.querySelector('.relay-btn-gpio');
    expect(btnGpioGroup?.nextElementSibling?.classList.contains('button-config')).toBe(true);

    for (const name of ['btnMinPulses', 'btnMinDuty', 'btnAcConfirm', 'btnDebDel']) {
        const elem = /** @type {HTMLInputElement} */(card1.querySelector(`input[name="${name}"]`));
        expect(getElementName(elem)).toEqual(`${name}1`);
    }
    expect(/** @type {HTMLInputElement} */(byKey('btnMinPulses1')).valueAsNumber).toEqual(7);
    expect(/** @type {HTMLInputElement} */(byKey('btnAcConfirm1')).valueAsNumber).toEqual(3);

    // buttons #2 (second one for relay #0) and #3 (no relay) stay in the fallback block
    const fallback = Array.from(document.querySelectorAll('#buttonConfig [data-key^="btnFreq"]'))
        .map((x) => getElementName(/** @type {HTMLSelectElement} */(x)));
    expect(fallback).toEqual(['btnFreq2', 'btnFreq3']);
    const legends = Array.from(document.querySelectorAll('#buttonConfig legend'))
        .map((x) => x.textContent);
    expect(legends).toEqual(['Button #2 (GPIO2)', 'Button #3 (GPIO4)']);
    expect(isHidden('buttonOther')).toBe(false);

    // filter group follows the type
    const filter0 = /** @type {HTMLElement} */(card0.querySelector('.button-ac-filter'));
    const filter2 = /** @type {HTMLElement} */
        (byKey('btnFreq2').closest('.button-config')?.querySelector('.button-ac-filter'));
    expect(filter0.hidden).toBe(false);
    expect(filter2.hidden).toBe(true);

    // global block is visible while at least one input is an AC switch
    expect(isHidden('buttonAcGlobal')).toBe(false);
    expect(/** @type {HTMLSelectElement} */
        (document.querySelector('select[name="btnAcFreq"]')).value).toEqual('50');

    choose(freq0, '0');
    expect(filter0.hidden).toBe(true);
    expect(isHidden('buttonAcGlobal')).toBe(false);

    choose(freq1, '0');
    expect(isHidden('buttonAcGlobal')).toBe(true);

    choose(/** @type {HTMLSelectElement} */(byKey('btnFreq2')), '1');
    expect(filter2.hidden).toBe(false);
    expect(isHidden('buttonAcGlobal')).toBe(false);

    // saved with the button index, not the relay index nor the card position
    const form = /** @type {HTMLFormElement} */(document.getElementById('form-relay'));
    const request = makeDataRequest([form]);
    expect(request.set).toEqual({btnFreq0: 0, btnFreq1: 0, btnFreq2: 1});

    // deleting a relay card must only cleanup relay keys
    const cleanup = groupCleanupElements(card0).map(getElementName);
    expect(cleanup).toEqual(['relayName0', 'relayBtnGpio0', 'relayTopicSub0']);
});
