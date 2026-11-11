import { expect, test } from 'vitest';
import {
    esp8266Route,
    fetchFits,
    fetchOutcome,
    firmwareHeader,
    firmwareRows,
    fits,
    imageCandidates,
    installable,
    miniFiles,
    miniFreeSize,
    roundedSize,
    uploadOutcome,
    stepTo,
    uploadSteps,
} from '../src/fwinstall.mjs';

/**
 * @param {string} text
 * @returns {Uint8Array}
 */
function bytes(text) {
    return new Uint8Array([...text].map((x) => x.charCodeAt(0)));
}

test('image headers of every chip are recognised', () => {
    expect(firmwareHeader(new Uint8Array([0xe9, 0x03, 0x03, 0x20]))).toBe('esp');
    expect(firmwareHeader(new Uint8Array([0x1f, 0x8b, 0x08, 0x00]))).toBe('gzip');
    expect(firmwareHeader(bytes('UF2\nWQ]\x9e'))).toBe('uf2');
    expect(firmwareHeader(bytes('ESPBEKEN'))).toBe('beken');
    expect(firmwareHeader(bytes('ESPMTAIL'))).toBe('matter-tail');
    expect(firmwareHeader(bytes('ESPBEK'))).toBe(null);
    expect(firmwareHeader(bytes('<!DOCTYP'))).toBe(null);
    expect(firmwareHeader(new Uint8Array([]))).toBe(null);
});

test('OTA size is counted in whole sectors', () => {
    expect(roundedSize(1)).toBe(4096);
    expect(roundedSize(4095)).toBe(4096);
    expect(roundedSize(4097)).toBe(8192);

    // /upgrade: the free space minus one sector
    const file = {name: 'x.bin', kind: 'ota', size: 500000, url: 'x.bin'};
    expect(fits(file, 0)).toBe(true);
    expect(fits(file, 507904)).toBe(true);
    expect(fits(file, 503808)).toBe(false);
    expect(fits(file, 499712)).toBe(false);

    // the board itself: one byte more than the file, in whole sectors
    expect(fetchFits(file, 503808)).toBe(true);
    expect(fetchFits(file, 499712)).toBe(false);
    expect(fetchFits({...file, size: 499712}, 499712)).toBe(false);
    expect(fetchFits({...file, size: 499711}, 499712)).toBe(true);
});

test('board answers to the upload', () => {
    expect(uploadOutcome(200, 'OK')).toBe('done');
    expect(uploadOutcome(200, 'OK, again')).toBe('again');
    expect(uploadOutcome(200, 'OK, again\n')).toBe('again');
    expect(uploadOutcome(409, 'ERROR: Matter is running from the OTA partition')).toBe('matter');
    expect(uploadOutcome(400, 'ERROR: No magic byte / invalid flash config')).toBe('error');
    expect(uploadOutcome(200, 'ERROR: something')).toBe('error');
});

const IMAGES = [
    'beken-matter',
    'esp32-matter',
    'esp32-matter-nspanel',
    'esp8266-1m',
    'esp8266-1m-light',
    'esp8266-1m-mini',
    'esp8266-1m-sensor',
    'esp8266-4m',
];

test('running image gets its own files, 1 MB boards the mini one as step 1', () => {
    expect(imageCandidates(IMAGES, 'esp32-matter', 'esp32'))
        .toEqual([{image: 'esp32-matter'}]);
    expect(imageCandidates(IMAGES, 'beken-matter', 'beken'))
        .toEqual([{image: 'beken-matter'}]);
    // 4 MB layout: no two-step update, the mini image is a 1 MB one
    expect(imageCandidates(IMAGES, 'esp8266-4m', 'esp8266'))
        .toEqual([{image: 'esp8266-4m'}]);

    const one = imageCandidates(IMAGES, 'esp8266-1m-light', 'esp8266');
    expect(one.map((x) => x.image)).toEqual(['esp8266-1m-light', 'esp8266-1m-mini']);
    expect(one[1].step).toMatch(/step 1/);
});

test('mini image offers the full images of the same flash size', () => {
    const out = imageCandidates(IMAGES, 'esp8266-1m-mini', 'esp8266');
    expect(out.map((x) => x.image)).toEqual(['esp8266-1m', 'esp8266-1m-light', 'esp8266-1m-sensor']);
    expect(out.every((x) => x.step && x.step.includes('step 2'))).toBe(true);
});

