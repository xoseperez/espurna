import { assert, afterAll, expect, test } from 'vitest';

import { randomString } from '../src/core.mjs';
import { addFromTemplate } from '../src/template.mjs';
import {
    checkAndSetElementChanged,
    getData,
    groupSettingsAdd,
    groupSettingsDel,
    setInputOrSelectValueByKey,
    setInputValue,
    setSpanValueByKey,
} from '../src/settings.mjs';

import {
    countChangedElements,
    isChangedElement,
    setChangedElement,
    setGroupElement,
} from '../src/settings/utils.mjs';

afterAll(() => {
  document.body.innerHTML = '';
  expect(document.body.childElementCount)
    .toEqual(0);
});

/**
 * @param {import('../src/settings.mjs').ElementValue} lhs
 * @param {import('../src/settings.mjs').DataValue} rhs
 */
function expectData(lhs, rhs) {
    if (typeof lhs === 'boolean') {
        assert(typeof rhs === 'number');
        expect(lhs).toEqual(!!rhs);
    } else {
        expect(lhs).toEqual(rhs);
    }
}

test('processed data can be gathered back', () => {
    const PLAIN = {
        plainText: 'foobar',
        plainNumber: 12345,
        plainRange: 74,
        plainBox: true,
    };

    const CFGS = [
        {groupName: 'one', groupValue: 1},
        {groupName: 'five', groupValue: 5},
        {groupName: 'nine', groupValue: 9},
        {groupName: 'fifty-five', groupValue: 55},
        {groupName: 'one-hundred', groupValue: 100},
    ];

    document.body.innerHTML += `
    <form id="gather">
        <fieldset id="gather-plain">
            <legend>Plain kvs</legend>
            <input name="plainText" type="text"></input>
            <input name="plainNumber" type="number"></input>
            <input name="plainRange" type="range"></input>
            <input name="plainBox" type="checkbox"></input>
        </fieldset>
        <div id="gather-group">
        </div>
    </form>
    <template id="template-gather-group">
        <fieldset>
            <legend>Group <span data-key="template-id" data-pre="#"></span></legend>
            <input name="groupName" type="text"></input>
            <input name="groupValue" type="number"></input>
        </fieldset>
    </template>`;

    const plain = document.getElementById('gather-plain');
    assert(plain instanceof HTMLFieldSetElement);

    for (let [key, value] of Object.entries(PLAIN)) {
        setInputOrSelectValueByKey(plain, key, value);
    }

    const group = document.getElementById('gather-group');
    assert(group instanceof HTMLElement);

    for (let cfg of CFGS) {
        addFromTemplate(group, 'gather-group', cfg);
    }

    expect(group.childElementCount)
        .toEqual(CFGS.length);

    const form = /** @type {HTMLFormElement | null} */
        (document.getElementById('gather'));
    assert(form);

    const form_data = new FormData(form);
    expect(form_data.getAll('groupValue'));

    // retrieves everything, regardless of 'changed' state
    const data = getData([form], {assumeChanged: true});
    expect(data.del.length)
        .toEqual(0);

    const dataset = data.set;
    expect(Object.entries(dataset).length)
        .toEqual(Object.keys(PLAIN).length + (2 * CFGS.length));

    for (let [key, value] of Object.entries(PLAIN)) {
        expectData(value, dataset[key]);
    }

    CFGS.forEach((cfg, index) => {
        for (let [key, value] of Object.entries(cfg)) {
            expectData(value, dataset[`${key}${index}`]);
        }
    })
});

/**
 * @param {string} name
 * @returns {string}
 */
function makeFormGroup(name) {
    return `
    <form id="${name}">
        <div id="${name}-group" class="settings-group" data-settings-schema="foo bar">
        </div>
    </form>`;
}

const TEMPLATE_GROUP = `
    <template id="template-group">
        <fieldset>
            <legend>Foo <span data-key="template-id" data-pre="#"></span></legend>
            <input name="foo" type="text" required></input>
            <legend>Bar #<span data-key="template-id"></span></legend>
            <input name="bar" type="text" ></input>
        </fieldset>
    </template>`;

test('settings group modify', () => {
    document.body.innerHTML += makeFormGroup('modify');
    document.body.innerHTML += TEMPLATE_GROUP;

    const modify = /** @type {HTMLDivElement | null} */
        (document.getElementById('modify-group'));
    assert(modify);

    addFromTemplate(modify, 'group', {foo: 'one'});
    addFromTemplate(modify, 'group', {foo: 'two'});
    addFromTemplate(modify, 'group', {foo: 'three'});

    const last = /** @type {HTMLInputElement | null} */
        (modify?.lastElementChild?.children[1]);
    assert(last);

    setInputValue(last, 'something else');
    assert(checkAndSetElementChanged(last));

    const first = /** @type {HTMLInputElement | null} */
        (modify?.firstElementChild?.children[1]);
    assert(first);

    setInputValue(first, 'complete opposite');
    assert(checkAndSetElementChanged(first));

    const form = /** @type {HTMLFormElement | null} */
        (document.getElementById('modify'));
    assert(form);

    const data = getData([form]);

    expect(data.del.length)
        .toEqual(0);

    expect(data.set)
        .toEqual({
            foo0: 'complete opposite',
            foo2: 'something else',
        });
});

