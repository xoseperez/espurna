import { send, sendAction } from './connection.mjs';
import { variableListeners } from './settings.mjs';

class CmdOutputBase {
    constructor() {
        /** @type {HTMLTextAreaElement | null} */
        this.elem = null;

        /** @type {number} */
        this.lastScrollHeight = 0;

        /** @type {number} */
        this.lastScrollTop = 0;

        /** @type {boolean} */
        this.followScroll = true;
    }

    /** @param {HTMLTextAreaElement} elem */
    attach(elem) {
        this.elem = elem;

        this.lastScrollHeight = elem.scrollHeight;
        this.lastScrollTop = elem.scrollTop;

        this.elem.addEventListener("scroll", () => {
            if (!this.elem) {
                return;
            }

            // in case we adjust the scroll manually
            const current = this.elem.scrollHeight - this.elem.scrollTop;
            const last = this.lastScrollHeight - this.lastScrollTop;
            if ((current - last) > 16) {
                this.followScroll = false;
            }

            // ...and, in case we return to the bottom row
            const offset = current - this.elem.offsetHeight;
            if (offset < 16) {
                this.followScroll = true;
            }

            this.lastScrollHeight = this.elem.scrollHeight;
            this.lastScrollTop = this.elem.scrollTop;
        });
    }

    follow() {
        if (this.elem && this.followScroll) {
            this.elem.scrollTop = this.elem.scrollHeight;
            this.lastScrollHeight = this.elem.scrollHeight;
            this.lastScrollTop = this.elem.scrollTop;
        }
    }

    clear() {
        if (this.elem) {
            this.elem.textContent = "";
        }

        this.followScroll = true;
    }

    /** @param {string} line */
    push(line) {
        this?.elem?.appendChild(new Text(line));
    }

    /** @param {string} line */
    pushAndFollow(line) {
        this?.elem?.appendChild(new Text(`${line}\n`));
        this.followScroll = true
    }
}

const CmdOutput = new CmdOutputBase();

/**
 * @returns {import('./settings.mjs').KeyValueListeners}
 */
function listeners() {
    return {
        "log": (_, value) => {
            send("{}");

            for (const message of value) {
                CmdOutput.push(message);
            }

            CmdOutput.follow();
        },
    };
}

/** @param {Event} event */
function onFormSubmit(event) {
    event.preventDefault();

    if (!(event.target instanceof HTMLFormElement)) {
        return;
    }

    const cmd = event.target.elements
        .namedItem("cmd");
    if (!(cmd instanceof HTMLInputElement)) {
        return;
    }

    const value = cmd.value;
    cmd.value = "";

    CmdOutput.pushAndFollow(value);
    sendAction("cmd", {"line": `${value}\n`});
}

/**
 * While the settings are grouped using forms, actual submit is useless here
 * b/c the data is intended to be sent with the websocket connection and never through some http endpoint
 * *NOTICE* that manual event cancellation should happen asap, any exceptions will stop the specific
 * handler function, but will not stop anything else left in the chain.
 * @param {Event} event
 */
function disableFormSubmit(event) {
    event.preventDefault();
}

export function init() {
    variableListeners(listeners());

    const output = document.getElementById("cmd-output");
    if (output instanceof HTMLTextAreaElement) {
        CmdOutput.attach(output);
    }

    document.forms.namedItem("form-debug")
        ?.addEventListener("submit", onFormSubmit);
    document.querySelectorAll("form:not([name='form-debug'])")
        .forEach((form) => {
            form.addEventListener("submit", disableFormSubmit);
        });

    document.querySelector(".button-dbg-clear")
        ?.addEventListener("click", (event) => {
            event.preventDefault();
            CmdOutput.clear();
        });
}
