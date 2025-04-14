import { randomString } from './core.mjs';
import { setChangedElement } from './settings/utils.mjs';

function randomApiKey() {
    const form = document.forms.namedItem("form-admin");
    if (!form) {
        return;
    }

    const elem = form.elements.namedItem("apiKey");
    if (!(elem instanceof HTMLInputElement)) {
        return;
    }

    elem.value = randomString(16, {hex: true});
    setChangedElement(elem);
}

export function init() {
    document.querySelector(".button-apikey")
        ?.addEventListener("click", randomApiKey);
}
