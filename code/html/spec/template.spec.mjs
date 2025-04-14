import { assert, expect, test, beforeAll } from 'vitest';
import { addFromTemplate } from '../src/template.mjs';
import { loadTemplate, mergeTemplate } from '../src/settings/template.mjs';
import { countChangedElements } from '../src/settings/utils.mjs';

beforeAll(async () => {
    document.body.innerHTML += `
    <div id="basic">
    </div>
    <template id="template-basic">
        <div>
            <h1>Hello</h1>
            <p>world</p>
        </div>
    </template>
    `;

    document.body.innerHTML += `
    <form id="basic-cfg">
    </form>
    <template id="template-basic-cfg">
        <fieldset>
            <legend>Some input and text fields</legend>
            <input name="foo" type="text"></input>
            <span data-key="bar"></span>
            <span data-key="template-id" data-pre="#"></span>
        </fieldset>
    </template>
    `;

    document.body.innerHTML += `
    <template id="template-number-input">
        <div class="pure-control-group">
            <label></label>
            <input type="number" min="0">
        </div>
    </template>
    `;
});

test('basic template fragment', () => {
    const container = document.getElementById('basic');
    assert(container);

    expect(container.children.length)
        .toBe(0);

    const times = 10;

    for (let count = 0; count < times; ++count) {
        const fragment = loadTemplate('basic');
        mergeTemplate(container, fragment);
    }

    expect(container.children.length)
        .toBe(times);

    for (let count = 0; count < times; ++count) {
        const child = /** @type {HTMLElement | null} */
            (container.children[count]);
        assert(child);

        expect(child.childElementCount)
            .toBe(2);

        expect(child.children[0].tagName)
            .toBe('H1');
        expect(child.children[0].textContent)
            .toBe('Hello');

        expect(child.children[1].tagName)
            .toBe('P');
        expect(child.children[1].textContent)
            .toBe('world');
    }
});

test('template fragment with config', () => {
    const container = document.getElementById('basic-cfg');
    assert(container);

    expect(container.children.length)
        .toBe(0);

    const cfgs = [
        {foo: 'first', bar: 12321},
        {foo: 'second', bar: 'datadatadata'},
        {foo: 'third', bar: false},
    ];

    cfgs.forEach((cfg, index) => {
        addFromTemplate(container, 'basic-cfg', cfg);

        const last = /** @type {HTMLElement | null} */
            (container.lastElementChild);
        assert(last);

        const foo = /** @type {HTMLInputElement | null} */
            (last.querySelector('input[name=foo]'));
        assert(foo);
        expect(foo.value)
            .toBe(cfg.foo.toString());

        const bar = /** @type {HTMLSpanElement | null} */
            (last.querySelector('span[data-key=bar]'));
        assert(bar);
        expect(bar.textContent)
            .toBe(cfg.bar.toString());

        const id = /** @type {HTMLSpanElement | null} */
            (last.querySelector('span[data-key=template-id]'));
        assert(id);
        expect(id.textContent)
            .toBe(`#${index}`);
    });

    expect(container.children.length)
        .toEqual(cfgs.length);
    expect(countChangedElements(container))
        .toEqual(0);
});
