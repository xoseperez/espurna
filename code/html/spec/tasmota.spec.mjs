import { expect, test } from 'vitest';
import { importSummary, tasmotaToEspurna } from '../src/tasmota.mjs';

// Relay1 @ GPIO14, Relay2 @ GPIO15, Switch1 @ GPIO13, Switch2 @ GPIO12, Button1 @ GPIO2
const GPIO = new Array(18).fill(0);
GPIO[2] = 32;
GPIO[12] = 161;
GPIO[13] = 160;
GPIO[14] = 224;
GPIO[15] = 225;

const TEMPLATE = {NAME: 'Wall', GPIO, FLAG: 0, BASE: 18};

test('Button and Switch inputs control the relay with the same number', () => {
    const settings = tasmotaToEspurna(TEMPLATE);

    // button indices follow GPIO order: GPIO2 (Button1), GPIO12 (Switch2), GPIO13 (Switch1)
    expect(settings['btnGpio0']).toEqual('2');
    expect(settings['btnMode0']).toEqual('pushbutton');
    expect(settings['btnRelay0']).toEqual('0');

    expect(settings['btnGpio1']).toEqual('12');
    expect(settings['btnMode1']).toEqual('switch');
    expect(settings['btnRelay1']).toEqual('1');
    expect(settings['btnGpio2']).toEqual('13');
    expect(settings['btnRelay2']).toEqual('0');

    // AC mains detection is set on the RELAYS panel, not by the import
    expect(Object.keys(settings).filter((x) => x.startsWith('btnFreq'))).toHaveLength(0);
    expect(settings['btnAcFreq']).toBeUndefined();
});

test('summary', () => {
    expect(importSummary(tasmotaToEspurna(TEMPLATE)))
        .toEqual('2 relay(s), 3 button(s), 0 LED(s)');
});

// templates.blakadder.com, legacy 8-bit codes
const MS104B = {NAME: 'Moes MS-104B', GPIO: [0, 0, 17, 0, 160, 0, 0, 0, 10, 9, 21, 22, 0], FLAG: 0, BASE: 18};
const MINIR2 = {NAME: 'Sonoff MINIR2', GPIO: [17, 0, 0, 0, 9, 0, 0, 0, 21, 157, 0, 0, 0], FLAG: 0, BASE: 1};
// ESP32, 36 entries
const MINIR4 = {NAME: 'Sonoff MINIR4', GPIO: [32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 576, 0, 0, 0, 0, 0, 0,
    224, 160, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], FLAG: 0, BASE: 1};

test('legacy ESP8266 template uses the user pin layout', () => {
    expect(tasmotaToEspurna(MS104B)['sysDevice']).toEqual('Moes MS-104B');
    expect(tasmotaToEspurna(MS104B)['desc']).toBeUndefined();

    const settings = tasmotaToEspurna(MS104B);

    // same as the MOES_MS_104B board: relays GPIO14 / 15, switches GPIO13 / 12, button GPIO2
    expect(settings['relayGpio0']).toEqual('14');
    expect(settings['relayGpio1']).toEqual('15');
    expect(settings['btnGpio0']).toEqual('2');
    expect(settings['btnRelay0']).toEqual('0');
    expect(settings['btnMode0']).toEqual('pushbutton');

    // Switch2 @ GPIO12, Switch1 @ GPIO13
    expect(settings['btnGpio1']).toEqual('12');
    expect(settings['btnRelay1']).toEqual('1');
    expect(settings['btnGpio2']).toEqual('13');
    expect(settings['btnRelay2']).toEqual('0');
    expect(settings['btnMode1']).toEqual('switch');
    expect(settings['btnMode2']).toEqual('switch');
});

test('switch inputs toggle the relay on every change, without click actions', () => {
    const settings = tasmotaToEspurna(MINIR2);

    expect(settings['relayGpio0']).toEqual('12');
    expect(settings['btnGpio0']).toEqual('0');
    expect(settings['btnRelay0']).toEqual('0');

    expect(settings['btnGpio1']).toEqual('4');
    expect(settings['btnMode1']).toEqual('switch');
    expect(settings['btnPress1']).toEqual('relay-toggle');
    expect(settings['btnRlse1']).toEqual('relay-toggle');
    expect(settings['btnLclk1']).toEqual('none');
    expect(settings['btnLLclk1']).toEqual('none');

    // legacy LedLink
    expect(settings['ledGpio0']).toEqual('13');
    expect(settings['ledMode0']).toEqual('wifi');
});

test('ESP32 template positions past GPIO5 are remapped', () => {
    const settings = tasmotaToEspurna(MINIR4);

    // same as the itead-sonoff-minir4 build: relay GPIO26, S1 GPIO27, LED GPIO19, button GPIO0
    expect(settings['relayGpio0']).toEqual('26');
    expect(settings['btnGpio1']).toEqual('27');
    expect(settings['ledGpio0']).toEqual('19');
    expect(settings['ledInv0']).toEqual('1');
    expect(settings['btnGpio0']).toEqual('0');
});

test('PWM channels become light channels with a light state relay', () => {
    const GPIO = new Array(14).fill(0);
    GPIO[8] = 416; // GPIO12
    GPIO[10] = 417; // GPIO14
    GPIO[9] = 418; // GPIO13
    const settings = tasmotaToEspurna({NAME: 'Bulb', GPIO});

    expect(settings['ltDimmerGPIO0']).toEqual('12');
    expect(settings['ltDimmerGPIO1']).toEqual('13');
    expect(settings['ltDimmerGPIO2']).toEqual('14');
    expect(settings['relayProv0']).toEqual('light-state');
    expect(settings['relayGpio0']).toBeUndefined();
});

test('SwitchDebounce ending with 9 in the template commands makes Switch inputs AC mains switches', () => {
    const settings = tasmotaToEspurna({...MS104B, CMND: 'SwitchDebounce 69'});
    expect(settings['btnFreq0']).toBeUndefined();
    expect(settings['btnFreq1']).toEqual('1');
    expect(settings['btnFreq2']).toEqual('1');
    expect(settings['btnDebDel1']).toEqual('100');

    expect(tasmotaToEspurna({...MS104B, CMND: 'SwitchDebounce 50'})['btnFreq1']).toBeUndefined();
});
