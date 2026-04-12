import { assert, expect, test } from 'vitest';

import { checkAndSetElementChanged } from '../src/settings/change.mjs';
import {
    getElementOriginal,
    isChangedElement,
    setOriginalsFromValues,
    setOriginalsFromValuesForNode,
} from '../src/settings/dataset.mjs';
import { setInputValue } from '../src/settings/input.mjs';
import { setBitsetSelect, setSelectValue } from '../src/settings/select.mjs';
import { setSpanValue } from '../src/settings/span.mjs';
import { getElementValue, getNamedElementValue } from '../src/settings/value.mjs';

test('select unchanged with empty value when original is missing', () => {
    const select = document.createElement('select');
    select.name = 'empty-one-two-three';
    select.innerHTML = `
        <option value=""></option>
        <option value="1">One</option>
        <option value="2">Two</option>
        <option value="3">Three</option>`;

    expect(getElementValue(select))
        .toBe('');
    expect(getElementOriginal(select))
        .toBeNull();

    select.selectedIndex = -1;
    expect(getElementValue(select))
        .toBeNull();
    expect(getElementOriginal(select))
        .toBeNull();
    expect(isChangedElement(select))
        .toBe(false);

    select.selectedIndex = 1;
    expect(isChangedElement(select)).toBe(false);
    expect(checkAndSetElementChanged(select)).toBe(true);
    expect(isChangedElement(select)).toBe(true);

    select.selectedIndex = 0;
    expect(isChangedElement(select)).toBe(true);
    expect(checkAndSetElementChanged(select)).toBe(false);
    expect(isChangedElement(select)).toBe(true);

    setOriginalsFromValues([select]);
    expect(getElementOriginal(select))
        .toBe('');
    expect(getElementOriginal(select))
        .toBe('');
    expect(checkAndSetElementChanged(select)).toBe(false);
    expect(isChangedElement(select)).toBe(false);
});

test('number input unchanged with empty value when original is missing', () => {
    const input = document.createElement('input');
    input.name = 'empty-value';
    input.type = 'number';

    expect(getElementValue(input))
        .toEqual(getElementOriginal(input));

    expect(getElementValue(input)).toBeNaN();
    assert(!isChangedElement(input));

    setInputValue(input, 12345);
    assert(!isChangedElement(input));
    assert(checkAndSetElementChanged(input));

    setInputValue(input, '');
    assert(isChangedElement(input));

    assert(checkAndSetElementChanged(input));
    assert(!isChangedElement(input));

    setOriginalsFromValues([input]);
    setInputValue(input, '');

    assert(!checkAndSetElementChanged(input));
    assert(!isChangedElement(input));
});

test('text input unchanged with empty value when original is missing', () => {
    const node = document.createElement('input');
    node.type = 'text';

    expect(getElementValue(node))
        .toEqual(getElementOriginal(node));

    const data = 'this value does not make the element changed';
    expect(getElementValue(node)).toBe('');
    assert(!isChangedElement(node));

    node.value = data;
    assert(!isChangedElement(node));
    assert(checkAndSetElementChanged(node));

    node.value = '';
    assert(isChangedElement(node));

    assert(checkAndSetElementChanged(node));
    assert(!isChangedElement(node));

    setOriginalsFromValuesForNode(node);
    node.value = '';

    assert(!checkAndSetElementChanged(node));
    assert(!isChangedElement(node));
});

test('element input data with and without original', () => {
    const node = document.createElement('input');
    node.type = 'text';

    const data = 'some kind of basic input value';
    expect(getElementValue(node)).toBe('');
    assert(!isChangedElement(node));

    node.value = data;
    assert(!isChangedElement(node));
    assert(checkAndSetElementChanged(node));
    expect(getElementValue(node)).toBe(data);
    assert(isChangedElement(node));

    setOriginalsFromValues([node]);
    assert(!isChangedElement(node));

    node.value = `${data}${data}`;
    assert(checkAndSetElementChanged(node));
    assert(isChangedElement(node));
});

test('span value replacement', () => {
    const span = document.createElement('span');

    span.dataset['valueTrue'] = 'SUCCESS';
    setSpanValue(span, true);
    expect(span.textContent).toBe('SUCCESS');

    span.dataset['valueFalse'] = 'FAILURE';
    setSpanValue(span, false);
    expect(span.textContent).toBe('FAILURE');

    span.dataset['value12345'] = '...';
    setSpanValue(span, 12345);
    expect(span.textContent).toBe('...');

    span.dataset['value56789'] = '???';
    setSpanValue(span, '56789');
    expect(span.textContent).toBe('???');

    span.dataset['valueFoo'] = 'bar';
    setSpanValue(span, ['foo','bar']);
    expect(span.textContent).toBe('barbar');
});

test('span value update', () => {
    const span = document.createElement('span');

    setSpanValue(span, true);
    expect(span.textContent).toBe('true');

    setSpanValue(span, false);
    expect(span.textContent).toBe('false');

    setSpanValue(span, 12345);
    expect(span.textContent).toBe('12345');

    setSpanValue(span, '56789');
    expect(span.textContent).toBe('56789');

    setSpanValue(span, ['123', 456, true]);
    expect(span.textContent).toBe('123456true');
});

