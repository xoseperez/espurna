import { expect, test } from 'vitest';
import { acFromCommands, importSummary, tasmotaToEspurna } from '../src/tasmota.mjs';

// Relay1 @ GPIO14, Relay2 @ GPIO15, Switch1 @ GPIO13, Switch2 @ GPIO12, Button1 @ GPIO2
const GPIO = new Array(18).fill(0);
GPIO[2] = 32;
GPIO[12] = 161;
GPIO[13] = 160;
GPIO[14] = 224;
GPIO[15] = 225;

const TEMPLATE = {NAME: 'Wall', GPIO, FLAG: 0, BASE: 18};

test('without AC option, nothing about frequency detection is imported', () => {
    const settings = tasmotaToEspurna(TEMPLATE);
    expect(Object.keys(settings).filter((x) => x.startsWith('btnFreq')))
        .toHaveLength(0);
    expect(settings['btnAcFreq']).toBeUndefined();
    expect(settings['btnMode0']).toEqual('pushbutton');
});

test('AC option marks only Switch inputs by default', () => {
    const settings = tasmotaToEspurna(TEMPLATE, {ac: true, acButtons: false, freq: 50});

    // button indices follow GPIO order: GPIO2 (Button1), GPIO12 (Switch2), GPIO13 (Switch1)
    expect(settings['btnGpio0']).toEqual('2');
    expect(settings['btnFreq0']).toBeUndefined();

    for (const index of [1, 2]) {
        expect(settings[`btnFreq${index}`]).toEqual('1');
        expect(settings[`btnMode${index}`]).toEqual('switch');
        expect(settings[`btnPress${index}`]).toEqual('relay-toggle');
        expect(settings[`btnRlse${index}`]).toEqual('relay-toggle');
        expect(settings[`btnDebDel${index}`]).toEqual('100');
    }

    // SwitchN controls RelayN, ESPurna indices are in GPIO order
    expect(settings['btnGpio1']).toEqual('12');
    expect(settings['btnRelay1']).toEqual('1');
    expect(settings['btnGpio2']).toEqual('13');
    expect(settings['btnRelay2']).toEqual('0');

    expect(settings['btnAcFreq']).toEqual('50');
});

test('AC option for Button inputs and 60Hz', () => {
    const settings = tasmotaToEspurna(TEMPLATE, {ac: true, acButtons: true, freq: 60});
    expect(settings['btnFreq0']).toEqual('1');
    expect(settings['btnRelay0']).toEqual('0');
    expect(settings['btnAcFreq']).toEqual('60');
});

test('summary mentions AC inputs', () => {
    const json = JSON.stringify(TEMPLATE);
    expect(importSummary(json))
        .toEqual('"Wall": 2 relay(s), 3 button(s), 0 LED(s)');
    expect(importSummary(json, {ac: true, acButtons: false, freq: 50}))
        .toEqual('"Wall": 2 relay(s), 3 button(s), 0 LED(s), 2 of them AC mains switch(es) @ 50 Hz');
});

test('SwitchDebounce ending with 9 enables AC detection', () => {
    expect(acFromCommands('SwitchDebounce 69')).toBe(true);
    expect(acFromCommands('backlog SwitchMode1 1; switchdebounce 99')).toBe(true);
    expect(acFromCommands('SwitchDebounce 50')).toBe(false);
    expect(acFromCommands('')).toBe(false);
});
