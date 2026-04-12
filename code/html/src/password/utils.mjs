/** @import { PasswordInputPair } from '../password.mjs' */

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

const FORM_INPUT_NAME = "adminPass[]";

/**
 * @param {HTMLFormElement} form
 * @returns {PasswordInputPair | []}
 */
export function formPassPair(form) {
    const items = form.elements.namedItem(FORM_INPUT_NAME);
    if ((items instanceof RadioNodeList)
     && (items.length === 2)
     && (items[0] instanceof HTMLInputElement)
     && (items[1] instanceof HTMLInputElement))
    {
        return [items[0], items[1]]
    }

    return [];
}

/**
 * @param {string} value
 * @returns {boolean}
 */
export function validatePassword(value) {
    // http://www.the-art-of-web.com/javascript/validate-password/
    // at least one lowercase and one uppercase letter or number
    // at least eight characters (letters, numbers or special characters)

    // MUST be 8..63 printable ASCII characters. See:
    // https://en.wikipedia.org/wiki/Wi-Fi_Protected_Access#Target_users_(authentication_key_distribution)
    // https://github.com/xoseperez/espurna/issues/1151

    const Pattern = /^(?=.*[A-Z\d])(?=.*[a-z])[\w~!@#$%^&*()<>,.?;:{}[\]\\|]{8,63}/;
    return ((typeof value === "string")
        && (value.length >= 8)
        && Pattern.test(value));
}
