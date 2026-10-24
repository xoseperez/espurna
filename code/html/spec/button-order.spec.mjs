import { beforeAll, expect, test } from 'vitest';
import { BUTTON_SCHEMA, RELAY_SCHEMA, setupButtonDom } from './button.dom.mjs';
import { updateKeyValue } from '../src/settings.mjs';
import { getElementName } from '../src/settings/name.mjs';

beforeAll(() => {
    setupButtonDom();
});

test('relayConfig before btnConfig, plain buttons only', () => {
    updateKeyValue('relayConfig', {
        schema: RELAY_SCHEMA,
        values: [
            ['Left', '153', 'a'],
            ['Right', '12', 'b'],
        ],
    });

    updateKeyValue('btnConfig', {
        schema: BUTTON_SCHEMA,
        values: [
            // relay #1 has two candidates and no GPIO match, first one wins
            ['14', '0', '5', '0', '2', '50', '1'],
            ['5', 'false', '5', '0', '2', '50', '1'],
            // relay #0 button, its GPIO is not the relay 'Button GPIO' but it is the only one
            ['0', 'false', '5', '0', '2', '50', '0'],
        ],
    });

    const cards = /** @type {HTMLCollection} */(document.getElementById('relayConfig')?.children);
    /** @param {Element} card */
    const name = (card) =>
        getElementName(/** @type {HTMLSelectElement} */(card.querySelector('select[name="btnFreq"]')));
    expect(name(cards[0])).toEqual('btnFreq2');
    expect(name(cards[1])).toEqual('btnFreq0');

    const fallback = Array.from(document.querySelectorAll('#buttonConfig [data-key^="btnFreq"]'))
        .map((x) => getElementName(/** @type {HTMLSelectElement} */(x)));
    expect(fallback).toEqual(['btnFreq1']);

    for (const filter of document.querySelectorAll('.button-ac-filter')) {
        expect(/** @type {HTMLElement} */(filter).hidden).toBe(true);
    }

    expect(document.getElementById('buttonAcGlobal')?.hidden).toBe(true);
    expect(document.getElementById('buttonOther')?.hidden).toBe(false);
});
