import { expect, test } from 'vitest';

import {
    applyField,
    formatStatus,
    formatValue,
    itemKind,
    KIND_FIELDS,
    KINDS,
    parsePath,
    setItemKind,
    formatValidateResult,
    itemContains,
    itemName,
    jsonErrorOffset,
    moveItem,
    newPage,
    offsetToLineColumn,
    pageGrid,
    perScreen,
    previewCards,
    previewDesignGrids,
    previewGrid,
    previewRow,
    previewSlots,
    previewTarget,
    renameItem,
    TEMPLATES,
    validateJson,
} from '../src/nspanel.mjs';

test('preview splits long pages like the panel', () => {
    const items = Array.from({length: 9}, (_, index) => ({name: `t${index}`, topic: `x/${index}`}));
    const cards = previewCards({pages: [
        {title: 'Heating', items},
        {title: 'Short', items: items.slice(0, 2)},
        {title: 'Setup', type: 'settings'},
    ]});

    expect(cards.map((card) => card.title))
        .toEqual(['Heating 1/2', 'Heating 2/2', 'Short', 'Setup']);
    expect(cards[0].items.length).toEqual(8);
    expect(cards[1].items.length).toEqual(1);
});

test('preview: 3 columns put 6 tiles a screen in slots 1-3 and 5-7', () => {
    const rooms = Array.from({length: 9}, (_, index) => ({name: `r${index}`, type: 'thermostat', topic: 't'}));
    const cards = previewCards({pages: [{title: 'Heating', columns: 3, items: rooms}]});

    expect(cards.map((card) => card.title)).toEqual(['Heating 1/2', 'Heating 2/2']);
    expect(cards.map((card) => card.items.length)).toEqual([6, 3]);
    expect(previewSlots(cards[0]).map((item) => item?.name ?? null))
        .toEqual(['r0', 'r1', 'r2', null, 'r3', 'r4', 'r5', null]);
    expect(previewSlots(cards[1]).map((item) => item?.name ?? null))
        .toEqual(['r6', 'r7', 'r8', null, null, null, null, null]);
});

test('preview: per_screen splits explicitly, bad values are ignored', () => {
    const items = Array.from({length: 9}, (_, index) => ({name: `t${index}`, topic: `x/${index}`}));
    expect(perScreen({per_screen: 4})).toEqual(4);
    expect(perScreen({columns: 3, per_screen: 8})).toEqual(6);
    expect(perScreen({columns: 3, per_screen: 2})).toEqual(2);
    expect(perScreen({per_screen: 0})).toEqual(8);
    expect(perScreen({per_screen: 9, columns: 5})).toEqual(8);
    expect(perScreen({}, 6)).toEqual(6);

    const cards = previewCards({pages: [{title: 'P', per_screen: 4, items}]});
    expect(cards.map((card) => card.items.length)).toEqual([4, 4, 1]);
    expect(cards[2].title).toEqual('P 3/3');
    expect(previewSlots(cards[0]).map((item) => item?.name ?? null))
        .toEqual(['t0', 't1', 't2', 't3', null, null, null, null]);
});

test('preview: list pages are 7 rows a sheet, grids on Lovelace', () => {
    const items = Array.from({length: 9}, (_, index) => ({name: `t${index}`, topic: `x/${index}`}));
    const config = {pages: [{title: 'Yard', type: 'list', columns: 3, items}]};

    const cards = previewCards(config);
    expect(cards.map((card) => card.title)).toEqual(['Yard 1/2', 'Yard 2/2']);
    expect(cards.map((card) => card.items.length)).toEqual([7, 2]);
    expect(cards.every((card) => card.list)).toBe(true);
    expect(previewCards({pages: [{type: 'list', per_screen: 5, items}]}).map((card) => card.items.length))
        .toEqual([5, 4]);

    const lovelace = previewCards(config, 8, false);
    expect(lovelace.map((card) => card.list)).toEqual([false, false]);
    expect(lovelace.map((card) => card.items.length)).toEqual([6, 3]);
});

