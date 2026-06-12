import {
    MODULE_API,
    MODULE_CMD,
    MODULE_CURTAIN,
    MODULE_DBG,
    MODULE_DCZ,
    MODULE_DEV,
    MODULE_GARLAND,
    MODULE_HA,
    MODULE_LED,
    MODULE_LIGHT,
    MODULE_LIGHTFOX,
    MODULE_OTA,
    MODULE_RELAY,
    MODULE_RFB,
    MODULE_RFM69,
    MODULE_RPN,
    MODULE_SCH,
    MODULE_SNS,
    MODULE_THERMOSTAT,
    MODULE_TSPK,
} from '@build-preset/constants.mjs';

import { notifyError, notifyErrorEvent } from './notify.mjs';
window.addEventListener("error", (event) => {
    notifyErrorEvent(event);
    console.error(event.error);
});

import {
    onMenuLinkClick,
    onPanelTargetClick,
    pageReloadIn,
    showPanelByName,
    styleInject,
} from './core.mjs';

import { askAndCall } from './question.mjs';

import {
    init as initSettings,
    askSaveSettings,
    updateVariables,
    variableListeners,
} from './settings.mjs';
import { init as initPassword } from './password.mjs';

import { init as initWiFi } from './wifi.mjs';
import { init as initGpio } from './gpio.mjs';
import {
    init as initConnection,
    connect,
    sendAction,
} from './connection.mjs';

import { init as initApi } from './api.mjs';
import { init as initCurtain } from './curtain.mjs';
import { init as initDebug } from './debug.mjs';
import { init as initDomoticz } from './domoticz.mjs';
import { init as initGarland } from './garland.mjs';
import { init as initHa } from './ha.mjs';
import { init as initLed } from './led.mjs';
import { init as initLight } from './light.mjs';
import { init as initLightfox } from './lightfox.mjs';
import { init as initNtp } from './ntp.mjs';
import { init as initOta } from './ota.mjs';
import { init as initRelay } from './relay.mjs';
import { init as initRfbridge } from './rfbridge.mjs';
import { init as initRfm69 } from './rfm69.mjs';
import { init as initRules } from './rules.mjs';
import { init as initSchedule } from './schedule.mjs';
import { init as initSensor } from './sensor.mjs';
import { init as initThermostat } from './thermostat.mjs';
import { init as initThingspeak } from './thingspeak.mjs';

import { init as initDev } from './dev.mjs';
import { init as initTasmota } from './tasmota.mjs';

/** @type {number | null} */
let KeepTime = null;

/** @type {number} */
let Ago = 0;

/**
 * @typedef {{date: Date | null, offset: string}} NowType
 * @type NowType
 */
const Now = {
    date: null,
    offset: "",
};

/**
 * @type {{[k: string]: string}}
 */
const __title_cache = {
    hostname: "?",
    app_name: "ESPurna",
    app_version: "0.0.0",
};

/**
 * @param {string} key
 * @param {string} value
 */
function documentTitle(key, value) {
    __title_cache[key] = value;
    document.title = `${__title_cache.hostname} - ${__title_cache.app_name} ${__title_cache.app_version}`;
}

/**
 * @param {string} module
 */
function moduleVisible(module) {
    styleInject([`.module-${module} { display: revert; }`]);
}

/**
 * @param {string[]} modules
 */
function modulesVisible(modules) {
    modules.forEach((module) => {
        moduleVisible(module);
    });
}

function modulesVisibleAll() {
    document.querySelectorAll("[class*=module-]")
        .forEach((elem) => {
            /** @type {HTMLElement} */(elem).style.display = "revert";
        });
}

/**
 * @param {string} value
 */
function deviceNow(value) {
    try {
        Now.date = normalizedDate(value);
        Now.offset = timestampOffset(value);
    } catch (e) {
        notifyError(/** @type {Error} */(e));
    }
}

/**
 * @param {string} value
 */
function onAction(value) {
    if ("reload" === value) {
        pageReloadIn(1000);
    }
}

/**
 * @param {string} value
 */
function onMessage(value) {
    window.alert(value);
}

/**
 * @param {number} value
 */
function initWebMode(value) {
    const layout = /** @type {!HTMLElement} */
        (document.getElementById("layout"));
    layout.style.display = "inherit";

    if (1 === value) {
        layout.classList.add("initial");
    }

    showPanelByName(
        (1 === value) ? "password" : "status");
}

/**
 * @param {string} value
 * @returns {string}
 */
function timestampDatetime(value) {
    return value.slice(0, 19);
}

/**
 * @param {string} value
 * @returns {string}
 */
function timestampOffset(value) {
    if (value.endsWith("Z")) {
        return "Z";
    }

    return value.slice(-6);
}

/**
 * @param {NowType} now
 * @returns {string}
 */
function displayDatetime(now) {
    if (now.date) {
        let datetime = timestampDatetime(now.date.toISOString());
        datetime = datetime.replace("T", " ");
        datetime = `${datetime} ${now.offset}`;
        return datetime;
    }

    return "?";
}

/**
 * @param {string} value
 * @returns {string}
 */
function normalizedTimestamp(value) {
    return `${timestampDatetime(value)}Z`;
}

/**
 * @param {string} value
 * @returns {Date}
 */
function normalizedDate(value) {
    return new Date(normalizedTimestamp(value));
}


