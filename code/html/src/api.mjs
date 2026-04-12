import { randomString } from './core.mjs';

function randomApiKey() {
    const elem = document
        ?.forms?.namedItem("form-api")
        ?.elements?.namedItem("apiKey");
    if (!(elem instanceof HTMLInputElement)) {
        return;
    }

    elem.value = randomString(16, {hex: true});
    elem.dispatchEvent(new Event("change"));
}

export function init() {
    document.querySelector(".button-apikey")
        ?.addEventListener("click", randomApiKey);
}
