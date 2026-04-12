import { assert, beforeAll, expect, test } from 'vitest';

import { onElementChange } from '../src/settings/change.mjs';
import { isChangedElement } from '../src/settings/dataset.mjs';
import { setInputValue } from '../src/settings/input.mjs';

import {
    validateFormsPasswords,
    validateFormsReportValidity,
} from '../src/validate.mjs';

import {
    filterForm,
    formPassPair,
} from '../src/password/utils.mjs';

import { readFile } from 'node:fs/promises';

/**
 * @import { PasswordInputPair } from '../src/password.mjs'
 */

/** @returns {PasswordInputPair} */
function getFormPassPair() {
    const [form] = filterForm([...document.forms]);
    assert(form instanceof HTMLFormElement);

    const pair = formPassPair(form);
    assert(pair.length === 2);

    return pair;
}

beforeAll(async () => {
    for (let panel of ['password', 'general']) {
        const html = await readFile(`${import.meta.dirname}/../src/panel-${panel}.html`);
        document.body.innerHTML += Buffer.from(html.buffer).toString();
    }

    document.body.querySelectorAll('input').forEach((elem) => {
        elem.addEventListener('change', onElementChange);
        elem.dataset['original'] = '';
    });

    // TODO: impl detail? for password form, these are errors
    window.alert = (message) => {
        throw new Error(`WINDOW_ALERT: ${message}`);
    };

    const pair = getFormPassPair();
    expect(pair.length).toEqual(2);

    expect(pair[0].value).toEqual('');
    expect(isChangedElement(pair[0]))
        .toBeFalsy();

    expect(pair[1].value).toEqual('');
    expect(isChangedElement(pair[1]))
        .toBeFalsy();
});

/**
 * @param {HTMLInputElement} input
 * @param {string} value
 */
function changeInput(input, value) {
    setInputValue(input, value);
    input.dispatchEvent(new Event('change'));
}

/**
 * @param {PasswordInputPair} pair
 * @param {string} first
 * @param {string} second
 */
function changePasswordPair(pair, first, second) {
    setInputValue(pair[0], first);
    setInputValue(pair[1], second);
    pair.forEach((elem) => {
        elem.dispatchEvent(new Event('change'));
    });
}

test('password can be empty when unchanged', () => {
    const inputs = getFormPassPair();

    changePasswordPair(inputs, '', '');
    assert(validateFormsPasswords([...document.forms], {strict: false}));
});

test('password can be empty when other forms change', () => {
    const inputs = getFormPassPair();
    changePasswordPair(inputs, '', '');

    const hostname = document.forms.namedItem('form-general')
        ?.elements.namedItem('hostname');
    assert(hostname instanceof HTMLInputElement);

    changeInput(hostname, 'espurna-test1');
    assert(validateFormsReportValidity([...document.forms]));
    assert(validateFormsPasswords([...document.forms], {strict: false}));

    changeInput(hostname, 'espurna-test2');
    assert(validateFormsReportValidity([...document.forms]));
    assert(validateFormsPasswords([...document.forms], {strict: false}));
});

test('password cannot be empty when validator requires it', () => {
    const inputs = getFormPassPair();

    changePasswordPair(inputs, '', '');
    expect(() => validateFormsPasswords([...document.forms], {assumeChanged: true}))
        .toThrowError(/WINDOW_ALERT:/);
});

test('password equality check in lenient mode', () => {
    const inputs = getFormPassPair();

    changePasswordPair(inputs, '11111111', '11111111');
    assert(validateFormsReportValidity([...document.forms]));
    assert(validateFormsPasswords([...document.forms], {strict: false}));
});

test('password equality check in strict mode', () => {
    const inputs = getFormPassPair();

    changePasswordPair(inputs, 'does not work', 'does not work');
    expect(() => validateFormsPasswords([...document.forms], {strict: true}))
        .toThrowError(/WINDOW_ALERT:/);
});

test('password inputs missing one of the values', () => {
    const inputs = getFormPassPair();

    changePasswordPair(inputs, '', 'hello world');
    expect(() => validateFormsPasswords([...document.forms]))
        .toThrowError(/WINDOW_ALERT:/);

    changePasswordPair(inputs, 'hello world', '');
    expect(() => validateFormsPasswords([...document.forms]))
        .toThrowError(/WINDOW_ALERT:/);
});
