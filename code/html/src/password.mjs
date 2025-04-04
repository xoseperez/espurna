import {
    randomString,
} from './core.mjs';

import { validatePassword, validateFormsPasswords } from './validate.mjs';

import {
    applySettingsFromForms,
    resetChangedElement,
    listenVariables,
} from './settings.mjs';

const FORM_SETUP_PASSWORD = "form-setup-password";

/** @param {function(HTMLFormElement): void} callback */
export function withForm(callback) {
    callback(/** @type {!HTMLFormElement} */
        (document.forms.namedItem(FORM_SETUP_PASSWORD)));
}

/**
 * @param {HTMLFormElement[]} forms
 * @returns {HTMLFormElement[]}
 */
export function filterForm(forms) {
    return forms.filter(
        (x) => x.id === FORM_SETUP_PASSWORD);
}

/**
 * @typedef {[HTMLInputElement, HTMLInputElement]} PasswordInputPair
 */

/**
 * @param {HTMLFormElement} form
 * @returns {PasswordInputPair | []}
 */
export function formPassPair(form) {
    const out = ["adminPass0", "adminPass1"]
        .map((x) => form.elements.namedItem(x))
        .filter((x) => x instanceof HTMLInputElement);

    if (out.length === 2) {
        return [out[0], out[1]];
    }

    return [];
}

/**
 * @returns {string}
 */
function generatePassword() {
    let password = "";
    do {
        password = randomString(10, {special: true});
    } while (!validatePassword(password));

    return password;
}

/**
 * @param {HTMLFormElement} form
 */
function generatePasswordsForForm(form) {
    const value = generatePassword();
    for (let elem of formPassPair(form)) {
        elem.type = "text";
        elem.value = value;
        elem.dispatchEvent(new Event("change"));
    }
}

/**
 * @param {HTMLFormElement} form
 */
function clearPasswordsForForm(form) {
    for (let elem of formPassPair(form)) {
        elem.type = "password";
        elem.value = "";
        resetChangedElement(elem);
    }
}

/**
 * @param {HTMLFormElement} form
 */
function initSetupPassword(form) {
    document.querySelectorAll(".button-setup-password")
        .forEach((elem) => {
            elem.addEventListener("click", (event) => {
                event.preventDefault();

                const target = /** @type {!HTMLButtonElement} */
                    (event.target);
                const strict = target.classList
                    .contains("button-setup-strict");

                const forms = [form];
                if (validateFormsPasswords(forms, {strict, assumeChanged: true})) {
                    applySettingsFromForms(forms);
                }
            });
        });

    document.querySelector(".button-generate-password")
        ?.addEventListener("click", (event) => {
            event.preventDefault();
            generatePasswordsForForm(form);
        });

    listenVariables("saved", (_, value) => {
        if (value !== true) {
            return;
        }

        clearPasswordsForForm(form);
    });
}

/**
 * @param {Event} event
 */
function onPasswordRevealClick(event) {
    const target = event.target;
    if (!(target instanceof HTMLElement)) {
        return;
    }

    const input = target.previousElementSibling;
    if (!(input instanceof HTMLInputElement)) {
        return;
    }

    if (input.type === "password") {
        input.type = "text";
    } else {
        input.type = "password";
    }
}

/**
 * @param {Element | DocumentFragment} container
 */
export function passwordReveal(container) {
    container.querySelectorAll(".password-reveal")
        .forEach((elem) => {
            elem.addEventListener("click", onPasswordRevealClick);
        });
}

export function init() {
    withForm((form) => {
        initSetupPassword(form);
    });

    passwordReveal(document.body);
}
