import { addSimpleEnumerables } from './settings/enumerable.mjs';
import { mergeTemplate } from './settings/template.mjs';
import { variableListeners } from './settings.mjs';

import { sendAction } from './connection.mjs';
import { loadConfigTemplate } from './template.mjs';

const BACKGROUND_MOVING = "rgb(192, 0, 0)";
const BACKGROUND_STOPPED = "rgb(64, 184, 221)";

/** @param {Event} event */
function buttonHandler(event) {
    if (event.type !== "click") {
        return;
    }

    if (!(event.target instanceof HTMLButtonElement)) {
        return;
    }

    event.preventDefault();

    let code = -1;

    const list = event.target.classList;
    if (list.contains("button-curtain-pause")) {
        code = 0;
    } else if (list.contains("button-curtain-open")) {
        code = 1;
    } else if (list.contains("button-curtain-close")) {
        code = 2;
    }

    if (code >= 0) {
        sendAction("curtainAction", {button: code});
        event.target.style.background = "red";
    }
}

/**
 * @param {boolean} moving
 * @param {number} button
 */
function styleButtons(moving, button) {
    const elems = /** @type {NodeListOf<HTMLButtonElement>} */
        (document.querySelectorAll("curtain-button"));
    if (!moving || (0 === button)) {
        if (!moving) {
            elems.forEach((elem) => {
                elem.style.background = BACKGROUND_STOPPED;
            });
        }
        if (0 === button) {
            elems.forEach((elem) => {
                if (elem.classList.contains("button-curtain-pause")) {
                    elem.style.background = BACKGROUND_MOVING;
                }
            });
        }

        return;
    }

    elems.forEach((elem) => {
        if (elem.classList.contains("button-curtain-open")) {
            elem.style.background =
                (1 === button) ? BACKGROUND_MOVING :
                (2 === button) ? BACKGROUND_STOPPED :
                    BACKGROUND_STOPPED;
        } else if (elem.classList.contains("button-curtain-close")) {
            elem.style.background =
                (1 === button) ? BACKGROUND_STOPPED :
                (2 === button) ? BACKGROUND_MOVING :
                    BACKGROUND_STOPPED;
        }
    });

}

/** @param {Event} event */
function positionHandler(event) {
    if (!(event.target instanceof HTMLButtonElement)) {
        return;
    }

    sendAction("curtainAction", {position: event.target.value});
}

//Create the controls for one curtain. It is called when curtain is updated (so created the first time)
//Let this there as we plan to have more than one curtain per switch
function initCurtain() {
    const container = document.getElementById("curtains");
    if (!container || container.childElementCount > 0) {
        return;
    }

    // add and init curtain template, prepare multi switches
    const line = loadConfigTemplate("curtain-control");
    ["open", "pause", "close"]
        .forEach((name) => {
            line.querySelector(`.button-curtain-${name}`)
                ?.addEventListener("click", buttonHandler);
        });

    mergeTemplate(container, line);

    // simple position slider
    document.getElementById("curtainSet")
        ?.addEventListener("change", positionHandler);

    addSimpleEnumerables("curtain", "Curtain", 1);
}

/** @param {function(HTMLInputElement): void} callback */
function withSet(callback) {
    const elem = document.getElementById("curtain-set");
    if (elem instanceof HTMLInputElement) {
        callback(elem);
    }
}

/** @param {function(HTMLElement): void} callback */
function withPicture(callback) {
    const elem = document.getElementById("curtain-picture");
    if (elem instanceof HTMLElement) {
        callback(elem);
    }
}

/**
 * @param {string} side_or_corner
 * @param {number} angle
 */
function setBackground(side_or_corner, angle) {
    withPicture((elem) => {
        elem.style.background =
            `linear-gradient(${side_or_corner}, black ${angle}%, #a0d6ff ${angle}%)`;
    });
}

/**
 * @param {number} angle
 * @param {number} hint_angle
 */
function setBackgroundTwoSides(angle, hint_angle) {
    withPicture((elem) => {
        elem.style.background =
            `linear-gradient(90deg, black ${angle}%, #a0d6ff ${angle}% ${hint_angle}%, black ${hint_angle}%)`;
    });
}

/** @typedef {{get: number, set: number, button: number, moving: boolean, type: string}} CurtainValue */

/** @param {CurtainValue} value */
function updateCurtain(value) {
    switch (value.type) {
    case '1': //One side left to right
        setBackground('90deg', value.get);
        break;
    case '2': //One side right to left
        setBackground('270deg', value.get);
        break;
    case '3': //Two sides
        setBackgroundTwoSides(value.get / 2, (100 - value.get / 2));
        break;
    case '0': //Roller
    default:
        setBackground('180deg', value.get);
        break;
    }

    withSet((elem) => {
        elem.value = value.set.toString();
    });

    styleButtons(value.moving, value.button);
}

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "curtainState": (_, value) => {
            initCurtain();
            updateCurtain(value);
        },
    };
}

export function init() {
    variableListeners(listeners());
}
