import { afterEach, assert, expect, test } from 'vitest';

import { randomString } from '../src/core.mjs';

import { checkAndSetElementChanged } from '../src/settings/change.mjs';
import {
    getOriginals,
    isChangedElement,
    makeDataRequest,
    pendingChanges,
    setChangedElement,
    setOriginalFromValue,
    setOriginalsFromValuesForNode,
} from '../src/settings/dataset.mjs';
import { setGroup, setGroupCleanup, setGroupElement, setGroupElements } from '../src/settings/group.elem.mjs';
import {
    groupSettingsAdd,
    groupSettingsDel,
} from '../src/settings/group.mjs';
import { setInputValue } from '../src/settings/input.mjs';

import {
    addFromTemplate,
    addOriginalsFromTemplate,
    fromSchema,
} from '../src/template.mjs';

import {
    setInputOrSelectValueByKey,
    setSpanValueByKey,
} from '../src/settings.mjs';

const __forms = new Set();

afterEach(() => {
  document.querySelectorAll('form')
    .forEach((form) => {
        expect(__forms.has(form.id)).toBe(false);
        __forms.add(form.id);
    });
  document.body.innerHTML = '';
});

test('processed data can be gathered back', () => {
    const PLAIN = {
        plainText: 'foobar',
        plainNumber: 12345,
        plainRange: 74,
        plainBox: true,
    };

    const GROUP_NAME = 'groupName'
    const GROUP_VALUE = 'groupValue';

    const KEYS = [GROUP_NAME, GROUP_VALUE];
    const VALUES = [
        ['one', 1],
        ['five', 5],
        ['nine', 9],
        ['fifty-five', 55],
        ['one-hundred', 100],
    ];

    const formId = 'gather';

    const plainId = 'gather-plain';
    const groupId = 'gather-group';

    document.body.innerHTML += `
    <form id="${formId}">
        <fieldset id="${plainId}">
            <legend>Plain kvs</legend>
            <input name="plainText" type="text"></input>
            <input name="plainNumber" type="number"></input>
            <input name="plainRange" type="range"></input>
            <input name="plainBox" type="checkbox"></input>
        </fieldset>
        <div id="${groupId}" class="settings-group">
        </div>
    </form>
    <template id="template-${groupId}">
        <fieldset>
            <legend>Group <span data-key="template-id" data-pre="#"></span></legend>
            <input name="groupName" type="text"></input>
            <input name="groupValue" type="number"></input>
        </fieldset>
    </template>`;

    const plain = document.getElementById(plainId);
    assert(plain instanceof HTMLFieldSetElement);

    for (let [key, value] of Object.entries(PLAIN)) {
        setInputOrSelectValueByKey(plain, key, value);
    }

    const group = document.getElementById(groupId);
    assert(group instanceof HTMLElement);

    // w/ schema variant is expected to set originals after adding elements
    addOriginalsFromTemplate(group, groupId, {entries: VALUES, schema: KEYS});
    expect(VALUES.length)
        .toEqual(group.childElementCount)

    // retrieves everything, independent of ours 'changed' state
    const data = getOriginals(formId);
    const flat = Object.assign(PLAIN,
        VALUES.map((x, index) =>
            fromSchema(x, KEYS.map((key) => `${key}${index}`)))
        .reduce((prev, curr) => Object.assign(prev, curr), {}));

    expect(flat).toEqual(data);
});

/**
 * @param {string} name
 * @returns {string}
 */