test('preview: layout picks the grid page of the custom TFT and its capacity', () => {
    const rooms = Array.from({length: 9}, (_, index) => ({name: `r${index}`, type: 'thermostat', topic: 't'}));
    /** @param {number} count */
    const tiles = (count) => rooms.slice(0, count);

    expect(pageGrid({layout: '3x3', columns: 4})).toEqual('3x3');
    expect(pageGrid({columns: 3})).toEqual('3x2');
    expect(pageGrid({layout: '5x5'})).toEqual('4x2');
    expect(['4x2', '3x3', '3x2', '2x2', '1x2'].map((layout) => perScreen({layout}, 8, false, true)))
        .toEqual([8, 9, 6, 4, 2]);
    expect(perScreen({layout: '3x3', per_screen: 7}, 8, false, true)).toEqual(7);
    expect(perScreen({layout: '2x2', per_screen: 9}, 8, false, true)).toEqual(4);

    const heating = previewCards({pages: [{title: 'Heating', layout: '3x3', items: rooms}]}, 8, true, true);
    expect(heating.map((card) => card.title)).toEqual(['Heating']);
    expect(previewGrid(heating[0], true)).toEqual({columns: 3, slots: rooms, indexes: [0, 1, 2, 3, 4, 5, 6, 7, 8]});

    const big = previewCards({pages: [{title: 'Big', layout: '2x2', items: tiles(5)}]}, 8, true, true);
    expect(big.map((card) => card.title)).toEqual(['Big 1/2', 'Big 2/2']);
    expect(previewGrid(big[1], true).columns).toEqual(2);
    expect(previewGrid(big[1], true).slots.map((item) => item?.name ?? null)).toEqual(['r4', null, null, null]);

    const wide = previewCards({pages: [{title: 'Wide', layout: '1x2', items: tiles(3)}]}, 8, true, true);
    expect(wide.map((card) => card.items.length)).toEqual([2, 1]);
    expect(previewGrid(wide[0], true)).toEqual({columns: 1, slots: tiles(2), indexes: [0, 1]});
    expect(previewGrid(big[1], true).indexes).toEqual([0, -1, -1, -1]);
    expect(big.map((card) => card.offset)).toEqual([0, 4]);
});

test('preview: layouts on the stock TFT use the columns of the 4x2 slots', () => {
    const rooms = Array.from({length: 9}, (_, index) => ({name: `r${index}`, type: 'thermostat', topic: 't'}));
    /** @param {import('../src/nspanel.mjs').PreviewCard} card */
    const names = (card) => previewSlots(card).map((item) => item?.name ?? null);

    expect(['4x2', '3x3', '3x2', '2x2', '1x2'].map((layout) => perScreen({layout})))
        .toEqual([8, 6, 6, 4, 2]);

    const heating = previewCards({pages: [{title: 'Heating', layout: '3x3', items: rooms}]});
    expect(heating.map((card) => card.items.length)).toEqual([6, 3]);
    expect(names(heating[0])).toEqual(['r0', 'r1', 'r2', null, 'r3', 'r4', 'r5', null]);

    const big = previewCards({pages: [{title: 'Big', layout: '2x2', items: rooms.slice(0, 4)}]});
    expect(names(big[0])).toEqual(['r0', 'r1', null, null, 'r2', 'r3', null, null]);

    const wide = previewCards({pages: [{title: 'Wide', layout: '1x2', items: rooms.slice(0, 2)}]});
    expect(names(wide[0])).toEqual(['r0', null, null, null, 'r1', null, null, null]);
    expect(previewGrid(wide[0]).columns).toEqual(4);

    // Lovelace: no grid pages either, the same capacity
    expect(previewCards({pages: [{layout: '2x2', items: rooms}]}, 8, false).map((card) => card.items.length))
        .toEqual([4, 4, 1]);
});

test('preview follows the design the panel reports', () => {
    expect(previewDesignGrids({driver: 'ohpanel', version: 'd3731fbb'}, false)).toBe(true);
    expect(previewDesignGrids({driver: 'ohpanel', version: 'bc629e1e'}, false)).toBe(true);
    expect(previewDesignGrids({driver: 'ohpanel', version: '4508cd3f'}, false)).toBe(true);
    expect(previewDesignGrids({driver: 'ohpanel', version: '0badc0de'}, true)).toBe(false);
    expect(previewDesignGrids({driver: 'ohpanel', version: 'unknown'}, true)).toBe(true);
    expect(previewDesignGrids({driver: 'lovelace', version: '53'}, true)).toBe(true);
    expect(previewDesignGrids(null, false)).toBe(false);
});

