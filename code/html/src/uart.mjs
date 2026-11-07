import { setInputOrSelectValueByKey, variableListeners } from './settings.mjs';

// GPIO_NONE of the firmware: the serial port does not use that pin
export const PIN_NONE = 153;

/**
 * @typedef {{tx: number, rx: number, txUser: string, rxUser: string}} UartPins
 */

/**
 * Options of the TX / RX select: the pin the build gives the serial port, or none
 * @param {number} pin
 * @returns {{value: string, text: string}[]}
 */
export function pinOptions(pin) {
    return [
        {value: pin.toString(), text: `GPIO${pin}: serial port`},
        {value: PIN_NONE.toString(), text: 'Off: the pin is free for a button, a relay or an LED'},
    ];
}

/**
 * What the WebUI says about the pins that a button, a relay or an LED took from the serial port by itself
 * @param {UartPins} pins
 * @returns {string}
 */
export function releaseText(pins) {
    const out = [];
    if (pins.txUser) {
        out.push(`GPIO${pins.tx} (TX) is used by ${pins.txUser}`);
    }

    if (pins.rxUser) {
        out.push(`GPIO${pins.rx} (RX) is used by ${pins.rxUser}`);
    }

    if (!out.length) {
        return '';
    }

    return `${out.join(', ')}: the serial port leaves it, nothing to set here.`;
}

/**
 * @param {string} name
 * @param {number} pin
 */
function fillSelect(name, pin) {
    const select = document.querySelector(`select[name='${name}']`);
    if (!(select instanceof HTMLSelectElement)) {
        return;
    }

    select.replaceChildren(...pinOptions(pin).map((option) => {
        const elem = document.createElement('option');
        elem.value = option.value;
        elem.textContent = option.text;
        return elem;
    }));
}

/**
 * @param {string} _
 * @param {UartPins} pins
 */
function onPins(_, pins) {
    const fieldset = document.getElementById('uartPins');
    if (!fieldset) {
        return;
    }

    fillSelect('uartTx0', pins.tx);
    fillSelect('uartRx0', pins.rx);

    const hint = document.getElementById('uartRelease');
    if (hint) {
        hint.textContent = releaseText(pins);
        hint.hidden = !hint.textContent;
    }

    fieldset.hidden = false;
}

/**
 * the values come after the options exist (uartPins is sent first)
 * @param {string} key
 * @param {number} value
 */
function onPin(key, value) {
    setInputOrSelectValueByKey(document, key, value.toString());
}

export function init() {
    variableListeners({
        uartPins: onPins,
        uartTx0: onPin,
        uartRx0: onPin,
    });
}