test('settings group append', () => {
    document.body.innerHTML += makeFormGroup('append');
    document.body.innerHTML += TEMPLATE_GROUP;

    const append = /** @type {HTMLDivElement | null} */
        (document.getElementById('append-group'));
    assert(append);

    addFromTemplate(append, 'group', {foo: 'first'});
    addFromTemplate(append, 'group', {foo: 'second'});
    addFromTemplate(append, 'group', {foo: 'third'});
    addFromTemplate(append, 'group', {foo: 'fourth'});
    expect(append.children.length).toEqual(4);

    addFromTemplate(append, 'group', {foo: 'fifth', bar: 'element'});
    groupSettingsAdd(append);
    expect(append.children.length).toEqual(5);

    const last = /** @type {HTMLFieldSetElement | null} */
        (append?.lastElementChild);
    assert(last);

    const foo = /** @type {HTMLInputElement | null} */
        (last.children[1]);
    assert(foo);

    assert(isChangedElement(foo));
    setInputValue(foo, 'pending value');

    const form = /** @type {HTMLFormElement | null} */
        (document.getElementById('append'));
    assert(form);

    let data = getData([form]);

    expect(data.del.length)
        .toEqual(0);
    expect(data.set)
        .toEqual({
            foo4: 'pending value',
        });

    groupSettingsDel(append, last);
    expect(append.children.length).toEqual(4);

    data = getData([form]);

    expect(data.del.length)
        .toEqual(0);
    expect(data.set)
        .toEqual({});
});

test('settings group remove', () => {
    document.body.innerHTML += makeFormGroup('remove');
    document.body.innerHTML += TEMPLATE_GROUP;

    const remove = /** @type {HTMLDivElement | null} */
        (document.getElementById('remove-group'));
    assert(remove);

    addFromTemplate(remove, 'group', {foo: '1111111'});
    addFromTemplate(remove, 'group', {foo: '2222222', bar: 'foobarfoo'});
    addFromTemplate(remove, 'group', {foo: '3333333', bar: 'barfoobar'});
    addFromTemplate(remove, 'group', {foo: '4444444'});
    expect(remove.children.length).toEqual(4);

    const second = remove.children[1];
    assert(second instanceof HTMLFieldSetElement);

    const form = document.getElementById('remove');
    assert(form instanceof HTMLFormElement);

    let data = getData([form]);
    expect(data.del.length)
        .toEqual(0);
    expect(Object.entries(data.set).length)
        .toEqual(0);

    // until now, all rows are expected to be 'unchanged'
    groupSettingsDel(remove, second);
    expect(remove.children.length)
        .toEqual(3);
    expect(countChangedElements(remove))
        .toEqual(4);

    // 2nd row removal should handle following keys
    data = getData([form]);

    expect(data.del)
        .toEqual(['foo3', 'bar3']);
    expect(data.set)
        .toEqual({
            bar1: 'barfoobar',
            bar2: '',
            foo1: 3333333,
            foo2: 4444444,
        });

    // extra row is always at the end. because add event was triggered,
    // make sure that the required fields are in the payload
    addFromTemplate(remove, 'group', {foo: '5555555', bar: 'ttttttt'});
    groupSettingsAdd(remove);

    // since the new row is on top of the removed one, no need to erase it
    // non-required data, however, should still be removed when still 'unchanged'
    data = getData([form]);

    expect(data.del)
        .toEqual(['bar3']);
    expect(data.set)
        .toEqual({
            bar1: 'barfoobar',
            bar2: '',
            foo1: 3333333,
            foo2: 4444444,
            foo3: 5555555,
        });

    addFromTemplate(remove, 'group', {foo: '6666666', bar: 'yyyyyyy'});
    groupSettingsAdd(remove);

    const last = remove?.lastElementChild;
    assert(last instanceof HTMLFieldSetElement);

    const bar = last.querySelector('input[name=bar]');
    assert(bar instanceof HTMLInputElement);

    setChangedElement(bar);

    const first = remove.children[0];
    assert(first instanceof HTMLFieldSetElement);

    expect(remove.children.length)
        .toEqual(5);
    expect(countChangedElements(remove))
        .toEqual((3 * 2) + 1);

    groupSettingsDel(remove, first);
    expect(remove.children.length)
        .toEqual(4);
    expect(countChangedElements(remove))
        .toEqual(4 * 2);

    // substituted row keys should no longer be in del set
    // resulting data is effectively every element present
    data = getData([form]);

    expect(data.del.length)
        .toEqual(0);
    expect(data.set)
        .toEqual({
            bar0: 'barfoobar',
            bar1: '',
            bar2: 'ttttttt',
            bar3: 'yyyyyyy',
            foo0: 3333333,
            foo1: 4444444,
            foo2: 5555555,
            foo3: 6666666,
        });

    while (remove.firstElementChild instanceof HTMLFieldSetElement) {
        groupSettingsDel(remove, remove.firstElementChild);
    }

    // original data removed, extra rows are omitted
    data = getData([form]);

    expect(data.del.length)
        .toEqual(8);
    expect(data.del)
        .toEqual(expect.arrayContaining([
            'foo0', 'bar0',
            'foo1', 'bar1',
            'foo2', 'bar2',
            'foo3', 'bar3',
        ]));
    expect(data.set)
        .toEqual({});
});

