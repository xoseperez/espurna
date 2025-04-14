import { notifyMessage } from './notify.mjs';
import { listenAppConnected } from './connection.mjs';
import { variableListeners } from './settings.mjs';

let __free_size = 0;

/** @param {function(HTMLInputElement): void} callback */
function withUpgrade(callback) {
    callback(/** @type {!HTMLInputElement} */
        (document.querySelector("input[name=upgrade]")));
}

/** @param {function(HTMLProgressElement): void} callback */
function withProgress(callback) {
    callback(/** @type {!HTMLProgressElement} */
        (document.querySelector("progress#upgrade-progress")));
}

/**
 * @param {number} flash_mode
 * @returns string
 */
function describeFlashMode(flash_mode) {
    if ((0 <= flash_mode) && (flash_mode <= 3)) {
        const modes = ['QIO', 'QOUT', 'DIO', 'DOUT'];
        return `${modes[flash_mode]} (${flash_mode})`;
    }

    return `Unknown flash mode ${flash_mode}`;
}

/**
 * @param {Uint8Array} buffer
 * @returns boolean
 */
function isGzip(buffer) {
    return (0x1f === buffer[0]) && (0x8b === buffer[1]);
}

/**
 * @param {Uint8Array} buffer
 * @returns number
 */
function flashMode(buffer) {
    return buffer[2];
}

/**
 * @param {Uint8Array} buffer
 * @returns boolean
 */
function checkMagic(buffer) {
    return 0xe9 === buffer[0];
}

/**
 * @param {Uint8Array} buffer
 * @returns boolean
 */
function checkFlashMode(buffer) {
    return 0x03 === flashMode(buffer);
}

/**
 * @param {Event} event
 */
function notifyValueError(event) {
    notifyMessage(`ERROR while attempting OTA upgrade - XHR ${event.type}`);
}

/**
 * @param {Event} event
 */
function onButtonClick(event) {
    event.preventDefault();

    if (!(event.target instanceof HTMLButtonElement)) {
        return;
    }

    const url = event.target.dataset["url"];
    if (!url) {
        alert("Not connected");
        return;
    }

    withUpgrade((upgrade) => {
        const files = upgrade.files;
        if (!files) {
            alert("No file selected");
            return;
        }

        const data = new FormData();
        data.append("upgrade", files[0], files[0].name);

        const xhr = new XMLHttpRequest();

        xhr.addEventListener("error", notifyValueError, false);
        xhr.addEventListener("abort", notifyValueError, false);

        xhr.addEventListener("load",
            () => {
                if ("OK" === xhr.responseText) {
                    alert("Firmware image uploaded, board rebooting. This page will be refreshed in 5 seconds");
                } else {
                    alert(`ERROR while attempting OTA upgrade - ${xhr.status.toString()} ${xhr.statusText}, ${xhr.responseText}`);
                }
            }, false);

        withProgress((progress) => {
            xhr.addEventListener("load",
                () => {
                    progress.style.display = "none";
                });
            xhr.upload.addEventListener("progress",
                (event) => {
                    progress.style.display = "inherit";
                    if (event.lengthComputable) {
                        progress.value = event.loaded;
                        progress.max = event.total;
                    }
                }, false);
        });

        xhr.open("POST", url);
        xhr.send(data);
    });
}

/**
 * @param {number} size
 * @returns {number}
 */
function roundedSize(size) {
    return (size - (size % 4096)) + 4096;
}

/**
 * @param {Event} event
 */
async function onFileChanged(event) {
    event.preventDefault();

    const button = document.querySelector(".button-upgrade");
    if (!(button instanceof HTMLButtonElement)) {
        return;
    }

    button.disabled = true;

    if (!(event.target instanceof HTMLInputElement)) {
        return;
    }

    if (!event.target.files || !event.target.files.length) {
        return;
    }

    const file = event.target.files[0];

    const filename = document.querySelector("input[name=filename]");
    if (filename instanceof HTMLInputElement) {
        filename.value = file.name;
    }

    const need = roundedSize(file.size);
    if ((__free_size !== 0) && (need > __free_size)) {
        alert(`OTA .bin cannot be uploaded. Need at least ${need}bytes of free space, ${__free_size}bytes available.`);
        return;
    }

    const buffer = await file.slice(0, 3).arrayBuffer();
    const header = new Uint8Array(buffer);

    if (!isGzip(header)) {
        if (!checkMagic(header)) {
            alert("Invalid binary header, does not look like firmware .bin");
            return;
        }

        if (!checkFlashMode(header)) {
            alert(describeFlashMode(flashMode(header)));
        }
    }

    button.disabled = false;
}

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "free_size": (_, value) => {
            __free_size = parseInt(value, 10);
        },
    };
}

export function init() {
    variableListeners(listeners());

    const [upgrade] = document.querySelectorAll("input[name=upgrade]");
    if (!(upgrade instanceof HTMLInputElement)) {
        return;
    }

    const [browse] = document.querySelectorAll(".button-upgrade-browse")
    browse.addEventListener("click", () => {
        upgrade.click();
    });

    upgrade.addEventListener("change", onFileChanged);

    const button = document.querySelector(".button-upgrade");
    if (!(button instanceof HTMLButtonElement)) {
        return;
    }

    listenAppConnected((urls) => {
        button.dataset["url"] = urls.upgrade.href;
    });

    button.addEventListener("click", onButtonClick);
    button.disabled = true;
}