function keepTime() {
    const ago = document.querySelector("span[data-key='app:ago']");
    if (ago) {
        ago.textContent = Ago.toString();
    }

    ++Ago;

    if (null !== Now.date) {
        const now = document.querySelector("span[data-key='app:now']");
        if (now) {
            now.textContent = displayDatetime(Now);
        }

        Now.date = new Date(Now.date.valueOf() + 1000);
    }
}

/** @import { QuestionWrapper } from './question.mjs' */

/** @type {QuestionWrapper} */
function askDisconnect(ask) {
    return ask("Are you sure you want to disconnect from the current WiFi network?");
}

/** @type {QuestionWrapper} */
function askReboot(ask) {
    return ask("Are you sure you want to reboot the device?");
}

/** @param {CloseEvent} event */
function askReload(event) {
    /** @type {QuestionWrapper} */
    return function(ask) {
        return ask(`Connection lost with the device - ${event.reason}. Click OK to refresh the page.`);
    };
}

function askAndCallReconnect() {
    askAndCall([askSaveSettings, askDisconnect], () => {
        sendAction("reconnect");
    });
}

function askAndCallReboot() {
    askAndCall([askSaveSettings, askReboot], () => {
        sendAction("reboot");
    });
}

/** @param {Event} event */
function askAndCallSimpleAction(event) {
    const target = event.target;
    if (!(target instanceof HTMLButtonElement)) {
        return;
    }

    /** @type {QuestionWrapper} */
    const wrapper =
        (ask) => ask(`Confirm the action: "${target.textContent}"`);

    askAndCall([wrapper], () => {
        sendAction(target.name);
    });
}

// TODO https://github.com/microsoft/TypeScript/issues/58969
// at-import'ed type becomes 'unused' for some reason
// until fixed, prefer direct import vs. typedef

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "action": (_, value) => {
            onAction(value);
        },
        "app_name": documentTitle,
        "app_version": documentTitle,
        "hostname": documentTitle,
        "message": (_, value) => {
            onMessage(value);
        },
        "modulesVisible": (_, value) => {
            modulesVisible(value);
        },
        "now": (_, value) => {
            deviceNow(value);
        },
        "webMode": (_, value) => {
            initWebMode(value);
        },
    };
}

/**
 * @param {CloseEvent} event
 */
function onConnectionClose(event) {
    askAndCall([askReload(event)], () => {
        pageReloadIn(1000);
    });
}

/**
 * @param {MessageEvent<any>} event
 */
function onJsonPayload(event) {
    Ago = 0;

    if (!KeepTime) {
        KeepTime = window.setInterval(keepTime, 1000);
    }

    try {
        const parsed = JSON.parse(
            event.data
                .replace(/:Infinity/g, ':"inf"')
                .replace(/:-Infinity/g, ':"-inf"')
                .replace(/:NaN/g, ':"nan"')
                .replace(/\n/g, "\\n")
                .replace(/\r/g, "\\r")
                .replace(/\t/g, "\\t"));
        updateVariables(parsed);
    } catch (e) {
        notifyError(/** @type {Error} */(e));
    }
}

async function init() {
    // Sidebar menu & buttons
    document.querySelector(".menu-link")
        ?.addEventListener("click", onMenuLinkClick);
    document.querySelectorAll("[data-panel]")
        .forEach((elem) => {
            elem.addEventListener("click", onPanelTargetClick);
        });

    document.querySelector(".button-reconnect")
        ?.addEventListener("click", askAndCallReconnect);
    document.querySelectorAll(".button-reboot")
        .forEach((elem) => {
            elem.addEventListener("click", askAndCallReboot);
        });

    // Generic action sender
    document.querySelectorAll(".button-simple-action")
        .forEach((elem) => {
            elem.addEventListener("click", askAndCallSimpleAction);
        });

    variableListeners(listeners());

    if (!MODULE_DEV) {
        initConnection();
    }

    initSettings();
    initPassword();
    initWiFi();
    initGpio();
    initNtp();
    initTasmota();

    if (MODULE_OTA) {
        initOta();
    }

    if (MODULE_HA) {
        initHa();
    }

    if (MODULE_SNS) {
        initSensor();
    }

    if (MODULE_GARLAND) {
        initGarland();
    }

    if (MODULE_THERMOSTAT) {
        initThermostat();
    }

    if (MODULE_LIGHTFOX) {
        initLightfox();
    }

    if (MODULE_RELAY) {
        initRelay();
    }

    if (MODULE_RFM69) {
        initRfm69();
    }

    if (MODULE_RFB) {
        initRfbridge();
    }

    if (MODULE_CMD || MODULE_DBG) {
        initDebug();
    }

    if (MODULE_API) {
        initApi();
    }

    if (MODULE_LED) {
        initLed();
    }

    if (MODULE_LIGHT) {
        initLight();
    }

    if (MODULE_SCH) {
        initSchedule();
    }

    if (MODULE_RPN) {
        initRules();
    }

    if (MODULE_RELAY && MODULE_DCZ) {
        initDomoticz();
    }

    if (MODULE_RELAY && MODULE_TSPK) {
        initThingspeak();
    }

    if (MODULE_CURTAIN) {
        initCurtain();
    }

    if (MODULE_DEV) {
        initDev();
        KeepTime = window.setInterval(keepTime, 1000);
        modulesVisibleAll();
        return;
    }

    // don't autoconnect w/ localhost or file://
    connect({onclose: onConnectionClose, onmessage: onJsonPayload});
}

document.addEventListener("DOMContentLoaded", init);