test('local build without an image name lists every image of its chip', () => {
    expect(imageCandidates(IMAGES, '', 'esp32').map((x) => x.image))
        .toEqual(['esp32-matter', 'esp32-matter-nspanel']);
    expect(imageCandidates(IMAGES, '', 'esp8266').map((x) => x.image))
        .toEqual(['esp8266-1m', 'esp8266-1m-light', 'esp8266-1m-mini', 'esp8266-1m-sensor', 'esp8266-4m']);
    expect(imageCandidates(IMAGES, '', '').length).toBe(IMAGES.length);
});

test('image the release does not have', () => {
    expect(imageCandidates(['beken-bk7231n', 'esp32'], 'beken-matter', 'beken')).toEqual([]);
});

/** @type {import('../src/fwinstall.mjs').FirmwareManifest} */
const MANIFEST = {
    version: 1,
    repository: 'igorufox/espurna',
    generated: '2026-11-06T12:00:00Z',
    releases: [
        {
            tag: 'esp32-261106-abcdef1',
            date: '2026-11-06T10:00:00Z',
            commit: 'abcdef1234567890abcdef1234567890abcdef12',
            images: {
                'beken-matter': [
                    {name: 'espurna-beken-matter.bin', kind: 'ota', size: 1500000, url: 'esp32-261106-abcdef1/espurna-beken-matter.bin'},
                    {name: 'espurna-beken-initial-tuya.uf2', kind: 'serial', size: 2000000, url: 'esp32-261106-abcdef1/espurna-beken-initial-tuya.uf2'},
                ],
                'esp32-matter': [
                    {name: 'espurna-esp32-ota.bin', kind: 'ota', size: 2100000, url: 'esp32-261106-abcdef1/espurna-esp32-ota.bin'},
                    {name: 'espurna-esp32-full.bin', kind: 'serial', size: 3000000, url: 'esp32-261106-abcdef1/espurna-esp32-full.bin'},
                ],
            },
        },
        {
            tag: 'esp32-261105-1234567',
            date: '2026-11-05T10:00:00Z',
            commit: '1234567',
            images: {
                'beken-bk7231n': [
                    {name: 'espurna-beken-bk7231n.uf2', kind: 'ota', size: 1400000, url: 'esp32-261105-1234567/espurna-beken-bk7231n.uf2'},
                ],
            },
        },
    ],
};

test('release rows carry the files of the running image and mark the running build', () => {
    const rows = firmwareRows(MANIFEST, {image: 'beken-matter', arch: 'beken', version: '1.22.0-gitabcdef12'});
    expect(rows.length).toBe(2);

    expect(rows[0].tag).toBe('esp32-261106-abcdef1');
    expect(rows[0].running).toBe(true);
    expect(rows[0].choices.map((x) => x.file.name))
        .toEqual(['espurna-beken-matter.bin', 'espurna-beken-initial-tuya.uf2']);
    expect(rows[0].choices.map((x) => installable(x.file))).toEqual([true, false]);

    // older release with the old Beken names has nothing for the 'beken-matter' image
    expect(rows[1].running).toBe(false);
    expect(rows[1].choices).toEqual([]);
});

test('the NSPanel image gets the shared safeboot file of the release, like every ESP32 image', () => {
    const safeboot = {name: 'espurna-esp32-safeboot.bin', kind: 'safeboot', size: 720000, sha256: 'a'.repeat(64),
        url: 'esp32-261107-abcdef1/espurna-esp32-safeboot.bin'};
    /** @type {import('../src/fwinstall.mjs').FirmwareManifest} */
    const manifest = {
        version: 1,
        releases: [{
            tag: 'esp32-261107-abcdef1',
            date: '2026-11-07T10:00:00Z',
            commit: 'abcdef1',
            images: {
                'esp32-matter': [
                    {name: 'espurna-esp32-matter-ota.bin', kind: 'ota', size: 2100000, url: 'x/espurna-esp32-matter-ota.bin'},
                    safeboot,
                ],
                'esp32-matter-nspanel': [
                    {name: 'espurna-esp32-matter-nspanel-ota.bin', kind: 'ota', size: 2500000, url: 'x/espurna-esp32-matter-nspanel-ota.bin'},
                    safeboot,
                ],
            },
        }],
    };

    const rows = firmwareRows(manifest, {image: 'esp32-matter-nspanel', arch: 'esp32', version: '1.22.0-gitabcdef1'});
    expect(rows[0].choices.map((x) => x.file.name))
        .toEqual(['espurna-esp32-matter-nspanel-ota.bin', 'espurna-esp32-safeboot.bin']);
    expect(rows[0].choices.map((x) => installable(x.file))).toEqual([true, false]);
});