test('settings group schema remove', () => {
    document.body.innerHTML += `
    <form id="schema-del">
        <div id="schema-del-group" class="settings-group" data-settings-schema-del="foo" data-settings-schema="foo bar">
        </div>
    </form>`;
    document.body.innerHTML += TEMPLATE_GROUP;

    const group = document.getElementById('schema-del-group');
    assert(group instanceof HTMLDivElement);

    addFromTemplate(group, 'group', {foo: 'asdasdasd'});
    addFromTemplate(group, 'group', {foo: 'foobarfoo', bar: 'barfoobar'});
    addFromTemplate(group, 'group', {foo: 'oneoneone', bar: 'twotwotwo'});
    expect(group.children.length).toEqual(3);

    const form = group.parentElement;
    assert(form instanceof HTMLFormElement);

    let data = getData([form]);
    expect(data.set)
        .toEqual({});
    expect(data.del.length)
        .toEqual(0);

    while (group.firstElementChild instanceof HTMLFieldSetElement) {
        groupSettingsDel(group, group.firstElementChild);
    }

    data = getData([form]);

    expect(data.del.length)
        .toEqual(3);
    expect(data.del)
        .toEqual(expect.arrayContaining([
            'foo0',
            'foo1',
            'foo2',
        ]));
    expect(data.set)
        .toEqual({});
});

test('number inputs without data consistently serialize as nan string', () => {
    document.body.innerHTML += `
    <form id="numbers-and-nan-strings">
        <input name="number:a" type="number">
        <input name="number:b" type="number">
        <input name="number:c" type="number">
        <input name="number:d" type="number">
    </form>
    `;

    const form = document.forms.namedItem("numbers-and-nan-strings");
    assert(form instanceof HTMLFormElement);

    setInputOrSelectValueByKey(form, "number:a", 12345);
    setInputOrSelectValueByKey(form, "number:d", 56789);

    const data = getData([form], {assumeChanged: true});
    expect(data.del.length)
        .toEqual(0);
    expect(Object.keys(data.set))
        .toEqual(expect.arrayContaining(
            ['number:a', 'number:b', 'number:c', 'number:d']));

    expect(data.set["number:a"]).toEqual(12345);
    expect(data.set["number:b"]).toEqual("nan");
    expect(data.set["number:c"]).toEqual("nan");
expect(data.set["number:d"]).toEqual(56789);
});

test('mixed plain and group element names should not conflict with each other', () => {
    document.body.innerHTML += `
    <form id="mixed-plain-and-group">
        <fieldset id="plain-value-1">
            <input name="foo">
            <input name="bar">
            <input name="baz" readonly>
        </fieldset>
        <fieldset id="group-value-1">
            <input name="foo">
            <input name="foo">
            <input name="foo">
            <input name="foo">
            <input name="foo">
        </fieldset>
        <fieldset id="group-value-2">
            <input name="bar">
            <input name="bar">
            <input name="bar">
        </fieldset>
        <fieldset id="plain-value-2">
            <span data-key="foo">
            </span>
        </fieldset>
        <fieldset id="plain-value-2">
            <span data-key="bar">
            </span>
        </fieldset>
    </form>
    `;

    const form = document.forms.namedItem("mixed-plain-and-group");
    assert(form instanceof HTMLFormElement);

    /** @type {{[k: string]: string}} */
    const values = {};

    /**
     * @param {HTMLInputElement} elem
     * @param {number} index
     */
    function updateInput(elem, index) {
        setGroupElement(elem);
        setInputValue(elem, randomString(16));
        values[`${elem.name}${index}`] = elem.value;
    }

    /** @type {NodeListOf<HTMLInputElement>} */
    (form.querySelectorAll('#group-value-1 > input'))
        .forEach(updateInput);

    /** @type {NodeListOf<HTMLInputElement>} */
    (form.querySelectorAll('#group-value-2 > input'))
        .forEach(updateInput);

    const plain = [
        ['foo', 'plain value'],
        ['bar', 'set only once'],
    ];

    for (const [name, value] of plain) {
        setInputOrSelectValueByKey(form, name, value);
        setSpanValueByKey(form, name, value);
        values[name] = value;
    }

    setInputOrSelectValueByKey(form, 'baz', 'for plain elements');

    const data = getData([form], {assumeChanged: true});
    expect(data.del.length).toBe(0);
    expect(data.set).toEqual(values);

    form.querySelectorAll('span')
        .forEach((elem) => {
            const key = elem.dataset["key"];
            assert(key !== undefined);
            assert(values[key] !== undefined);
            expect(elem.textContent)
                .toEqual(values[key]);
        });

    const baz = form.querySelector('input[name="baz"]');
    assert(baz instanceof HTMLInputElement);
    expect(baz.value).toEqual('for plain elements');
});
