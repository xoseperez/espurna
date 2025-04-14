import { randomString } from './core.mjs';

import {
    applySettingsFromForms,
    listenVariables,
} from './settings.mjs';
import { resetChangedElement } from './settings/utils.mjs';

import {
    formPassPair,
    validatePassword,
    withForm,
} from './password/utils.mjs';

import { validateFormsPasswords } from './validate.mjs';

/**
 * @typedef {[HTMLInputElement, HTMLInputElement]} PasswordInputPair
 */

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
