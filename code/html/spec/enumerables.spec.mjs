import { assert, afterAll, expect, test } from 'vitest';
import {
    addEnumerables,
    addSimpleEnumerables,
    getEnumerables,
    listenEnumerable,
    listenEnumerableTarget,
    setSpanValue,
} from '../src/settings.mjs';

import {
    randomString,
} from '../src/core.mjs';

/** @import { EnumerableNames } from '../src/settings.mjs' */

afterAll(() => {
    document.body.innerHTML = '';
    expect(document.body.childElementCount)
        .toEqual(0);
});

test('enumerables for a select', () => {
    document.body.innerHTML += `
    <div id="enumerables-for-select">
        <select id="select-data" data-enumerable="select-data">
        </select>
    </div>`;

    const name = 'select-data';
    const number = 16;

    const select = document.querySelector(`select#${name}`);
    assert(select instanceof HTMLSelectElement);
    expect(select.children.length).toEqual(0);
    listenEnumerable(select);

    addSimpleEnumerables(name, name, number);
    expect(select.children.length)
        .toEqual(number + 1);

    /** @type { EnumerableNames } */
    const names =
        Array.from(select.children)
        .slice(1)
        .map((option) => {
            assert(option instanceof HTMLOptionElement);
            return [
                option.value,
                option.innerHTML,
            ];
        })
        .reduce((prev, it) =>
            ({...prev, [it[0]]: it[1]}), {});

    expect(names)
        .toEqual(getEnumerables(name));
});

test('enumerables for a span', () => {
    document.body.innerHTML += `
    <div id="enumerables-for-span">
    </div>`;

    const name = 'span-data';
    const number = 10;

    const container = document.getElementById('enumerables-for-span');
    assert(container instanceof HTMLDivElement);

    for (let index = 0; index < number; ++index) {
        listenEnumerableTarget(container, index, name);
    }

    let spans = container.querySelectorAll('span');
    expect(spans.length).toEqual(number);

    /** @type {EnumerableNames} */
    const names = {};

    spans.forEach((span, index) => {
        expect(span.innerHTML.length)
            .toEqual(0);
        names[index.toString()] = randomString(16);
    });

    addEnumerables(name, names);
    expect(getEnumerables(name))
        .toEqual(names);

    expect(spans.length).toEqual(number);

    /**
     * @param {number} number
     */
    function expectSpanInnerHTML(number) {
        const other = getEnumerables(name);
        for (let index = 0; index < number; ++index) {
            expect(spans[index].innerHTML)
                .toEqual(other[index.toString()]);
        }
    }

    expectSpanInnerHTML(number);

    const override = `${name.toUpperCase()}${name.toUpperCase} #${number}`;
    listenEnumerableTarget(
        container, number, name,
        (elem, _entries) => {
            assert(elem instanceof HTMLSpanElement);
            setSpanValue(elem, override);
        });

    spans = container.querySelectorAll('span');
    expect(spans.length).toEqual(number + 1);

    addSimpleEnumerables(name, name, number + 1);

    expectSpanInnerHTML(number);
    expect(spans[number].innerHTML)
        .toEqual(override);
});

test('enumerables generated from a range', () => {
    const name = 'range';
    const number = 8;

    const ids = Array.from({length: number}, (_, index) => index.toString());

    expect(getEnumerables(name))
        .toEqual({});
    addSimpleEnumerables(name, name, number);
    expect(Object.keys(getEnumerables(name)))
        .toEqual(ids);
});