test('broken manifest', () => {
    expect(() => firmwareRows(/** @type {any} */({}), {image: '', arch: '', version: ''}))
        .toThrow(/firmware.json/);
});

/**
 * @param {string} name
 * @param {number} size
 * @param {string} [sha256]
 * @returns {import('../src/fwinstall.mjs').FirmwareFile}
 */
function file(name, size, sha256 = 'ab'.repeat(32)) {
    return {name, kind: 'ota', size, url: `esp32-261106-abcdef1/${name}`, sha256};
}

/** @type {import('../src/fwinstall.mjs').FirmwareManifest} */
const MANIFEST_8266 = {
    version: 1,
    releases: [{
        tag: 'esp32-261106-abcdef1',
        date: '2026-11-06T10:00:00Z',
        commit: 'abcdef1234567890abcdef1234567890abcdef12',
        images: {
            'esp8266-1m': [file('espurna-esp8266-1m.bin.gz', 482695), file('espurna-esp8266-1m.bin', 683312)],
            'esp8266-1m-light': [file('espurna-esp8266-1m-light.bin.gz', 547027), file('espurna-esp8266-1m-light.bin', 761808)],
            'esp8266-1m-mini': [file('espurna-esp8266-1m-mini.bin.gz', 303767), file('espurna-esp8266-1m-mini.bin', 424160)],
            'esp8266-4m': [file('espurna-esp8266-4m.bin.gz', 482698), file('espurna-esp8266-4m.bin', 683312)],
        },
    }],
};

/**
 * @param {string} image
 * @returns {import('../src/fwinstall.mjs').FirmwareRow}
 */
function row8266(image) {
    return firmwareRows(MANIFEST_8266, {image, arch: 'esp8266', version: ''})[0];
}

/**
 * @param {import('../src/fwinstall.mjs').FirmwareRow} row
 * @param {string} name
 * @returns {import('../src/fwinstall.mjs').FirmwareChoice}
 */
function choice(row, name) {
    const out = row.choices.find((x) => x.file.name === name);
    if (!out) {
        throw new Error(`no ${name}`);
    }
    return out;
}

test('minimal image of the release and its free space', () => {
    const row = row8266('esp8266-1m');
    const mini = miniFiles(row);
    expect(mini && mini.gz.name).toBe('espurna-esp8266-1m-mini.bin.gz');
    expect(mini && mini.bin.name).toBe('espurna-esp8266-1m-mini.bin');

    // 0xFB000 - 0x68000 (424160 bytes in whole sectors)
    expect(miniFreeSize(file('x.bin', 424160))).toBe(602112);

    // 4 MB image: no minimal one offered
    expect(miniFiles(row8266('esp8266-4m'))).toBe(null);
});

test('1 MB board: the full .bin.gz goes through the minimal image, which downloads it', () => {
    const status = {image: 'esp8266-1m', fetch: 0, free: 344064, result: ''};
    const board = {freeSize: 344064, status};
    const row = row8266('esp8266-1m');

    expect(esp8266Route(row, choice(row, 'espurna-esp8266-1m.bin.gz'), board)).toBe('mini');
    // the uncompressed one does not fit next to the minimal image
    expect(esp8266Route(row, choice(row, 'espurna-esp8266-1m.bin'), board)).toBe(null);
    // the minimal image alone fits the free space, uploaded as before
    expect(esp8266Route(row, choice(row, 'espurna-esp8266-1m-mini.bin.gz'), board)).toBe('upload');

    // no SHA-256 in firmware.json: the board does not download it
    const plain = {...choice(row, 'espurna-esp8266-1m.bin.gz')};
    plain.file = {...plain.file, sha256: ''};
    expect(esp8266Route(row, plain, board)).toBe(null);

    // older firmware without /ota/status: only what fits the free space
    expect(esp8266Route(row, choice(row, 'espurna-esp8266-1m.bin.gz'), {freeSize: 344064, status: null})).toBe(null);
    expect(esp8266Route(row, choice(row, 'espurna-esp8266-1m-mini.bin.gz'), {freeSize: 344064, status: null})).toBe('upload');
});

