import { assert, expect, test } from 'vitest';
import {
    checkAndSetElementChanged,
    getDataForElement,
    getOriginalForElement,
    setOriginalsFromValues,
    setOriginalsFromValuesForNode,
    setSpanValue,
    setInputValue,
    setSelectValue,
} from '../src/settings.mjs';

import { isChangedElement } from '../src/settings/utils.mjs';

test('select unchanged with empty value when original is missing', () => {
    const node = document.createElement('select');
    for (let value of ['', '1', '2', '3']) {
        const elem = document.createElement('option');
        elem.value = value;
        node.appendChild(elem);
    }

    expect(getDataForElement(node))
        .toEqual(getOriginalForElement(node));

    node.selectedIndex = -1;
    expect(getDataForElement(node)).toBeNull();
    assert(!isChangedElement(node));

    node.selectedIndex = 1;
    assert(!isChangedElement(node));
    assert(checkAndSetElementChanged(node));

    node.selectedIndex = 0;
    assert(isChangedElement(node));

    assert(checkAndSetElementChanged(node));
    assert(!isChangedElement(node));

    setOriginalsFromValuesForNode(node);
    node.selectedIndex = 0;

    assert(!checkAndSetElementChanged(node));
    assert(!isChangedElement(node));
});

test('number input unchanged with empty value when original is missing', () => {
    const node = document.createElement('input');
    node.type = 'number';

    expect(getDataForElement(node))
        .toEqual(getOriginalForElement(node));

    expect(getDataForElement(node)).toBeNaN();
    assert(!isChangedElement(node));

    setInputValue(node, 12345);
    assert(!isChangedElement(node));
    assert(checkAndSetElementChanged(node));

    setInputValue(node, '');
    assert(isChangedElement(node));

    assert(checkAndSetElementChanged(node));
    assert(!isChangedElement(node));

    setOriginalsFromValuesForNode(node);
    setInputValue(node, '');

    assert(!checkAndSetElementChanged(node));
    assert(!isChangedElement(node));
});

test('text input unchanged with empty value when original is missing', () => {
    const node = document.createElement('input');
    node.type = 'text';

    expect(getDataForElement(node))
        .toEqual(getOriginalForElement(node));

    const data = 'this never gets commited';
    expect(getDataForElement(node)).toBe('');
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
    expect(getDataForElement(node)).toBe('');
    assert(!isChangedElement(node));

    node.value = data;
    assert(!isChangedElement(node));
    assert(checkAndSetElementChanged(node));
    expect(getDataForElement(node)).toBe(data);
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
    expect(getDataForElement(input)).toBeNaN();

    setInputValue(input, 12345);
    expect(getDataForElement(input)).toBe(12345);

    setInputValue(input, '');
    expect(getDataForElement(input)).toBeNaN();

    setInputValue(input, '56789');
    expect(getDataForElement(input)).toBe(56789);

    setInputValue(input, 'text');
    expect(getDataForElement(input)).toBeNaN();

    input.type = 'text';
    input.value = '';

    setInputValue(input, null);
    expect(getDataForElement(input)).toBe('');

    setInputValue(input, 12345);
    expect(getDataForElement(input)).toBe('12345');

    setInputValue(input, '56789');
    expect(getDataForElement(input)).toBe('56789');

    setInputValue(input, 'text');
    expect(getDataForElement(input)).toBe('text');
});

test('checkbox input value update', () => {
    const input = document.createElement('input');
    input.type = 'checkbox';

    setInputValue(input, null);
    expect(getDataForElement(input)).toEqual(false);

    setInputValue(input, 12345);
    expect(getDataForElement(input)).toEqual(true);

    setInputValue(input, 0);
    expect(getDataForElement(input)).toEqual(false);

    setInputValue(input, 'true');
    expect(getDataForElement(input)).toEqual(true);

    setInputValue(input, 'false');
    expect(getDataForElement(input)).toEqual(false);

    setInputValue(input, 'yes');
    expect(getDataForElement(input)).toEqual(true);

    setInputValue(input, 'no');
    expect(getDataForElement(input)).toEqual(false);
});

test('select value update', () => {
    const select = document.createElement('select');
    select.innerHTML = `
        <option value="initial"></option>
        <option value="one">One</option>
        <option value="two">Two</option>
        <option value="three">Three</option>`;

    expect(getDataForElement(select)).toEqual('initial');

    setSelectValue(select, 'one');
    expect(getDataForElement(select)).toEqual('one');

    setSelectValue(select, 'two');
    expect(getDataForElement(select)).toEqual('two');

    setSelectValue(select, 'three');
    expect(getDataForElement(select)).toEqual('three');
});

test('select bitset update', () => {
    const select = document.createElement('select');
    select.multiple = true;
    select.innerHTML = `
        <option value="0"></option>
        <option value="1">One</option>
        <option value="2">Two</option>
        <option value="3">Three</option>
        <option value="4">Four</option>
        <option value="5">Five</option>`;

    expect(getDataForElement(select)).toEqual(0);
    setOriginalsFromValues([select]);

    setSelectValue(select, 1 << 1);

    expect(getDataForElement(select))
        .toEqual(1 << 1);
    assert(checkAndSetElementChanged(select));

    setSelectValue(select, (1 << 2) | (1 << 4));

    expect(getDataForElement(select))
        .toEqual((1 << 2) | (1 << 4));
    assert(!checkAndSetElementChanged(select));
    assert(isChangedElement(select));

    setOriginalsFromValues([select]);
    setSelectValue(select, (1 << 2) | (1 << 4));

    assert(!checkAndSetElementChanged(select));
    assert(!isChangedElement(select));

    setSelectValue(select, (1 << 1) | (1 << 4));

    assert(checkAndSetElementChanged(select));
    assert(isChangedElement(select));

    setSelectValue(select, (1 << 2) | (1 << 4));
    assert(checkAndSetElementChanged(select));
    assert(!isChangedElement(select));
});