test('preview: nested pages follow their tile, outside the swipe row', () => {
    const lamp = (/** @type {number} */ index) => ({name: `l${index}`, topic: `x/${index}`});
    const config = {pages: [
        {title: 'Street', layout: '2x2', items: [
            {name: 'Front', icon: 'home-outline', page: {layout: '3x2', items: [lamp(1), lamp(2)]}},
            {icon: 'tree', page: {title: 'Back', layout: '3x2', items: [
                lamp(3),
                {name: 'Sockets', page: {layout: '2x2', items: [lamp(4), lamp(5), lamp(6), lamp(7), lamp(8)]}},
            ]}},
            {name: 'Gate', icon: 'gate', topic: 'g/0', type: 'button'},
        ]},
        {title: 'Heating', items: [lamp(9)]},
    ]};

    const cards = previewCards(config, 8, true, true);
    expect(cards.map((card) => [card.title, card.depth, card.grid])).toEqual([
        ['Street', 0, '2x2'],
        ['Front', 1, '3x2'],
        ['Back', 1, '3x2'],
        ['Sockets 1/2', 2, '2x2'],
        ['Sockets 2/2', 2, '2x2'],
        ['Heating', 0, '4x2'],
    ]);
    expect(previewRow(config, 8, true, true)).toEqual(['Street', 'Heating']);

    const street = config.pages[0].items;
    expect(itemName(street[0])).toEqual('Front');
    expect(itemName(street[1])).toEqual('Back');
    expect(previewTarget(street[1], {})).toEqual({type: 'page', target: 'page Back', bad: false});
    expect(previewTarget({name: 'x', page: 2}, {}).bad).toBe(true);
});

test('preview flags tiles without a target', () => {
    const groups = {house: []};
    expect(previewTarget({type: 'scene', group: 'house'}, groups).bad).toBe(false);
    expect(previewTarget({type: 'scene', group: 'nope'}, groups).bad).toBe(true);
    expect(previewTarget({name: 'x'}, groups).bad).toBe(true);
    expect(previewTarget({relay: 0}, groups).target).toEqual('relay 0');
    expect(previewTarget('Кухня', groups).type).toEqual('ref');
});

test('offset to line and column', () => {
    const text = '{\n  "a": 1,\n  "b": x\n}';
    expect(offsetToLineColumn(text, 0)).toEqual({line: 1, column: 1});
    expect(offsetToLineColumn(text, text.indexOf('x'))).toEqual({line: 3, column: 8});
});

test('valid json object', () => {
    const result = validateJson('{"pages": []}');
    expect(result.ok).toBeTruthy();
    expect(formatValidateResult(result)).toEqual('JSON is valid');
});

test('json must be an object', () => {
    for (const text of ['[]', '1', 'null', '   ']) {
        expect(validateJson(text).ok).toBeFalsy();
    }
});

test('syntax error has line and column', () => {
    const text = '{\n  "a": 1,\n  "b": x\n}';
    const result = validateJson(text);
    expect(result.ok).toBeFalsy();
    if (result.ok) {
        return;
    }

    expect(result.line).toEqual(3);
    expect(result.column).toEqual(8);
    expect(result.offset).toEqual(text.indexOf('x'));
    expect(formatValidateResult(result)).toMatch(/^Line 3, column 8: /);
});

test('truncated json points at the end', () => {
    const text = '{\n  "a": [1, 2';
    const result = validateJson(text);
    expect(result.ok).toBeFalsy();
    if (result.ok) {
        return;
    }

    expect(result.line).toEqual(2);
});

test('error offset scanner', () => {
    const valid = [
        '{}', '[]', '{"a": [1, -2.5e3, true, false, null, "x\\"\\u00e9"], "b": {"c": {}}}',
        ' \n{ "a" : 0 }\n ',
    ];
    for (const text of valid) {
        expect(jsonErrorOffset(text)).toEqual(-1);
        expect(() => JSON.parse(text)).not.toThrow();
    }

    /** @type {[string, number][]} */
    const invalid = [
        ['{"a": 1,}', 8],
        ['{"a" 1}', 5],
        ['{a: 1}', 1],
        ['[1 2]', 3],
        ['{"a": 01}', 7],
        ['{"a": "x\ny"}', 8],
        ['{"a": 1} x', 9],
        ['{"a": tru}', 6],
    ];
    for (const [text, offset] of invalid) {
        expect(() => JSON.parse(text)).toThrow();
        expect(jsonErrorOffset(text)).toEqual(offset);
    }
});

