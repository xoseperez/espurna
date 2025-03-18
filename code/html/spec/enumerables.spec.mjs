import { assert, afterAll, expect, test } from 'vitest';
import {
    addEnumerables,
    addSimpleEnumerables,
    getEnumerables,
    listenEnumerable,
    listenEnumerableTarget,
    setSpanValue,
} from '../src/settings.mjs';

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

    const results =
        Array.from(select.children)
        .slice(1)
        .map((entry) => {
            assert(entry instanceof HTMLOptionElement);
            return {
                "id": parseInt(entry.value),
                "name": entry.innerHTML,
            };
        });

    const enumerables = getEnumerables(name);
    expect(enumerables.length)
        .toEqual(number);
    expect(enumerables)
        .toEqual(results);
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

    spans.forEach((span) => {
        expect(span.innerHTML.length)
            .toEqual(0);
    });

    addSimpleEnumerables(name, name, number);
    expect(spans.length).toEqual(number);

    /**
     * @param {number} number
     */
    function expectNumberedLabels(number) {
        const enumerables = getEnumerables(name);
        expect(enumerables.length).toBeGreaterThanOrEqual(number);

        for (let index = 0; index < number; ++index) {
            const found = enumerables.filter((x) => x.id == index);
            expect(found.length).toEqual(1);

            const text = spans[index].innerHTML;
            expect(text).toEqual(found[0].name);
        }
    }

    /**
     * @param {number} number
     */
    function extraLabelString(number) {
        return `${name.toUpperCase()}${name.toUpperCase} #${number}`;
    }

    expectNumberedLabels(number);

    listenEnumerableTarget(
        container, number, name,
        (elem, _entries) => {
            assert(elem instanceof HTMLSpanElement);
            setSpanValue(elem, extraLabelString(number + 1));
        });

    spans = container.querySelectorAll('span');
    expect(spans.length).toEqual(number + 1);

    addSimpleEnumerables(name, name, number + 1);

    expectNumberedLabels(number);
    expect(spans[number].innerHTML)
        .toEqual(extraLabelString(number + 1));
});

test('enumerables generated from a range', () => {
    const name = 'range';
    const label = name.toUpperCase();

    const expected = [
        {id: 0, name: `${label} #0`},
        {id: 1, name: `${label} #1`},
        {id: 2, name: `${label} #2`},
        {id: 3, name: `${label} #3`},
        {id: 4, name: `${label} #4`},
    ];

    expect(getEnumerables(name).length)
        .toBe(0);
    addSimpleEnumerables(
        name, label, expected.length);
    expect(getEnumerables(name).length)
        .toBe(expected.length);

    addEnumerables('expected', expected);
    expect(getEnumerables('expected'))
        .toEqual(getEnumerables(name));
});