test('1 MB board running the biggest image: not even the minimal one fits', () => {
    const status = {image: 'esp8266-1m-light', fetch: 0, free: 262144, result: ''};
    const row = row8266('esp8266-1m-light');
    expect(esp8266Route(row, choice(row, 'espurna-esp8266-1m-light.bin.gz'), {freeSize: 262144, status})).toBe(null);
    expect(esp8266Route(row, choice(row, 'espurna-esp8266-1m-mini.bin.gz'), {freeSize: 262144, status})).toBe(null);
});

test('4 MB board downloads by itself when its image can, uploads otherwise', () => {
    const row = row8266('esp8266-4m');
    const gz = choice(row, 'espurna-esp8266-4m.bin.gz');
    const bin = choice(row, 'espurna-esp8266-4m.bin');

    const fetching = {freeSize: 2457600, status: {image: 'esp8266-4m', fetch: 1, free: 2457600, result: ''}};
    expect(esp8266Route(row, gz, fetching)).toBe('fetch');
    expect(esp8266Route(row, bin, fetching)).toBe('fetch');

    const uploading = {freeSize: 2457600, status: {image: 'esp8266-4m', fetch: 0, free: 2457600, result: ''}};
    expect(esp8266Route(row, gz, uploading)).toBe('upload');
});

test('result of the download as /ota/status shows it', () => {
    expect(fetchOutcome({image: 'esp8266-1m', result: ''})).toBe('wait');
    expect(fetchOutcome({image: 'esp8266-1m-mini', result: ''})).toBe('wait');
    expect(fetchOutcome({image: 'esp8266-1m-mini', result: 'ok: downloaded 482695 bytes, SHA-256 checked'})).toBe('wait');
    expect(fetchOutcome({image: 'esp8266-1m', result: 'ok: downloaded 482695 bytes, SHA-256 checked'})).toBe('done');
    expect(fetchOutcome({image: 'esp8266-1m-mini', result: 'failed: SHA-256 mismatch'})).toBe('failed');
    expect(fetchOutcome({image: 'esp8266-4m', result: 'failed: no Wi-Fi connection'})).toBe('failed');
});

test('install steps ahead of a Beken upload', () => {
    const keys = (steps) => steps.map((step) => step.key);
    const name = 'espurna-beken-matter.bin';

    expect(keys(uploadSteps({name, layout: 'Quectel', matter: true, sameBuild: false})))
        .toEqual(['download', 'upload', 'restart']);
    expect(keys(uploadSteps({name, layout: 'Tuya', matter: false, sameBuild: true})))
        .toEqual(['download', 'upload', 'restart']);
    expect(keys(uploadSteps({name, layout: 'Tuya', matter: false, sameBuild: false})))
        .toEqual(['download', 'upload', 'again-restart', 'again-upload', 'restart']);
    expect(keys(uploadSteps({name, layout: 'Tuya', matter: true, sameBuild: false})))
        .toEqual(['download', 'matter-upload', 'matter-restart', 'upload', 'again-restart', 'again-upload', 'restart']);
});

test('install steps follow what the board answers', () => {
    const steps = uploadSteps({name: 'x.bin', layout: 'Tuya', matter: true, sameBuild: false});

    // Matter was not running after all: its two steps are dropped
    let current = stepTo(steps, 0, {key: 'upload', text: ''});
    expect(current).toBe(1);
    expect(steps.map((step) => step.key)).toEqual(['download', 'upload', 'again-restart', 'again-upload', 'restart']);

    // the same build after all: done at once, the second upload is dropped
    current = stepTo(steps, current, {key: 'restart', text: ''});
    expect(current).toBe(2);
    expect(steps.map((step) => step.key)).toEqual(['download', 'upload', 'restart']);

    // an answer that was not planned is added after the current step
    const other = uploadSteps({name: 'x.bin', layout: 'Quectel', matter: false, sameBuild: false});
    current = stepTo(other, 1, {key: 'matter-restart', text: 'restart without Matter'});
    expect(current).toBe(2);
    expect(other.map((step) => step.key)).toEqual(['download', 'upload', 'matter-restart', 'restart']);
});