test('status line', () => {
    expect(formatStatus(null)).toEqual('?');
    expect(formatStatus({
        display: 'auto', driver: 'lovelace', connected: true,
        probing: false, version: '53', model: 'NSPanel', flashing: false,
    })).toEqual('lovelace, (config: auto), model NSPanel, version 53, connected');

    expect(formatStatus({
        display: 'auto', driver: 'ohpanel', connected: true, version: 'd3731fbb',
        design: 'custom ESPurna grids: layouts 4x2 3x3 3x2 2x2 1x2, background aurora only',
    })).toEqual('ohpanel, (config: auto), version d3731fbb, '
        + 'custom ESPurna grids: layouts 4x2 3x3 3x2 2x2 1x2, background aurora only, connected');
});

test('editor: an item takes the place of the one it is dropped on', () => {
    const list = ['a', 'b', 'c', 'd'];
    expect(moveItem(list, 0, list, 2)).toEqual(true);
    expect(list).toEqual(['b', 'c', 'a', 'd']);
    expect(moveItem(list, 3, list, 0)).toEqual(true);
    expect(list).toEqual(['d', 'b', 'c', 'a']);
    expect(moveItem(list, 1, list, 1)).toEqual(false);
    expect(moveItem(list, 7, list, 0)).toEqual(false);

    // past the end appends, also into another list
    const other = ['x'];
    expect(moveItem(list, 0, other, Number.MAX_SAFE_INTEGER)).toEqual(true);
    expect(list).toEqual(['b', 'c', 'a']);
    expect(other).toEqual(['x', 'd']);
});

test('editor: a page tile never goes into its own nested pages', () => {
    const inner = {name: 'Inner', page: {items: [{name: 'leaf', topic: 't'}]}};
    const outer = {name: 'Outer', page: {items: [inner]}};
    const top = [outer];

    expect(itemContains(outer, inner.page.items)).toEqual(true);
    expect(itemContains(outer, outer.page.items)).toEqual(true);
    expect(itemContains(inner, outer.page.items)).toEqual(false);
    expect(moveItem(top, 0, inner.page.items, 0)).toEqual(false);
    expect(top).toEqual([outer]);

    // the other way round is fine: the inner page tile moves up a level
    expect(moveItem(outer.page.items, 0, top, 1)).toEqual(true);
    expect(top.map(itemName)).toEqual(['Outer', 'Inner']);
});

test('editor: rename', () => {
    const list = [{name: 'Lamp', topic: 't'}, 'ref', {page: {title: 'Floor', items: []}, name: 'x'}];
    renameItem(list, 0, 'Desk');
    renameItem(list, 1, 'other');
    renameItem(list, 2, '');
    renameItem(list, 1, '');
    expect(list.map(itemName)).toEqual(['Desk', 'other', 'Floor']);
    expect(list[2]).not.toHaveProperty('name');
});

test('editor: palette templates make fresh items the preview knows', () => {
    for (const template of Object.values(TEMPLATES)) {
        const one = template.make();
        expect(one).not.toBe(template.make());
        expect(itemName(one).length).toBeGreaterThan(0);
        expect(previewTarget(one, {}).type.length).toBeGreaterThan(0);
    }
    expect(previewTarget(TEMPLATES.subpage.make(), {}).bad).toEqual(false);
    expect(previewTarget(TEMPLATES.switch.make(), {}).bad).toEqual(true);
    expect(previewCards({pages: [newPage()]}).map((card) => card.title)).toEqual(['New page']);
});

test('settings form: what a tile does', () => {
    expect(itemKind({name: 'a', topic: 't'})).toEqual('switch');
    expect(itemKind({type: 'button', topic: 't'})).toEqual('button');
    expect(itemKind({relay: 0})).toEqual('relay');
    expect(itemKind({type: 'scene', group: 'g'})).toEqual('scene');
    expect(itemKind({page: {items: []}})).toEqual('page');
    expect(itemKind({open: 'settings'})).toEqual('open');
    expect(itemKind({type: 'thermostat', topic: 't'})).toEqual('thermostat');
    expect(itemKind({type: 'nonsense', topic: 't'})).toEqual('switch');
    expect(itemKind('Lamp')).toEqual('ref');
    expect(Object.keys(KIND_FIELDS).sort()).toEqual(Object.keys(KINDS).sort());
});