test('span value pre attribute contents before the value', () => {
    const span = document.createElement('span');
    span.dataset['pre'] = 'this is a ';

    setSpanValue(span, 'test');
    expect(span.textContent).toBe('this is a test');

    setSpanValue(span, 12345);
    expect(span.textContent).toBe('this is a 12345');

    setSpanValue(span, false);
    expect(span.textContent).toBe('this is a false');

    setSpanValue(span, null);
    expect(span.textContent).toBe('this is a ');
});

test('span value post attribute contents after the value', () => {
    const span = document.createElement('span');
    span.dataset['post'] = ' dataset attribute';

    setSpanValue(span, 'another test for post');
    expect(span.textContent).toBe('another test for post dataset attribute');

    setSpanValue(span, 'test');
    expect(span.textContent).toBe('test dataset attribute');

    setSpanValue(span, 56789);
    expect(span.textContent).toBe('56789 dataset attribute');

    setSpanValue(span, true);
    expect(span.textContent).toBe('true dataset attribute');

    setSpanValue(span, null);
    expect(span.textContent).toBe(' dataset attribute');
});

test('span value pre and post attributes', () => {
    const span = document.createElement('span');
    span.dataset['pre'] = 'begin ';
    span.dataset['post'] = ' end';
    setSpanValue(span, ['123', '456', '789']);
    expect(span.textContent).toBe('begin 123 endbegin 456 endbegin 789 end');
});

test('input value update', () => {
    const input = document.createElement('input');

    input.type = 'number';
    input.value = '';

    setInputValue(input, null);
    expect(getElementValue(input)).toBeNaN();

    setInputValue(input, 12345);
    expect(getElementValue(input)).toBe(12345);

    setInputValue(input, '');
    expect(getElementValue(input)).toBeNaN();

    setInputValue(input, '56789');
    expect(getElementValue(input)).toBe(56789);

    setInputValue(input, 'text');
    expect(getElementValue(input)).toBeNaN();

    input.type = 'text';
    input.value = '';

    setInputValue(input, null);
    expect(getElementValue(input)).toBe('');

    setInputValue(input, 12345);
    expect(getElementValue(input)).toBe('12345');

    setInputValue(input, '56789');
    expect(getElementValue(input)).toBe('56789');

    setInputValue(input, 'text');
    expect(getElementValue(input)).toBe('text');
});

test('checkbox input value update', () => {
    const input = document.createElement('input');
    input.type = 'checkbox';

    setInputValue(input, null);
    expect(getElementValue(input)).toBe(false);

    setInputValue(input, 12345);
    expect(getElementValue(input)).toBe(true);

    setInputValue(input, 0);
    expect(getElementValue(input)).toBe(false);

    setInputValue(input, 'true');
    expect(getElementValue(input)).toBe(true);

    setInputValue(input, 'false');
    expect(getElementValue(input)).toBe(false);

    setInputValue(input, 'yes');
    expect(getElementValue(input)).toBe(true);

    setInputValue(input, 'no');
    expect(getElementValue(input)).toBe(false);
});

test('select value update', () => {
    const select = document.createElement('select');
    select.name = 'value';
    select.innerHTML = `
        <option value="initial"></option>
        <option value="one">One</option>
        <option value="two">Two</option>
        <option value="three">Three</option>`;

    expect(getNamedElementValue(select))
        .toEqual({
            'name': 'value',
            'value': 'initial',
        });

    for (const value of ['one', 'two', 'three']) {
        setSelectValue(select, value);
        expect(getNamedElementValue(select))
            .toEqual({
                'name': 'value',
                value,
            });
    }
});

test('select bitset update', () => {
    const select = document.createElement('select');
    select.multiple = true;
    select.name = 'bitset';
    select.innerHTML = `
        <option value="0"></option>
        <option value="1">One</option>
        <option value="2">Two</option>
        <option value="3">Three</option>
        <option value="4">Four</option>
        <option value="5">Five</option>`;

    setBitsetSelect(select);
    expect(getNamedElementValue(select))
        .toEqual({
            name: 'bitset',
            value: 0,
        });
    setOriginalsFromValues([select]);

    setSelectValue(select, 1 << 1);

    expect(getElementValue(select))
        .toEqual(1 << 1);
    expect(checkAndSetElementChanged(select))
        .toBe(true);

    function makeSelected() {
        return Array.from(select.options)
            .filter((x) => x.selected)
            .map((x) => x.text);
    }

    expect(makeSelected())
        .toEqual(["One"]);
    setSelectValue(select, (1 << 2) | (1 << 4));

    expect(getElementValue(select))
        .toEqual((1 << 2) | (1 << 4));
    expect(checkAndSetElementChanged(select))
        .toBe(false);
    expect(isChangedElement(select))
        .toBe(true);
    expect(makeSelected())
        .toEqual(["Two", "Four"]);

    setOriginalsFromValues([select]);
    setSelectValue(select, (1 << 2) | (1 << 4));

    expect(checkAndSetElementChanged(select))
        .toBe(false);
    expect(isChangedElement(select))
        .toBe(false);
    expect(makeSelected())
        .toEqual(["Two", "Four"]);

    setSelectValue(select, (1 << 1) | (1 << 5));

    expect(checkAndSetElementChanged(select))
        .toBe(true);
    expect(isChangedElement(select))
        .toBe(true);
    expect(makeSelected())
        .toEqual(["One", "Five"]);

    setSelectValue(select, (1 << 3) | (1 << 4));
    expect(checkAndSetElementChanged(select))
        .toBe(false);
    expect(isChangedElement(select))
        .toBe(true);
    expect(makeSelected())
        .toEqual(["Three", "Four"]);
});
