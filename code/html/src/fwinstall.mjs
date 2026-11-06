// Firmware install from GitHub (ADMIN, OTA)
//
// CI copies the files of the kept releases to GitHub Pages with a firmware.json (`webFwUrl` replaces the default one).
// The release files themselves cannot be used: GitHub serves them without CORS, so the browser cannot download them,
// and the Beken mbedTLS cannot talk to that host. This browser downloads the file of the image the board runs
// (`fwImage`, ESPURNA_IMAGE of the build) and uploads it to the board, same as Browse / Upgrade:
// - ESP8266 (/ota/status answers): the board downloads the .bin.gz itself (https, SHA-256 of firmware.json):
//   'fetch' - the image fits next to the running one and the running one can download (4 MB layouts), POST /ota/fetch
//   and it restarts for it; 'mini' - 1 MB boards: /ota/fetch only stores the request, the browser uploads the minimal
//   image (espurna-esp8266-1m-mini.bin.gz) to /upgrade, the minimal image downloads the full one and boots it;
//   'upload' - the file fits the free space, the browser uploads it (also older firmware without /ota/status)
// - Beken (and ESP8266 'upload'): POST /upgrade. HTTP 409 - Matter was running from the OTA partition, the board
//   reboots without it and the same file goes up once more; "OK, again" - Beken with the Tuya bootloader, the same
//   file once more after the reboot
// - ESP32: the board downloads the -ota.bin itself, through its safeboot image (https, SHA-256 of firmware.json).
//   The browser only brings the safeboot image of the release when the installed one differs (/safeboot/install),
//   or, on the old two-slot layout, for the migration to the safeboot layout (/safeboot/migrate with the image URL)

import { notifyError } from './notify.mjs';
import { listenAppConnected } from './connection.mjs';
import { variableListeners } from './settings.mjs';
import { pageReloadIn } from './core.mjs';

export const FIRMWARE_URL = 'https://igorufox.github.io/espurna/firmware.json';

/**
 * @typedef {{name: string, kind: string, size: number, url: string, sha256?: string}} FirmwareFile
 * @typedef {{tag: string, date: string, commit: string, release?: string, images: {[k: string]: FirmwareFile[]}}} FirmwareRelease
 * @typedef {{version: number, repository?: string, generated?: string, releases: FirmwareRelease[]}} FirmwareManifest
 */

/**
 * @typedef {{image: string, step?: string, file: FirmwareFile}} FirmwareChoice
 * @typedef {{tag: string, date: string, commit: string, release: string, running: boolean, choices: FirmwareChoice[]}} FirmwareRow
 */

// ---------------------------------------------------------------------------
// image headers, also used by Browse / Upgrade
// ---------------------------------------------------------------------------

/**
 * @param {Uint8Array} buffer
 * @param {string} text
 * @returns {boolean}
 */
function startsWithText(buffer, text) {
    if (buffer.length < text.length) {
        return false;
    }

    for (let index = 0; index < text.length; ++index) {
        if (buffer[index] !== text.charCodeAt(index)) {
            return false;
        }
    }

    return true;
}

/**
 * What kind of image the first bytes belong to
 * - 'esp' - ESP8266 / ESP32 application (magic 0xE9), buffer[2] is the flash mode
 * - 'gzip' - compressed ESP8266 image
 * - 'uf2' - LibreTiny UF2 (Beken)
 * - 'beken' - ESPBEKEN, Beken OTA image of this firmware
 * - 'matter-tail' - ESPMTAIL, Beken Matter code of the Tuya layout
 * @param {Uint8Array} buffer first 8 bytes (or more) of the file
 * @returns {string | null}
 */
export function firmwareHeader(buffer) {
    if ((buffer.length >= 2) && (0x1f === buffer[0]) && (0x8b === buffer[1])) {
        return 'gzip';
    }

    if ((buffer.length >= 1) && (0xe9 === buffer[0])) {
        return 'esp';
    }

    if (startsWithText(buffer, 'UF2\n')) {
        return 'uf2';
    }

    if (startsWithText(buffer, 'ESPBEKEN')) {
        return 'beken';
    }

    if (startsWithText(buffer, 'ESPMTAIL')) {
        return 'matter-tail';
    }

    return null;
}

/**
 * OTA space the image takes, in whole flash sectors
 * @param {number} size
 * @returns {number}
 */
export function roundedSize(size) {
    return (size - (size % 4096)) + 4096;
}