test('settings form: another kind keeps what fits and adds what is needed', () => {
    const lamp = {name: 'Lamp', icon: 'lamp', topic: 'z/Lamp', path: ['state'], on: 'ON', off: 'OFF', font: 3};
    expect(setItemKind(lamp, 'button')).toEqual({name: 'Lamp', icon: 'lamp', type: 'button', topic: 'z/Lamp', path: ['state'], on: 'ON'});
    expect(setItemKind(lamp, 'scene')).toEqual({name: 'Lamp', icon: 'lamp', type: 'scene', group: '', action: 'toggle'});
    expect(setItemKind(lamp, 'relay')).toEqual({name: 'Lamp', icon: 'lamp', relay: 0});
    expect(setItemKind(lamp, 'page')).toEqual({name: 'Lamp', icon: 'lamp', page: {items: []}});
    expect(setItemKind(lamp, 'open')).toEqual({name: 'Lamp', icon: 'lamp', open: 'settings'});
    expect(setItemKind(lamp, 'switch')).toEqual({name: 'Lamp', icon: 'lamp', topic: ''});
    expect(setItemKind({name: 'T', topic: 'istabai/out', room: 'Hall', step: 0.5}, 'thermostat'))
        .toEqual({name: 'T', topic: 'istabai/out', room: 'Hall', step: 0.5, type: 'thermostat'});
    for (const kind of Object.keys(KINDS)) {
        expect(itemKind(setItemKind({name: 'x', topic: 't'}, kind))).toEqual(kind);
    }
});

test('settings form: values', () => {
    expect(parsePath('Гостиная, set_temperature')).toEqual(['Гостиная', 'set_temperature']);
    expect(parsePath('list, 0 ,, value')).toEqual(['list', 0, 'value']);
    expect(formatValue(['a', 0])).toEqual('a, 0');
    expect(formatValue(undefined)).toEqual('');
    expect(formatValue(1.5)).toEqual('1.5');

    /** @type {any} */
    const item = {topic: 'x', step: 1};
    applyField(item, 'topic', 'text', '  z/Lamp ');
    applyField(item, 'step', 'number', '0.5');
    applyField(item, 'relay', 'number', 'abc');
    applyField(item, 'min', 'numpath', '5');
    applyField(item, 'max', 'numpath', 'room, max');
    applyField(item, 'open', 'numtext', '2');
    applyField(item, 'draw', 'bool', 'false');
    applyField(item, 'path', 'path', 'state');
    expect(item).toEqual({topic: 'z/Lamp', step: 0.5, min: 5, max: ['room', 'max'], open: 2, draw: false, path: ['state']});

    applyField(item, 'open', 'numtext', 'Дом');
    applyField(item, 'topic', 'text', '');
    expect(item.open).toEqual('Дом');
    expect(item).not.toHaveProperty('topic');
});

test('settings form: group members, a topic or a relay number a line', () => {
    /** @type {any} */
    const groups = {house: ['a/relay/0']};
    applyField(groups, 'house', 'members', 'ESPURNA-KITCHEN/relay/0\n  2 \n\nzigbee2mqtt/Lamp\n');
    expect(groups).toEqual({house: ['ESPURNA-KITCHEN/relay/0', 2, 'zigbee2mqtt/Lamp']});
    applyField(groups, 'house', 'members', '  ');
    expect(groups).toEqual({});
});

test('settings form: a tile calling a URL', () => {
    expect(itemKind({name: 'Gate', url: 'http://10.0.0.9/relay/0?turn=on'})).toEqual('url');
    expect(itemKind({name: 'Lamp', on_url: 'http://x/on', off_url: 'http://x/off', topic: 'x/state'})).toEqual('url');

    // a switch becomes a URL tile: the state topic stays, the MQTT command goes
    const lamp = {name: 'Lamp', icon: 'lamp', topic: 'z/Lamp', command: 'z/Lamp/set', payload: '%s'};
    expect(setItemKind(lamp, 'url')).toEqual({name: 'Lamp', icon: 'lamp', topic: 'z/Lamp', url: ''});
    expect(setItemKind({name: 'x', on_url: 'http://a'}, 'url')).toEqual({name: 'x', on_url: 'http://a'});
    expect(setItemKind({name: 'x', url: 'http://a', topic: 't'}, 'button'))
        .toEqual({name: 'x', type: 'button', topic: 't'});

    expect(previewTarget({url: 'http://a/b'}, {})).toEqual({type: 'url', target: 'GET http://a/b', bad: false});
    expect(previewTarget({url: 'ftp://a'}, {}).bad).toEqual(true);
    expect(previewTarget(TEMPLATES.url.make(), {}).bad).toEqual(true);
});

