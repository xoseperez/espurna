import { expect, test } from 'vitest';

import {
    parseCommand,
    pictureFor,
    pictureNames,
    quantize,
    rgb565,
    shownAtLoad,
    themeColour,
    splitArgs,
    swipeOf,
    unescapeText,
} from '../src/nspanel-screen.mjs';
import { screenUnsupported } from '../src/nspanel.mjs';

test('screen: commands as the firmware writes them', () => {
    expect(parseCommand('page 7')).toEqual({op: 'page', page: 7});
    expect(parseCommand('vis busy1,0')).toEqual({op: 'vis', name: 'busy1', on: false});
    expect(parseCommand('vis 255,0')).toEqual({op: 'vis', name: '255', on: false});
    expect(parseCommand('tile3_name.txt="Кухня \\"1\\"\\rx"'))
        .toEqual({op: 'attr', name: 'tile3_name', attr: 'txt', value: 'Кухня "1"\nx'});
    expect(parseCommand('tile3_pic.pic=265')).toEqual({op: 'attr', name: 'tile3_pic', attr: 'pic', value: 265});
    expect(parseCommand('dim=80')).toEqual({op: 'var', name: 'dim', value: 80});
    expect(parseCommand('fill 10,20,30,40,65535')).toEqual({op: 'draw', name: 'fill', args: [10, 20, 30, 40, 65535]});
    expect(parseCommand('xstr 1,2,3,4,0,65535,0,1,1,3,"a, b"'))
        .toEqual({op: 'draw', name: 'xstr', args: [1, 2, 3, 4, 0, 65535, 0, 1, 1, 3, 'a, b']});
    expect(parseCommand('#reset')).toEqual({op: 'marker', name: 'reset'});
    expect(parseCommand('ref_stop')).toEqual({op: 'other'});
});

test('screen: text and arguments', () => {
    expect(unescapeText('a\\\\b\\"c\\rd')).toEqual('a\\b"c\nd');
    expect(splitArgs('1,2,"x"')).toEqual([1, 2, 'x']);
    expect(rgb565(65535)).toEqual('rgb(255,255,255)');
    expect(rgb565(0xF800)).toEqual('rgb(255,0,0)');
    expect(rgb565(0)).toEqual('rgb(0,0,0)');
});

test('screen: swipes as the TFT reports them', () => {
    expect(swipeOf(150, 0)).toEqual('right');
    expect(swipeOf(-150, 0)).toEqual('left');
    expect(swipeOf(0, 120)).toEqual('down');
    expect(swipeOf(10, -120)).toEqual('up');
    expect(swipeOf(60, 60)).toEqual('');
});

test('screen: picture ids of the manifest', () => {
    const manifest = {
        backgrounds: [{pic: 291}],
        pictures: {candy: {button1: {off: 264, on: 265}, close: {x: 288}, rowcard1: {box: 296}, weather_home: {rain: 341}}},
    };
    const names = pictureNames(manifest);
    expect(names.get(291)).toEqual('background');
    expect(names.get(265)).toEqual('button1:on');
    expect(names.get(288)).toEqual('close:x');
    expect(names.get(341)).toEqual('weather_home:rain');

    expect(pictureFor('button1:on')).toEqual({picture: 'tile:on'});
    expect(pictureFor('button8:off')).toEqual({picture: 'tile:off'});
    expect(pictureFor('close:x')).toEqual({picture: 'close:x'});
    expect(pictureFor('weather_home:rain')).toEqual({picture: 'weather:rain'});
    expect(pictureFor('rowcard1:box')).toEqual({box: 10597});
    expect(pictureFor('badge1:red')).toEqual({badge: 'red'});
    expect(pictureFor('logo:boot')).toEqual({});
});

test('screen: what a page does itself when it loads', () => {
    // most pages: everything hidden but the title and the close cross; home: everything
    expect(shownAtLoad('thermostat', {n: 'title', t: 116}, false)).toEqual(true);
    expect(shownAtLoad('thermostat', {n: 'now', t: 116}, false)).toEqual(false);
    expect(shownAtLoad('home', {n: 'clock', t: 116}, false)).toEqual(false);
    expect(shownAtLoad('buttons', {n: 'close_hs', t: 109}, false)).toEqual(true);
    expect(shownAtLoad('buttons', {n: 'title', t: 116}, false)).toEqual(false);
    // pages that keep their objects hide the pictures the firmware sets, not the cross
    expect(shownAtLoad('settings', {n: 'set1_text', t: 116}, false)).toEqual(true);
    expect(shownAtLoad('settings', {n: 'restart_pic', t: 112}, true)).toEqual(false);
    expect(shownAtLoad('settings', {n: 'close', t: 112}, true)).toEqual(true);
    expect(shownAtLoad('settings', {n: 'busy1', t: 116}, false)).toEqual(false);

    expect(themeColour('home', 'date')).toEqual('th_dim');
    expect(themeColour('home', 'high_arrow3')).toEqual('th_dim');
    expect(themeColour('home', 'info2_text')).toEqual('th_text');
    expect(themeColour('home', 'clock')).toEqual('');
    expect(themeColour('weather', 'low_arrow4')).toEqual('th_dim');
    expect(themeColour('settings', 'title')).toEqual('th_text');
});

test('screen: pictures in the display colours', () => {
    expect(quantize(44, 5)).toEqual(41);
    expect(quantize(44, 6)).toEqual(44);
    expect(quantize(255, 5)).toEqual(255);
    expect(quantize(0, 6)).toEqual(0);
});

test('screen: only the OHpanel stock design', () => {
    expect(screenUnsupported({driver: 'ohpanel', version: 'bc629e1e'})).toEqual('');
    expect(screenUnsupported({driver: 'lovelace', version: 'x'})).not.toEqual('');
    expect(screenUnsupported({driver: 'ohpanel', version: 'd3731fbb'})).not.toEqual('');
});