// ---------------------------------------------------------------------------
// release list
// ---------------------------------------------------------------------------

/**
 * Two-step update of 1 MB ESP8266 boards: the mini image first, then a full image of the same flash size
 * @type {{[k: string]: string[]}}
 */
const TWO_STEP = {
    'esp8266-1m-mini': ['esp8266-1m', 'esp8266-1m-light', 'esp8266-1m-sensor'],
};

/** minimal image of the 1 MB boards */
export const MINI_IMAGE = 'esp8266-1m-mini';

/** end of the sketch + OTA space in the 1 MB layout (eagle.flash.1m.ld: EEPROM at 0xFB000) */
const SKETCH_END_1M = 0xfb000;

/**
 * @param {string} image
 * @returns {boolean}
 */
function isMinimal(image) {
    return Object.hasOwn(TWO_STEP, image);
}

/**
 * Images of a release the board can be updated to, with the step they are for:
 * - the running image
 * - running the minimal image (1st step of the two-step update on 1 MB boards): the full images of the same flash size
 * - running a full image: the minimal one of the same flash size, as the 1st step when the full one does not fit
 * - no image (local build): every image of this chip
 * @param {string[]} images names in the release
 * @param {string} image running one, '' when not known
 * @param {string} arch esp8266, esp32, beken
 * @returns {{image: string, step?: string}[]}
 */
export function imageCandidates(images, image, arch) {
    if (!image) {
        return images
            .filter((name) => !arch || name.startsWith(arch))
            .map((name) => ({image: name}));
    }

    if (isMinimal(image)) {
        return TWO_STEP[image]
            .filter((name) => images.includes(name))
            .map((name) => ({image: name, step: 'two-step update, step 2'}));
    }

    /** @type {{image: string, step?: string}[]} */
    const out = [];
    if (images.includes(image)) {
        out.push({image});
    }

    for (const [mini, full] of Object.entries(TWO_STEP)) {
        if (full.includes(image) && images.includes(mini)) {
            out.push({image: mini, step: 'two-step update, step 1, when the image does not fit'});
        }
    }

    return out;
}

/**
 * Rows of the release list, newest release first, files the board can be updated with
 * @param {FirmwareManifest} manifest
 * @param {{image: string, arch: string, version: string}} board
 * @returns {FirmwareRow[]}
 */
export function firmwareRows(manifest, {image, arch, version}) {
    if (!manifest || !Array.isArray(manifest.releases)) {
        throw new Error('not a firmware.json (no releases)');
    }

    return manifest.releases.map((release) => {
        const images = release.images || {};
        const candidates = imageCandidates(Object.keys(images), image, arch);

        /** @type {FirmwareChoice[]} */
        const choices = [];
        for (const candidate of candidates) {
            for (const file of images[candidate.image] || []) {
                choices.push({...candidate, file});
            }
        }

        const commit = String(release.commit || '');
        const short = commit.slice(0, 7);

        return {
            tag: String(release.tag),
            date: String(release.date || ''),
            commit,
            release: String(release.release || ''),
            running: (short.length === 7) && version.includes(short),
            choices,
        };
    });
}

/**
 * @param {FirmwareFile} file
 * @returns {boolean}
 */
export function installable(file) {
    return file.kind === 'ota';
}

/**
 * Whether the file can be uploaded to /upgrade (ESP8266, free space as the board reported it; 0 - not known).
 * The board starts the update for the free space minus one sector
 * @param {FirmwareFile} file
 * @param {number} freeSize
 * @returns {boolean}
 */
export function fits(file, freeSize) {
    return (freeSize <= 0) || (file.size <= (freeSize - 4096));
}

/**
 * Whether the board can download the file into its free space itself (ota_fetch.cpp: Update for one byte more)
 * @param {FirmwareFile} file
 * @param {number} freeSize
 * @returns {boolean}
 */
export function fetchFits(file, freeSize) {
    return (freeSize <= 0) || (roundedSize(file.size) <= freeSize);
}

/**
 * Free space of the minimal image, from the size of its .bin (1 MB layout)
 * @param {FirmwareFile} bin
 * @returns {number}
 */
export function miniFreeSize(bin) {
    return SKETCH_END_1M - (Math.ceil(bin.size / 4096) * 4096);
}

/**
 * Files of the minimal image in a release row: .bin.gz to upload, .bin for its size
 * @param {FirmwareRow} row
 * @returns {{gz: FirmwareFile, bin: FirmwareFile} | null}
 */
