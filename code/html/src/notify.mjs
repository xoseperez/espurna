import { formatErrorEvent } from './errors.mjs';
import { loadTemplate, mergeTemplate } from './settings/template.mjs';

/** @param {string} message */
export function notifyMessage(message) {
    showNotification(
        formatErrorEvent(message, "", 0, 0, null));
}

/** @param {Error} error */
export function notifyError(error) {
    showNotification(
        formatErrorEvent("", "", 0, 0, error));
}

/** @param {ErrorEvent} event */
export function notifyErrorEvent(event) {
    showNotification(
        formatErrorEvent(
            event.message,
            event.filename,
            event.lineno,
            event.colno,
            event.error));
}

/** @type {number} */
let __errors = 0;

/**
 * @param {string} text
 */
export function showNotification(text) {
    const container = document.getElementById("error-notification");
    if (!container) {
        return;
    }

    __errors += 1;
    text += "\n\nFor more info see the Debug Log and / or Developer Tools console.";

    if (0 === container.childElementCount) {
        container.classList.add("show-error");
        mergeTemplate(
            container, loadTemplate("error-notification"));
    }

    container.children[0].textContent = text;
    if (0 !== container.childElementCount) {
        container.children[1].textContent =
            `\n(${__errors} unhandled errors so far)`;
    }
}

