/**
 * @param {string[]} rules
 */
export function styleInject(rules) {
    if (!rules.length) {
        return;
    }

    const style = document.createElement("style");
    style.setAttribute("type", "text/css");
    document.head.appendChild(style);

    const sheet = style.sheet;
    if (!sheet) {
        return;
    }

    let pos = sheet.cssRules.length;
    for (let rule of rules) {
        style.sheet.insertRule(rule, pos++);
    }
}

/**
 * @param {string} selector
 * @param {boolean} value
 */
export function styleVisible(selector, value) {
    return `${selector} { content-visibility: ${value ? "visible": "hidden"}; }`
}

/**
 * @param {number} timeout
 */
export function pageReloadIn(timeout) {
    setTimeout(() => window.location.reload(), timeout);
}

/**
 * @param {HTMLElement} container
 */
function moreElem(container) {
    container.querySelectorAll(".more")
        .forEach((elem) => {
            if (!(elem instanceof HTMLElement)) {
                return;
            }

            elem.style.display = (elem.style.display === "")
                ? "inherit" : "";
        });
}

/**
 * @param {Event} event
 */
export function onMoreParent(event) {
    event.preventDefault();
    event.stopPropagation();

    const target = event.target;
    if (!(target instanceof HTMLElement)) {
        return;
    }

    const parent = target?.parentElement?.parentElement;
    if (parent) {
        moreElem(parent);
    }
}

/**
 * @param {Element | DocumentFragment} container
 */
export function moreParent(container) {
    for (let elem of container.querySelectorAll("button.button-more-parent")) {
        elem.addEventListener("click", onMoreParent);
    }
}

/**
 * @param {HTMLElement} elem
 */
export function lastMoreElem(elem) {
    if (elem.lastChild instanceof HTMLElement) {
        moreElem(elem.lastChild)
    }
}

/**
 * @param {HTMLElement} elem
 */
function menuToggle(elem) {
    elem.classList.toggle("active");
}

/**
 * @param {HTMLElement} elem
 */
function menuHide(elem) {
    elem.classList.remove("active");
}

/**
 * @param {Event} event
 * @returns {any}
 */
export function onMenuLinkClick(event) {
    event.preventDefault();

    const target = event.target;
    if (!(target instanceof HTMLElement)) {
        return;
    }

    if (target.parentElement) {
        menuToggle(target.parentElement);
    }
}

/**
 * @param {HTMLElement} elem
 */
export function showPanel(elem) {
    if (elem.style.display !== "revert") {
        for (const panel of document.getElementsByClassName("panel")) {
            if (!(panel instanceof HTMLElement)) {
                continue;
            }

            panel.style.display = "none";
        }

        elem.style.display = "revert";
    }

    const layout = document.getElementById("layout")
    if (layout) {
        menuHide(layout);
    }

    // TODO: sometimes, switching view causes us to scroll past
    // the header (e.g. emon ratios panel on small screen)
    // layout itself stays put, but the root element seems to scroll,
    // at least can be reproduced with Chrome
    if (document.documentElement) {
        document.documentElement.scrollTop = 0;
    }
}

/**
 * @param {string} name
 */
export function showPanelByName(name) {
    // only a single panel is shown on the 'layout'
    const panel = document.getElementById(`panel-${name}`);
    if (!panel) {
        return;
    }

    showPanel(panel);
}

/**
 * @param {Event} event
 */
export function onPanelTargetClick(event) {
    event.preventDefault();

    const target = event.target;
    if (!(target instanceof HTMLElement)) {
        return;
    }

    const name = target.dataset["panel"];
    if (name) {
        showPanelByName(name);
    }

    panelTargetShowSelected(target);
}

/**
 * @param {HTMLElement} elem
 */
function panelTargetShowSelected(elem) {
    const root = elem.closest("#menu");
    if (!root) {
        return;
    }

    const parent = elem.parentElement;
    if (!parent) {
        return;
    }

    parent.classList.add("pure-menu-selected");

    /** @type {NodeListOf<HTMLAnchorElement>} */
    (root.querySelectorAll("#menu a[data-panel]"))
        .forEach((a) => {
            if (a.parentElement && a.parentElement !== parent) {
                a.parentElement.classList.remove("pure-menu-selected");
            }
        });
}

/**
 * @typedef {{hex?: boolean, lowercase?: boolean, numbers?: boolean, special?: boolean, uppercase?: boolean}} RandomStringOptions
 *
 * @param {number} length
 * @param {RandomStringOptions} options
 */
export function randomString(length, {hex = false, lowercase = true, numbers = true, special = false, uppercase = true} = {}) {
    let mask = "";
    if (lowercase || hex) { mask += "abcdef"; }
    if (lowercase) { mask += "ghijklmnopqrstuvwxyz"; }
    if (uppercase || hex) { mask += "ABCDEF"; }
    if (uppercase) { mask += "GHIJKLMNOPQRSTUVWXYZ"; }
    if (numbers || hex) { mask += "0123456789"; }
    if (special) { mask += "~`!@#$%^&*()_+-={}[]:\";'<>?,./|\\"; }

    const source = new Uint32Array(length);
    const result = new Array(length);

    window.crypto
        .getRandomValues(source)
        .forEach((value, i) => {
            result[i] = mask[value % mask.length];
        });

    return result.join("");
}

/**
 * @throws {Error}
 * @param {boolean} value
 * @param {string} message
 * @returns {asserts value}
 */
export function assert(value, message = "") {
    if (!value) {
        throw new Error(message ?? "assertion failed");
    }
}

/**
 * @template T
 * @param {T[]} values
 * @param {function(T): boolean} callback
 * @returns {number}
 */
export function count(values, callback) {
    return values.filter(callback).length;
}

/**
 * @param {string} value
 * @returns {string}
 */
export function capitalize(value) {
    return value === ""
        ? value
        : `${value.charAt(0).toUpperCase()}${value.slice(1)}`;
}