export function miniFiles(row) {
    const files = row.choices
        .filter((choice) => (choice.image === MINI_IMAGE) && installable(choice.file))
        .map((choice) => choice.file);

    const gz = files.find((file) => file.name.endsWith('.bin.gz'));
    const bin = files.find((file) => file.name.endsWith('.bin'));

    return (gz && bin) ? {gz, bin} : null;
}

/**
 * @typedef {{image?: string, fetch?: number, free?: number, url?: string, result?: string}} OtaStatus
 */

/**
 * How an ESP8266 board gets the file, null - it cannot
 * - 'fetch' - the running image downloads it itself (/ota/fetch, then it restarts)
 * - 'upload' - it fits the free space, the browser uploads it (/upgrade)
 * - 'mini' - the request is stored (/ota/fetch), the browser uploads the minimal image, which downloads the file
 * @param {FirmwareRow} row
 * @param {FirmwareChoice} choice
 * @param {{freeSize: number, status: OtaStatus | null}} board status - /ota/status, null for older firmware
 * @returns {string | null}
 */
export function esp8266Route(row, choice, {freeSize, status}) {
    const {file} = choice;
    if (!installable(file)) {
        return null;
    }

    const sha256 = !!file.sha256;
    if (status && status.fetch && sha256 && fetchFits(file, freeSize)) {
        return 'fetch';
    }

    if (fits(file, freeSize)) {
        return 'upload';
    }

    if (status && sha256 && (choice.image !== MINI_IMAGE)) {
        const mini = miniFiles(row);
        if (mini && fits(mini.gz, freeSize) && fetchFits(file, miniFreeSize(mini.bin))) {
            return 'mini';
        }
    }

    return null;
}

/**
 * What /ota/status tells about the download, once the board is back
 * - 'wait' - nothing yet (old firmware before its restart, the minimal image downloading)
 * - 'done' - a full image is back and the download went fine
 * - 'failed' - the download failed (the minimal image stays, a full image restarted unchanged)
 * @param {OtaStatus} status
 * @returns {string}
 */
export function fetchOutcome(status) {
    const result = String(status.result || '');
    if (result.startsWith('failed')) {
        return 'failed';
    }

    if (result && (status.image !== MINI_IMAGE)) {
        return 'done';
    }

    return 'wait';
}

/**
 * What the board answered to the upload
 * - 'done' - written, the board reboots into it
 * - 'again' - written, the same file once more after the reboot (Beken, Tuya bootloader)
 * - 'matter' - nothing written, Matter runs from the OTA partition, the board reboots without it
 * - 'error'
 * @param {number} status
 * @param {string} text
 * @returns {string}
 */
export function uploadOutcome(status, text) {
    if (status === 409) {
        return 'matter';
    }

    if (status === 200) {
        const trimmed = text.trim();
        if (trimmed === 'OK, again') {
            return 'again';
        }

        if (trimmed === 'OK') {
            return 'done';
        }
    }

    return 'error';
}

// ---------------------------------------------------------------------------
// install
// ---------------------------------------------------------------------------

const state = {
    url: '',
    image: '',
    imageKnown: false,
    arch: '',
    version: '',
    freeSize: 0,
    /** @type {OtaStatus | null} */
    otaStatus: null,
    busy: false,
    /** @type {import('./connection.mjs').ConnectionUrls | null} */
    urls: null,
};

/**
 * The board reboots on its own while installing, the WebUI must not reload the page meanwhile
 * @returns {boolean}
 */
export function firmwareInstallRunning() {
    return state.busy;
}

/** @returns {string} */
function manifestUrl() {
    return state.url.trim() || FIRMWARE_URL;
}

