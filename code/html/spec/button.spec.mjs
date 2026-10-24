import { beforeAll, expect, test } from 'vitest';
import { init } from '../src/button.mjs';
import { updateKeyValue } from '../src/settings.mjs';
import { getElementName } from '../src/settings/name.mjs';

import panelRelay from '../src/panel-relay.html?raw';
import templates from '../src/template-relay.html?raw';

// Real markup, so that the spec also covers which fields require a reboot
beforeAll(() => {
    document.body.innerHTML += templates;

    const panel = document.createElement('div');
    panel.innerHTML = panelRelay;

    const fieldset = panel.querySelector('#buttonConfig')?.closest('fieldset');
    expect(fieldset).toBeTruthy();
    fieldset?.setAttribute('id', 'button-fieldset');

    const form = document.createElement('form');
    form.id = 'form-button';
    form.appendChild(/** @type {HTMLElement} */(fieldset));
    document.body.appendChild(form);

    init();
});

/**
 * @param {string} name
 * @returns {HTMLInputElement[]}
 */
function inputs(name) {
    return Array.from(document.querySelectorAll(`input[name="${name}"]`));
}

/** @returns {HTMLElement[]} */
function filters() {
    return Array.from(document.querySelectorAll('#buttonConfig .button-ac-filter'));
}

/** @returns {HTMLElement} */
function global() {
    return /** @type {HTMLElement} */(document.getElementById('buttonAcGlobal'));
}

test('button panel is created from btnConfig', () => {
    updateKeyValue('btnAcFreq', 50);
    updateKeyValue('btnAcDbg', false);
    updateKeyValue('btnConfig', {
        schema: ['btnGpio', 'btnFreq', 'btnMinPulses', 'btnMinDuty', 'btnAcConfirm', 'btnDebDel'],
        values: [
            ['13', 'true', '5', '30', '3', '100'],
            ['12', 'false', '7', '0', '2', '50'],
        ],
    });

    const fieldset = document.getElementById('button-fieldset');
    expect(fieldset?.hasAttribute('hidden')).toBe(false);

    const freq = inputs('btnFreq');
    expect(freq).toHaveLength(2);
    expect(freq[0].checked).toBe(true);
    expect(freq[1].checked).toBe(false);
    expect(getElementName(freq[0])).toEqual('btnFreq0');
    expect(getElementName(freq[1])).toEqual('btnFreq1');

    const pulses = inputs('btnMinPulses');
    expect(pulses[1].valueAsNumber).toEqual(7);
    expect(getElementName(pulses[1])).toEqual('btnMinPulses1');

    const duty = inputs('btnMinDuty');
    expect(duty[0].valueAsNumber).toEqual(30);
    expect(getElementName(duty[0])).toEqual('btnMinDuty0');

    const confirm = inputs('btnAcConfirm');
    expect(confirm[0].valueAsNumber).toEqual(3);
    expect(confirm[1].valueAsNumber).toEqual(2);
    expect(getElementName(confirm[0])).toEqual('btnAcConfirm0');

    const window = inputs('btnDebDel');
    expect(window[1].valueAsNumber).toEqual(50);
    expect(getElementName(window[1])).toEqual('btnDebDel1');

    const select = /** @type {HTMLSelectElement} */
        (document.querySelector('select[name="btnAcFreq"]'));
    expect(select.value).toEqual('50');

    const legends = Array.from(document.querySelectorAll('#buttonConfig legend'))
        .map((x) => x.textContent);
    expect(legends).toEqual(['Button #0 (GPIO13)', 'Button #1 (GPIO12)']);
});

test('only btnFreq requires reboot', () => {
    for (const name of ['btnMinPulses', 'btnMinDuty', 'btnAcConfirm', 'btnDebDel']) {
        for (const input of inputs(name)) {
            expect(input.dataset['action']).toBeUndefined();
        }
    }

    for (const input of inputs('btnFreq')) {
        expect(input.dataset['action']).toEqual('reboot');
    }

    for (const name of ['btnAcFreq', 'btnAcDbg']) {
        const elem = /** @type {HTMLElement} */
            (document.querySelector(`[name="${name}"]`));
        expect(elem.dataset['action']).toBeUndefined();
    }
});

test('AC filter is only shown for AC switches', () => {
    const [first, second] = filters();
    expect(first.hidden).toBe(false);
    expect(second.hidden).toBe(true);
    expect(global().hidden).toBe(false);

    const freq = inputs('btnFreq');
    freq[1].checked = true;
    freq[1].dispatchEvent(new Event('change', {bubbles: true}));
    expect(second.hidden).toBe(false);
    expect(global().hidden).toBe(false);

    freq[0].checked = false;
    freq[0].dispatchEvent(new Event('change', {bubbles: true}));
    freq[1].checked = false;
    freq[1].dispatchEvent(new Event('change', {bubbles: true}));
    expect(first.hidden).toBe(true);
    expect(second.hidden).toBe(true);
    expect(global().hidden).toBe(true);
});