function makeFormGroup(name) {
    expect(getOriginals(name), `${name} should not exist yet`).toEqual({});
    return `
    <form id="${name}">
        <div id="${name}-group" class="settings-group">
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
    const formId = 'modify';

    document.body.innerHTML += makeFormGroup(formId);
    document.body.innerHTML += TEMPLATE_GROUP;

    const modify = /** @type {HTMLDivElement | null} */
        (document.getElementById('modify-group'));
    assert(modify);

    addFromTemplate(modify, 'group', {foo: 'one'});
    addFromTemplate(modify, 'group', {foo: 'two'});
    addFromTemplate(modify, 'group', {foo: 'three'});

    setOriginalsFromValuesForNode(modify);

    const last = /** @type {HTMLInputElement | null} */
        (modify?.lastElementChild?.children[1]);
    assert(last);

    setInputValue(last, 'something else');
    expect(checkAndSetElementChanged(last)).toBe(true);

    const first = /** @type {HTMLInputElement | null} */
        (modify?.firstElementChild?.children[1]);
    assert(first);

    setInputValue(first, 'complete opposite');
    expect(checkAndSetElementChanged(first)).toBe(true);

    const form = /** @type {HTMLFormElement | null} */
        (document.getElementById(formId));
    assert(form);

    const data = getOriginals(formId);
    expect(data['foo0']).toEqual('one');
    expect(data['foo1']).toEqual('two');
    expect(data['foo2']).toEqual('three');

    const request = makeDataRequest([form]);
    expect(request.del).toEqual([]);
    expect(request.set).toEqual({
        foo0: 'complete opposite',
        foo2: 'something else',
    });
});

test('settings group append', () => {
    const formId = 'append';

    document.body.innerHTML += makeFormGroup(formId);
    document.body.innerHTML += TEMPLATE_GROUP;

    const append = /** @type {HTMLDivElement | null} */
        (document.getElementById(`${formId}-group`));
    assert(append);

    let target = addFromTemplate(append, 'group', {foo: 'first'});
    assert(target instanceof HTMLElement);
    groupSettingsAdd(append, target);

    target = addFromTemplate(append, 'group', {foo: 'second'});
    assert(target instanceof HTMLElement);
    groupSettingsAdd(append, target);

    target = addFromTemplate(append, 'group', {foo: 'third'});
    assert(target instanceof HTMLElement);
    groupSettingsAdd(append, target);

    target = addFromTemplate(append, 'group', {foo: 'fourth'});
    assert(target instanceof HTMLElement);
    groupSettingsAdd(append, target);

    expect(append.children.length).toEqual(4);

    setOriginalsFromValuesForNode(append);

    target = addFromTemplate(append, 'group', {foo: 'fifth', bar: 'element'});
    assert(target instanceof HTMLElement);
    groupSettingsAdd(append, target);

    expect(append.children.length).toEqual(5);

    const last = /** @type {HTMLFieldSetElement | null} */
        (append?.lastElementChild);
    assert(last);

    const foo = last.querySelector('input[name="foo"]');
    assert(foo instanceof HTMLInputElement);

    // already 'changed' by the event handler
    expect(isChangedElement(foo)).toBe(true);
    setInputValue(foo, 'pending value');

    const form = /** @type {HTMLFormElement | null} */
        (document.getElementById(formId));
    assert(form instanceof HTMLFormElement);

    let request = makeDataRequest([form]);
    expect(request.del)
        .toEqual([]);
    expect(request.set)
        .toEqual({
            bar4: 'element',
            foo4: 'pending value',
        });

    groupSettingsDel(append, last);
    expect(append.children.length).toEqual(4);

    request = makeDataRequest([form]);

    expect(request.del)
        .toEqual([]);
    expect(request.set)
        .toEqual({});

    expect(pendingChanges(form)).toBe(false);
});

test('settings group remove', () => {
    const formId = 'remove';

    document.body.innerHTML += makeFormGroup(formId);
    document.body.innerHTML += TEMPLATE_GROUP;

    const remove = document.getElementById('remove-group');
    assert(remove instanceof HTMLDivElement);

    addFromTemplate(remove, 'group', {foo: '1111111'});
    addFromTemplate(remove, 'group', {foo: '2222222', bar: 'foobarfoo'});
    addFromTemplate(remove, 'group', {foo: '3333333', bar: 'barfoobar'});
    addFromTemplate(remove, 'group', {foo: '4444444'});
    expect(remove.children.length).toBe(4);

    setOriginalsFromValuesForNode(remove);

    const second = remove.children[1];
    assert(second instanceof HTMLFieldSetElement);

    const form = document.getElementById('remove');
    assert(form instanceof HTMLFormElement);

    let request = makeDataRequest([form]);
    expect(request.del)
        .toEqual([]);
    expect(request.set)
        .toEqual({});

    // until now, all rows are expected to be 'unchanged'
    groupSettingsDel(remove, second);
    expect(remove.children.length)
        .toEqual(3);

    // 2nd row removal should shift indices of 3rd and 4th rows
    request = makeDataRequest([form]);

    expect(request.del)
        .toEqual(['foo3', 'bar3']);
    expect(request.set)
        .toEqual({
            bar1: 'barfoobar',
            bar2: '',
            foo1: 3333333,
            foo2: 4444444,
        });

    // extra row is always at the end. because add event was triggered,
    // make sure that the required fields are in the payload
    let target = addFromTemplate(remove, 'group', {foo: '5555555'});
    assert(target instanceof HTMLElement);
    groupSettingsAdd(remove, target);

    // since the new row is on top of the removed one, no need to erase it
    request = makeDataRequest([form]);

    expect(request.del)
        .toEqual([]);
    expect(request.set)
        .toEqual({
            bar1: 'barfoobar',
            bar2: '',
            foo1: 3333333,
            foo2: 4444444,
            foo3: 5555555,
        });

    target = addFromTemplate(remove, 'group', {foo: '6666666', bar: 'yyyyyyy'});
    assert(target instanceof HTMLElement);
    groupSettingsAdd(remove, target);

    const last = remove?.lastElementChild;
    assert(last instanceof HTMLFieldSetElement);

    const bar = last.querySelector('input[name=bar]');
    assert(bar instanceof HTMLInputElement);

    setChangedElement(bar);

    const first = remove.children[0];
    assert(first instanceof HTMLFieldSetElement);

    expect(remove.children.length)
        .toEqual(5);

    groupSettingsDel(remove, first);
    expect(remove.children.length)
        .toEqual(4);

    // substituted row keys should no longer be in del set
    // resulting data is effectively every element present
    request = makeDataRequest([form]);

    expect(request.del)
        .toEqual([]);
    expect(request.set)
        .toEqual({
            bar0: 'barfoobar',
            bar1: '',
            bar2: '',
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
    request = makeDataRequest([form]);

    expect(request.del)
        .toEqual(expect.arrayContaining([
            'foo0', 'bar0',
            'foo1', 'bar1',
            'foo2', 'bar2',
            'foo3', 'bar3',
        ]));
    expect(request.set)
        .toEqual({});
});

test('settings group schema remove', () => {
    const formId = 'schema-del';

    document.body.innerHTML += `
    <form id="${formId}">
        <div id="${formId}-group" class="settings-group">
        </div>
    </form>`;
    document.body.innerHTML += TEMPLATE_GROUP;

    const group = document.getElementById(`${formId}-group`);
    assert(group instanceof HTMLDivElement);

    const foo = /** @type {HTMLTemplateElement} */
        (document.getElementById('template-group'))
        ?.content?.querySelector('input[name="foo"]');
    assert(foo instanceof HTMLInputElement);

    setGroupCleanup(foo);

    addFromTemplate(group, 'group', {foo: 'asdasdasd'});
    addFromTemplate(group, 'group', {foo: 'foobarfoo', bar: 'barfoobar'});
    addFromTemplate(group, 'group', {foo: 'oneoneone', bar: 'twotwotwo'});
    expect(group.children.length).toEqual(3);

    setOriginalsFromValuesForNode(group);

    const form = group.parentElement;
    assert(form instanceof HTMLFormElement);

    expect(pendingChanges(form)).toBe(false);

    let request = makeDataRequest([form]);
    expect(request.del)
        .toEqual([]);
    expect(request.set)
        .toEqual({});

    while (group.firstElementChild instanceof HTMLFieldSetElement) {
        groupSettingsDel(group, group.firstElementChild);
    }

    expect(pendingChanges(form)).toBe(true);

    request = makeDataRequest([form]);
    expect(request.del.sort())
        .toEqual(['foo0', 'foo1', 'foo2']);
    expect(request.set)
        .toEqual({});

});

test('number inputs without data consistently serialize as nan string', () => {
    const formId = "numbers-and-nan-strings";
    document.body.innerHTML += `
    <form id="${formId}">
        <input name="number:a" type="number">
        <input name="number:b" type="number">
        <input name="number:c" type="number">
        <input name="number:d" type="number">
    </form>
    `;

    const form = document.forms.namedItem(formId);
    assert(form instanceof HTMLFormElement);

    setInputOrSelectValueByKey(form, "number:a", 12345);
    setInputOrSelectValueByKey(form, "number:d", 56789);

    // not in originals storage
    expect(getOriginals(formId))
        .toEqual({
            'number:a': 12345,
            'number:d': 56789,
        });

    // but only when ready for the wire
    const request = makeDataRequest([form], {assumeChanged: true});
    expect(request.del)
        .toEqual([]);
    expect(request.set)
        .toEqual({
            'number:a': 12345,
            'number:b': 'nan',
            'number:c': 'nan',
            'number:d': 56789,
        });
});

test('mixed plain and group element names should not conflict with each other', () => {
    document.body.innerHTML += `
    <div id="mixed-plain-and-group">
        <form id="plain-value-1">
            <fieldset>
                <input name="foo">
                <input name="bar">
                <input name="baz" readonly>
            </fieldset>
        </form>
        <form id="group-value-1">
            <div>
                <fieldset>
                    <input name="foo">
                </fieldset>
                <fieldset>
                    <input name="foo">
                </fieldset>
                <fieldset>
                    <input name="foo">
                </fieldset>
                <fieldset>
                    <input name="foo">
                </fieldset>
                <fieldset>
                    <input name="foo">
                </fieldset>
            </div>
        </form>
        <form id="group-value-2">
            <div>
                <fieldset>
                    <input name="bar">
                </fieldset>
                <fieldset>
                    <input name="bar">
                </fieldset>
                <fieldset>
                    <input name="bar">
                </fieldset>
            </div>
        </form>
        <form id="plain-value-2">
            <span data-key="foo">
            </span>
        </form>
        <form id="plain-value-3">
            <span data-key="bar">
            </span>
        </form>
    </div>
    `;

    const root = document.getElementById('mixed-plain-and-group');
    assert(root instanceof HTMLDivElement);

    /** @type {{[k: string]: string}} */
    const values = {
        'baz': 'for plain elements',
    };

    /**
     * @param {HTMLInputElement} elem
     * @param {number} index
     */
    function updateInput(elem, index) {
        setGroupElement(elem);
        setInputValue(elem, randomString(16));
        setOriginalFromValue(elem);
        values[`${elem.name}${index}`] = elem.value;
    }

    ["#group-value-1", "#group-value-2"]
        .forEach((id) => {
            /** @type {NodeListOf<HTMLDivElement>} */
            (root.querySelectorAll(`${id} div`))
                .forEach(setGroup);

            /** @type {NodeListOf<HTMLFieldSetElement>} */
            (root.querySelectorAll(`${id} fieldset`))
                .forEach(setGroupElements);

            /** @type {NodeListOf<HTMLInputElement>} */
            (root.querySelectorAll(`${id} input`))
                .forEach(updateInput);
        });

    const plain = [
        ['foo', 'plain value'],
        ['bar', 'set only once'],
    ];

    (root.querySelectorAll('#plain-value-1, #plain-value-2, #plain-value-3'))
        .forEach((form) => {
            for (const [name, value] of plain) {
                setInputOrSelectValueByKey(form, name, value);
                setSpanValueByKey(form, name, value);
                values[name] = value;
            }
            setInputOrSelectValueByKey(form, 'baz', values['baz']);
        });

    root.querySelectorAll('span')
        .forEach((elem) => {
            const key = elem.dataset['key'] ?? 'does-not-exist';
            expect(values[key]).toBeDefined();
            expect(elem.textContent)
                .toEqual(values[key]);
        });

    const baz = root.querySelector('input[name="baz"]');
    assert(baz instanceof HTMLInputElement);
    expect(baz.value).toEqual('for plain elements');

    const forms = Array.from(root.querySelectorAll('form'));

    expect(getOriginals(forms.map((x) => x.id)))
        .toEqual(values);

    const request = makeDataRequest(
        forms, { assumeChanged: true });

    expect(request.del)
        .toEqual([]);
    expect(request.set)
        .toEqual(values);
});