/** @param {number} ms */
function wait(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

/**
 * @param {string | URL} url
 * @param {RequestInit} [options]
 * @param {number} [timeout]
 * @returns {Promise<Response>}
 */
async function fetchTimeout(url, options = {}, timeout = 4000) {
    const controller = new AbortController();
    const id = setTimeout(() => controller.abort(), timeout);
    try {
        return await fetch(url, {cache: 'no-store', credentials: 'same-origin', ...options, signal: controller.signal});
    } finally {
        clearTimeout(id);
    }
}

/**
 * @param {string} text
 * @param {string} [className]
 */
function setStatus(text, className = '') {
    // ADMIN release list, and the MATTER panel when it installs the Matter code
    for (const id of ['firmware-status', 'matter-code-status']) {
        const status = document.getElementById(id);
        if (status) {
            status.textContent = text;
            status.className = className;
        }
    }
}

/**
 * Waits until the board went away and answers again with the ESPurna WebUI login
 * @param {URL} auth
 * @param {string} what
 * @param {number} [timeout]
 */
async function waitForBoard(auth, what, timeout = 240000) {
    const start = Date.now();
    let down = false;

    for (;;) {
        await wait(1500);

        const seconds = Math.round((Date.now() - start) / 1000);
        if ((Date.now() - start) > timeout) {
            throw new Error(`the board did not come back in ${seconds} s. Open its address again later`);
        }

        try {
            const response = await fetchTimeout(auth, {}, 3000);
            // reboots take more than a few seconds (Wi-Fi), the old firmware still answering right after the upload
            // is not taken for the new one
            if (response.ok && (down || (seconds > 45))) {
                return;
            }
        } catch {
            down = true;
        }

        setStatus(`${what} (${seconds} s)...`);
    }
}

/**
 * @param {URL} url
 * @param {Blob} blob
 * @param {string} name
 * @param {boolean} form multipart (ESPurna /upgrade) or the file as the body (safeboot /upload)
 * @returns {Promise<{status: number, text: string}>}
 */
function upload(url, blob, name, form) {
    return new Promise((resolve, reject) => {
        const xhr = new XMLHttpRequest();
        xhr.open('POST', url.href);

        xhr.upload.addEventListener('progress', (event) => {
            if (event.lengthComputable) {
                setStatus(`Uploading ${name}: ${Math.round((event.loaded * 100) / event.total)}%`);
            }
        });

        xhr.addEventListener('load', () => {
            resolve({status: xhr.status, text: xhr.responseText});
        });
        xhr.addEventListener('error', () => {
            reject(new Error('connection error while uploading'));
        });
        xhr.addEventListener('abort', () => {
            reject(new Error('upload aborted'));
        });

        if (form) {
            const data = new FormData();
            data.append('upgrade', blob, name);
            xhr.send(data);
        } else {
            xhr.send(blob);
        }
    });
}

/**
 * @param {import('./connection.mjs').ConnectionUrls} urls
 * @param {Blob} blob
 * @param {string} name
 */
async function installUpgrade(urls, blob, name) {
    let matter = 1;
    let again = 1;

    for (;;) {
        const {status, text} = await upload(urls.upgrade, blob, name, true);
        const outcome = uploadOutcome(status, text);

        if ((outcome === 'matter') && (matter-- > 0)) {
            await waitForBoard(urls.auth, 'Matter was running from the OTA space, the board restarts without it, then the same file goes up again');
            continue;
        }

        if ((outcome === 'again') && (again-- > 0)) {
            await waitForBoard(urls.auth, 'First part written, the board restarts, then the same file goes up once more');
            continue;
        }

        if (outcome === 'done') {
            await waitForBoard(urls.auth, 'Uploaded, the board restarts into the new firmware');
            return;
        }

        throw new Error(`the board answered ${status}: ${text.trim() || '(nothing)'}`);
    }
}

/**
 * ESP8266 /ota/status, null when the firmware has none (older ones)
 * @param {URL} base
 * @returns {Promise<OtaStatus | null>}
 */
async function otaStatus(base) {
    try {
        const response = await fetchTimeout(new URL('ota/status', base), {}, 8000);
        if (!response.ok) {
            return null;
        }

        return await response.json();
    } catch {
        return null;
    }
}

/**
 * After /ota/fetch: waits until a full image is back with the result of the download
 * @param {URL} base
 * @param {string} name
 */
async function waitForFetch(base, name) {
    const start = Date.now();
    let mini = false;

    for (;;) {
        await wait(2000);
        const seconds = Math.round((Date.now() - start) / 1000);
        if (seconds > 420) {
            throw new Error(mini
                ? 'no result from the minimal image. Open the board address: its page shows the last download and lists the releases'
                : `the board did not come back in ${seconds} s. Open its address again later`);
        }

        /** @type {OtaStatus | null} */
        let status = null;
        try {
            const response = await fetchTimeout(new URL('ota/status', base), {}, 3000);
            if (response.ok) {
                status = await response.json();
            }
        } catch {
            status = null;
        }

        if (!status) {
            setStatus(`${mini ? 'The minimal image downloads' : 'The board restarts to download'} ${name} (${seconds} s)...`);
            continue;
        }

        mini = mini || (status.image === MINI_IMAGE);

        const outcome = fetchOutcome(status);
        if (outcome === 'done') {
            return;
        }

        if (outcome === 'failed') {
            throw new Error((status.image === MINI_IMAGE)
                ? `the minimal image could not install it (${status.result}). It stays running: open the board address, its page lists the releases`
                : `the board could not download it (${status.result}). It runs the previous firmware`);
        }

        setStatus(`${mini ? 'The minimal image downloads' : 'Waiting for the board to download'} ${name} (${seconds} s)...`);
    }
}

/**
 * ESP8266: the board downloads the file itself, directly or through the minimal image
 * @param {import('./connection.mjs').ConnectionUrls} urls
 * @param {FirmwareRow} row
 * @param {FirmwareChoice} choice
 * @param {string} route
 */
async function installEsp8266(urls, row, choice, route) {
    const base = urls.upgrade;
    const {file} = choice;
    const fields = {url: new URL(file.url, manifestUrl()).href, sha256: file.sha256 || ''};

    if (route === 'fetch') {
        setStatus('Restarting to download...');
        const result = await post(new URL('ota/fetch', base), fields);
        if ((result.status !== 200) || !result.text.startsWith('OK, fetch')) {
            throw new Error(`/ota/fetch: ${result.status} ${result.text.trim()}`);
        }

        await waitForFetch(base, file.name);
        return;
    }

    const mini = miniFiles(row);
    if (!mini) {
        throw new Error(`release ${releaseName(row)} has no minimal image`);
    }

    // downloaded before anything is stored on the board
    const blob = await downloadFile(mini.gz);

    const result = await post(new URL('ota/fetch', base), fields);
    if ((result.status !== 200) || !result.text.startsWith('OK')) {
        throw new Error(`/ota/fetch: ${result.status} ${result.text.trim()}`);
    }

    const uploaded = await upload(urls.upgrade, blob, mini.gz.name, true);
    if (uploadOutcome(uploaded.status, uploaded.text) !== 'done') {
        throw new Error(`upload of ${mini.gz.name}: the board answered ${uploaded.status}: ${uploaded.text.trim() || '(nothing)'}`);
    }

    await waitForFetch(base, file.name);
}

/**
 * @typedef {{layout?: string, running?: string, safeboot?: string, sha256?: string, result?: string, error?: string, last?: string}} SafebootStatus
 */

/**
 * @param {URL} base
 * @param {number} [size] bytes of the release's safeboot image: its SHA-256 over the installed one comes back
 * @returns {Promise<SafebootStatus | null>}
 */
async function safebootStatus(base, size = 0) {
    try {
        const url = new URL('safeboot/status', base);
        if (size) {
            url.searchParams.set('size', String(size));
        }
        const response = await fetchTimeout(url, {}, 8000);
        if (!response.ok) {
            return null;
        }

        return await response.json();
    } catch {
        return null;
    }
}

/**
 * @param {FirmwareFile} file
 * @returns {Promise<Blob>}
 */
async function downloadFile(file) {
    setStatus(`Downloading ${file.name}...`);
    const response = await fetch(new URL(file.url, manifestUrl()));
    if (!response.ok) {
        throw new Error(`download of ${file.name}: HTTP ${response.status}`);
    }

    const blob = await response.blob();
    if (file.size && (blob.size !== file.size)) {
        throw new Error(`download of ${file.name}: ${blob.size} bytes instead of ${file.size}`);
    }

    return blob;
}

/**
 * @param {URL} url
 * @param {{[k: string]: string}} fields
 * @param {{blob: Blob, name: string}} [file]
 * @returns {Promise<{status: number, text: string}>}
 */
function post(url, fields, file) {
    return new Promise((resolve, reject) => {
        const data = new FormData();
        for (const [key, value] of Object.entries(fields)) {
            data.append(key, value);
        }
        if (file) {
            data.append('file', file.blob, file.name);
        }

        const xhr = new XMLHttpRequest();
        xhr.open('POST', url.href);
        if (file) {
            xhr.upload.addEventListener('progress', (event) => {
                if (event.lengthComputable) {
                    setStatus(`Uploading ${file.name}: ${Math.round((event.loaded * 100) / event.total)}%`);
                }
            });
        }
        xhr.addEventListener('load', () => resolve({status: xhr.status, text: xhr.responseText}));
        xhr.addEventListener('error', () => reject(new Error('connection error')));
        xhr.addEventListener('abort', () => reject(new Error('aborted')));
        xhr.send(data);
    });
}

/**
 * After the board went into safeboot: waits for ESPurna again. Safeboot answers 404 on /safeboot/status, ESPurna
 * the JSON of the app0 image
 * @param {URL} base
 * @param {string} what
 * @returns {Promise<SafebootStatus>}
 */
async function waitForEspurna(base, what) {
    const start = Date.now();
    let away = false;
    let safeboot = 0;

    for (;;) {
        await wait(2000);
        const seconds = Math.round((Date.now() - start) / 1000);
        if (seconds > 300) {
            throw new Error((safeboot > 10)
                ? 'the board stays in safeboot. Open its address: the safeboot page (user admin, the admin password) lists the releases and shows the last result'
                : `the board did not come back in ${seconds} s. Open its address again later`);
        }

        try {
            const response = await fetchTimeout(new URL('safeboot/status', base), {}, 3000);
            if (response.status === 404) {
                away = true;
                ++safeboot;
                setStatus(`${what}: safeboot is downloading the image (${seconds} s)...`);
                continue;
            }

            // the old firmware still answers right after the request, before its restart
            if (response.ok && (away || (seconds > 60))) {
                return await response.json();
            }
        } catch {
            away = true;
        }

        setStatus(`${what} (${seconds} s)...`);
    }
}

/**
 * ESP32: safeboot of the release when the installed one differs (or the migration from the two-slot layout), then
 * the board downloads the image itself
 * @param {import('./connection.mjs').ConnectionUrls} urls
 * @param {FirmwareRow} row
 * @param {FirmwareChoice} choice
 */
async function installEsp32(urls, row, choice) {
    const base = urls.upgrade;
    const {file} = choice;
    const imageUrl = new URL(file.url, manifestUrl()).href;
    const sha256 = file.sha256 || '';
    const safebootFile = row.choices
        .map((other) => other.file)
        .find((other) => other.kind === 'safeboot');

    setStatus('Reading the flash layout...');
    const status = await safebootStatus(base, safebootFile ? safebootFile.size : 0);
    if (!status) {
        throw new Error('no answer from /safeboot/status: this firmware has no safeboot support, update it with a file');
    }

    let back;
    if (status.layout === 'old') {
        if (status.error) {
            throw new Error(`the migration to the safeboot layout is not possible: ${status.error}`);
        }
        if (!safebootFile) {
            throw new Error(`release ${releaseName(row)} has no safeboot image, the two-slot layout needs it for the migration`);
        }

        const blob = await downloadFile(safebootFile);
        const result = await post(new URL('safeboot/migrate', base), {url: imageUrl, sha256}, {blob, name: safebootFile.name});
        if (result.status !== 200) {
            throw new Error(`migration: ${result.status} ${result.text.trim()}`);
        }

        back = await waitForEspurna(base, 'New flash layout, the board restarts into safeboot');
    } else if (status.layout === 'safeboot') {
        if (safebootFile && safebootFile.sha256 && (status.sha256 !== safebootFile.sha256)) {
            const blob = await downloadFile(safebootFile);
            const result = await post(new URL('safeboot/install', base), {}, {blob, name: safebootFile.name});
            if (result.status !== 200) {
                throw new Error(`safeboot install: ${result.status} ${result.text.trim()}`);
            }

            const check = await safebootStatus(base, safebootFile.size);
            if (!check || (check.sha256 !== safebootFile.sha256)) {
                throw new Error('the installed safeboot image does not match the release (SHA-256), nothing else was changed');
            }
        }

        setStatus('Restarting into safeboot...');
        const result = await post(new URL('safeboot/update', base), {url: imageUrl, sha256});
        if (result.status !== 200) {
            throw new Error(`safeboot: ${result.status} ${result.text.trim()}`);
        }

        back = await waitForEspurna(base, 'Restarting into safeboot');
    } else {
        throw new Error(`unknown flash layout (${status.layout || '?'}), update it with a file`);
    }

    if (back.result && back.result.startsWith('failed')) {
        throw new Error(`safeboot ${back.result}. ESPurna is back unchanged`);
    }
}

/**
 * @param {FirmwareRow} row
 * @param {FirmwareChoice} choice
 */
async function install(row, choice) {
    const urls = state.urls;
    if (!urls) {
        alert('Not connected');
        return;
    }

    if (state.busy) {
        return;
    }

    const {file} = choice;
    const warning = state.imageKnown ? ''
        : '\n\nThis build does not say which image it is: make sure it is the one for this board.';
    const step = choice.step ? `\n(${choice.step})` : '';
    if (!window.confirm(`Install ${file.name} of ${releaseName(row)} on this board?${step}\n\nThe board restarts, keep it powered until it is back.${warning}`)) {
        return;
    }

    state.busy = true;
    for (const button of document.querySelectorAll('#firmware-releases button')) {
        /** @type {HTMLButtonElement} */(button).disabled = true;
    }

    try {
        const route = (state.arch === 'esp8266')
            ? esp8266Route(row, choice, {freeSize: state.freeSize, status: state.otaStatus}) : null;
        if (state.arch === 'esp32') {
            await installEsp32(urls, row, choice);
        } else if ((route === 'fetch') || (route === 'mini')) {
            await installEsp8266(urls, row, choice, route);
        } else {
            const blob = await downloadFile(file);
            const header = firmwareHeader(new Uint8Array(await blob.slice(0, 8).arrayBuffer()));
            if (!header) {
                throw new Error(`${file.name} does not look like a firmware image`);
            }

            await installUpgrade(urls, blob, file.name);
        }

        setStatus(`${file.name} installed, the board is back. Reloading the page...`, 'firmware-ok');
        pageReloadIn(2000);
    } catch (e) {
        setStatus(`Install stopped: ${/** @type {Error} */(e).message}`, 'firmware-error');
        notifyError(/** @type {Error} */(e));
        state.busy = false;
        for (const button of document.querySelectorAll('#firmware-releases button')) {
            /** @type {HTMLButtonElement} */(button).disabled = false;
        }
    }
}

/**
 * @param {number} size
 * @returns {string}
 */
function sizeText(size) {
    return `${Math.round(size / 1024)} KB`;
}

/**
 * @param {FirmwareRow} row
 * @param {FirmwareChoice} choice
 * @returns {HTMLLIElement}
 */
function choiceItem(row, choice) {
    const item = document.createElement('li');
    const {file} = choice;

    const name = document.createElement('code');
    name.textContent = file.name;
    item.appendChild(name);

    const info = [sizeText(file.size)];
    if (choice.step) {
        info.push(choice.step);
    }

    if (!installable(file)) {
        info.push((file.kind === 'serial') ? 'serial flash only' : file.kind);
    }

    const route = (state.arch === 'esp8266')
        ? esp8266Route(row, choice, {freeSize: state.freeSize, status: state.otaStatus}) : null;
    const ok = installable(file) && ((state.arch !== 'esp8266') || (route !== null));
    if (route === 'fetch') {
        info.push('the board downloads it');
    } else if (route === 'mini') {
        info.push('the board installs the minimal image first, which downloads this one');
    } else if (installable(file) && !ok) {
        info.push(`does not fit, ${state.freeSize} bytes free`);
    }

    item.appendChild(document.createTextNode(` (${info.join(', ')}) `));

    if (ok) {
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'pure-button button-firmware-install';
        button.textContent = 'Install';
        button.disabled = state.busy;
        button.addEventListener('click', (event) => {
            event.preventDefault();
            install(row, choice);
        });
        item.appendChild(button);
    } else {
        const link = document.createElement('a');
        link.href = new URL(file.url, manifestUrl()).href;
        link.textContent = 'download';
        item.appendChild(link);
    }

    return item;
}

/**
 * The build date names a release: CI tags start with 'esp32-' for every chip
 * @param {FirmwareRow} row
 * @returns {string}
 */
export function releaseName(row) {
    return row.date
        ? row.date.replace('T', ' ').replace(/:\d\dZ$/, ' UTC')
        : row.tag;
}

/**
 * @param {HTMLElement} list
 * @param {FirmwareRow[]} rows
 */
function showRows(list, rows) {
    list.replaceChildren();

    for (const row of rows) {
        const item = document.createElement('li');

        // the commit links to the release page
        const title = document.createElement('strong');
        title.textContent = releaseName(row);
        item.appendChild(title);

        if (row.commit) {
            item.appendChild(document.createTextNode(', commit '));
            const short = row.commit.slice(0, 7);
            if (row.release) {
                const link = document.createElement('a');
                link.href = row.release;
                link.target = '_blank';
                link.rel = 'noopener';
                link.textContent = short;
                item.appendChild(link);
            } else {
                item.appendChild(document.createTextNode(short));
            }
        }

        if (row.running) {
            item.appendChild(document.createTextNode(', running now'));
        }

        const files = document.createElement('ul');
        for (const choice of row.choices) {
            files.appendChild(choiceItem(row, choice));
        }

        item.appendChild(files);
        list.appendChild(item);
    }
}

/**
 * MATTER panel, Beken with the Tuya bootloader: the Matter code is missing (the second upload of the image did
 * not happen). Installs the newest release of the running image: the same build only adds its Matter code, a newer
 * one updates the firmware first and its Matter code goes up after the reboot by itself ('again')
 */
export async function installMatterCode() {
    if (state.busy) {
        return;
    }

    if (!state.imageKnown) {
        setStatus('This build does not say which image it is (local build): upload its espurna-beken-matter.bin in ADMIN, Upgrade.', 'firmware-error');
        return;
    }

    const url = manifestUrl();
    setStatus(`Downloading ${url}...`);

    /** @type {FirmwareRow[]} */
    let rows;
    try {
        const response = await fetch(url, {cache: 'no-store'});
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }

        rows = firmwareRows(await response.json(), {
            image: state.image, arch: state.arch, version: state.version})
            .filter((row) => row.choices.some((choice) => installable(choice.file)));
    } catch (e) {
        setStatus(`Could not load ${url}: ${/** @type {Error} */(e).message}`, 'firmware-error');
        return;
    }

    const row = rows[0];
    const choice = row ? row.choices.find((other) => installable(other.file)) : undefined;
    if (!row || !choice) {
        setStatus(`No release has files of the ${state.image} image.`, 'firmware-error');
        return;
    }

    setStatus('');
    await install(row, choice);
}

