import { sendAction } from './connection.mjs';

import { listenEnumerableName } from './settings/enumerable.mjs';
import { mergeTemplate } from './settings/template.mjs';
import { setInputValue } from './settings/input.mjs';
import { setOriginalsFromValues } from './settings/dataset.mjs';

import { variableListeners } from './settings.mjs';
import { loadConfigTemplate } from './template.mjs';

/** @param {Event} event */
function onButtonPress(event) {
    if (!(event.target instanceof HTMLElement)) {
        return;
    }

    const prefix = "button-rfb-";
    const [buttonRfbClass] = Array.from(event.target.classList)
        .filter(x => x.startsWith(prefix));

    if (!buttonRfbClass) {
        return;
    }

    const container = event.target?.parentElement?.parentElement;
    if (!container) {
        return;
    }

    const input = container.querySelector("input");
    if (!input || !input.dataset["id"]) {
        return;
    }

    sendAction(`rfb${buttonRfbClass.replace(prefix, "")}`, {
        id: parseInt(input.dataset["id"], 10),
        status: input.name === "rfbON"
    });
}

function addNode() {
    const container = document.getElementById("rfbNodes");
    if (!container) {
        return;
    }

    const id = container.childElementCount.toString();
    const line = loadConfigTemplate("rfb-node");

    const [span] = line.querySelectorAll("span");
    span.dataset["enumerableId"] = id.toString();
    listenEnumerableName(span, "relay");

    line.querySelectorAll("input")
        .forEach((input) => {
            input.dataset["id"] = id;
            input.setAttribute("id", `${input.name}${id}`);
        });

    ["learn", "forget"]
        .map((x) => line.querySelector(`.button-rfb-${x}`))
        .filter((x) => x instanceof HTMLButtonElement)
        .forEach((button) => {
            button.addEventListener("click", onButtonPress);
        });

    mergeTemplate(container, line);
}

/**
 * @typedef {[string, string]} CodePair
 * @param {CodePair[]} pairs
 * @param {!number} start
 */
function handleCodePairs(pairs, start) {
    pairs.forEach((pair, id) => {
        const realId = id + (start ?? 0);
        const [off, on] = pair;

        const rfbOff = /** @type {!HTMLInputElement} */
            (document.getElementById(`rfbOFF${realId}`));
        setInputValue(rfbOff, off);

        const rfbOn = /** @type {!HTMLInputElement} */
            (document.getElementById(`rfbON${realId}`));
        setInputValue(rfbOn, on);

        setOriginalsFromValues([rfbOn, rfbOff]);
    });
}

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "rfbCount": (_, value) => {
            for (let i = 0; i < value; ++i) {
                addNode();
            }
        },
        "rfb": (_, value) => {
            handleCodePairs(value.codes, value.start);
        },
    };
}

export function init() {
    variableListeners(listeners());
}
