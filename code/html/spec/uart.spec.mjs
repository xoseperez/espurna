import { expect, test } from 'vitest';
import { PIN_NONE, pinOptions, releaseText } from '../src/uart.mjs';

test('pin options: the build pin or none', () => {
    expect(pinOptions(3).map((x) => x.value)).toStrictEqual(['3', `${PIN_NONE}`]);
    expect(pinOptions(11)[0].text).toContain('GPIO11');
});

test('release text names the setting that took the pin', () => {
    expect(releaseText({tx: 1, rx: 3, txUser: '', rxUser: ''})).toBe('');
    expect(releaseText({tx: 1, rx: 3, txUser: '', rxUser: 'btnGpio2'}))
        .toBe('GPIO3 (RX) is used by btnGpio2: the serial port leaves it, nothing to set here.');
    expect(releaseText({tx: 11, rx: 10, txUser: 'ledGpio0', rxUser: 'relayGpio0'}))
        .toContain('GPIO11 (TX) is used by ledGpio0, GPIO10 (RX) is used by relayGpio0');
});