/** @param {HTMLElement} list */
async function loadReleases(list) {
    if (state.busy) {
        return;
    }

    const url = manifestUrl();
    setStatus(`Downloading ${url}...`);

    // ESP8266: whether the board downloads files itself, and the result of its last download
    state.otaStatus = ((state.arch === 'esp8266') && state.urls)
        ? await otaStatus(state.urls.upgrade) : null;
    const last = (state.otaStatus && state.otaStatus.result)
        ? ` Last download by the board: ${state.otaStatus.result}` : '';

    try {
        const response = await fetch(url, {cache: 'no-store'});
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }

        // releases without a file for this board are not listed
        const rows = firmwareRows(await response.json(), {
            image: state.image, arch: state.arch, version: state.version})
            .filter((row) => row.choices.length > 0);
        showRows(list, rows);

        const count = rows.length;
        setStatus((!count
            ? `No release has files of the ${state.image || state.arch || 'this'} image.`
            : state.imageKnown
            ? `${count} release(s), files of the ${state.image} image.`
            : `${count} release(s). This build does not say which image it is (local build): all ${state.arch || ''} images are listed, pick the one built for this board.`) + last,
            state.imageKnown ? '' : 'firmware-error');
    } catch (e) {
        list.replaceChildren();
        setStatus(`Could not load ${url}: ${/** @type {Error} */(e).message}`, 'firmware-error');
    }
}

export function init() {
    const button = document.querySelector('.button-firmware-list');
    const list = document.getElementById('firmware-releases');
    const image = document.getElementById('firmware-image');
    const url = /** @type {HTMLInputElement | null} */
        (document.querySelector('input[name="webFwUrl"]'));

    if (!(button instanceof HTMLButtonElement) || !list || !image || !url) {
        return;
    }

    variableListeners({
        'fwImage': (_, value) => {
            state.image = String(value);
            state.imageKnown = state.image.length > 0;
            image.textContent = state.imageKnown ? state.image : 'not known (local build)';
        },
        'arch': (_, value) => {
            state.arch = String(value);
        },
        'app_version': (_, value) => {
            state.version = String(value);
        },
        'free_size': (_, value) => {
            state.freeSize = parseInt(String(value), 10) || 0;
        },
        'webFwUrl': (_, value) => {
            state.url = String(value);
        },
    });

    // used as typed, Save keeps it on the device for the next time
    url.addEventListener('change', () => {
        state.url = url.value;
    });

    listenAppConnected((urls) => {
        state.urls = urls;
    });

    button.addEventListener('click', (event) => {
        event.preventDefault();
        loadReleases(list);
    });
}
