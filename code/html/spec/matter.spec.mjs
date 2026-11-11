import { expect, test } from 'vitest';
import { vendorName, windowText } from '../src/matter.mjs';

test('pairing window button text', () => {
    expect(windowText(0)).toBe("Open pairing window");
    expect(windowText(900)).toBe("Pairing window open, 15:00 left");
    expect(windowText(61)).toBe("Pairing window open, 1:01 left");
});

test('known controller vendors by name', () => {
    expect(vendorName(0x132F)).toBe("Smart speaker");
    expect(vendorName(0x1349)).toBe("Apple Home");
});

test('test vendor ids name the usual self-made controllers', () => {
    expect(vendorName(0xFFF1)).toContain("matter-server");
    expect(vendorName(0xFFF4)).toContain("0xFFF4");
});

test('other vendors as a number', () => {
    expect(vendorName(0x10)).toBe("Vendor 0x0010");
});
