import { send, sendAction } from './connection.mjs';
import { variableListeners } from './settings.mjs';

class CmdOutputBase {
    /** @param {HTMLTextAreaElement} elem */
    constructor(elem) {
        /** @type {HTMLTextAreaElement} */
        this.elem = elem;

        /** @type {number} */
        this.childrenMax = 4096;

        /** @type {number} */
        this.lastScrollHeight = 0;

        /** @type {number} */
        this.lastScrollTop = 0;

        /** @type {boolean} */
        this.followScroll = true;

        this.attach();
    }

    attach() {
        this.lastScrollHeight = this.elem.scrollHeight;
        this.lastScrollTop = this.elem.scrollTop;

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

    clearFirst() {
        while ((this.elem.childNodes.length > this.childrenMax) && this.elem.firstChild) {
            this.elem.removeChild(this.elem.firstChild);
        }
    }

    /** @param {string} line */
    push(line) {
        this.elem.appendChild(new Text(line));
        this.clearFirst();
    }

    /** @param {string} line */
    pushAndFollow(line) {
        this.elem.appendChild(new Text(`${line}\n`));
        this.followScroll = true;
        this.clearFirst();
        this.follow();
    }

    /** @param {string[]} lines */
    pushLines(lines) {
        for (const line of lines) {
            this.push(line);
        }

        this.follow();
    }
}

/**
 * @param {CmdOutputBase} output
 */
function onFormSubmit(output) {
    /**
     * @param {Event} event
     */
    return function(event) {
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

        output.pushAndFollow(value);
        sendAction("cmd", {"line": `${value}\n`});
    };
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
    const elem = document.getElementById("cmd-output");
    if (!(elem instanceof HTMLTextAreaElement)) {
        return;
    }

    const output = new CmdOutputBase(elem);

    variableListeners({
        "log": (_, value) => {
            send("{}");
            output.pushLines(value);
        },
    });

    document.querySelector(".button-dbg-clear")
        ?.addEventListener("click", (event) => {
            event.preventDefault();
            output.clear();
        });

    document.forms.namedItem("form-debug")
        ?.addEventListener("submit", onFormSubmit(output));
    document.querySelectorAll("form:not([name='form-debug'])")
        .forEach((form) => {
            form.addEventListener("submit", disableFormSubmit);
        });
}
