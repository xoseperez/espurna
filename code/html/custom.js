var Debug = false;

class UrlsBase {
    constructor(root) {
        this.root = root;

        const paths = ["ws", "upgrade", "config", "auth"];
        paths.forEach((path) => {
            this[path] = new URL(path, root);
            this[path].protocol = root.protocol;
        });

        if (this.root.protocol === "https:") {
            this.ws.protocol = "wss:";
        } else {
            this.ws.protocol = "ws:";
        }
    }
}

var Urls = null;

var WebsockPingPong = null;
var Websock = {
    send: function() {
    },
    close: function() {
    }
};

class SettingsBase {
    constructor() {
        this.counters = {};
        this.counters.changed = 0;
        this.counters.reboot = 0;
        this.counters.reconnect = 0;
        this.counters.reload = 0;
        this.saved = false;
    }

    resetCounters() {
        this.counters.changed = 0;
        this.counters.reboot = 0;
        this.counters.reconnect = 0;
        this.counters.reload = 0;
    }
}

var Settings = new SettingsBase();

var Enumerable = {};

var FreeSize = 0;
var Now = 0;
var Ago = 0;

//removeIf(!light)
var ColorPicker;
//endRemoveIf(!light)

//removeIf(!rfm69)
var Rfm69 = {
    filters: {}
};
//endRemoveIf(!rfm69)

//removeIf(!sensor)
var Magnitudes = [];
var MagnitudeErrors = {};
var MagnitudeNames = {};
var MagnitudeTypePrefixes = {};
var MagnitudePrefixTypes = {};
//endRemoveIf(!sensor)

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------

function notifyError(message, source, lineno, colno, error) {
    let container = document.getElementById("error-notification");
    if (container.childElementCount > 0) {
        return;
    }

    container.style.display = "inherit";
    container.style.whiteSpace = "pre-wrap";

    let notification = document.createElement("div");
    notification.classList.add("pure-u-1");
    notification.classList.add("pure-u-lg-1");
    if (error) {
        notification.textContent += error.stack;
    } else {
        notification.textContent += message;
    }
    notification.textContent += "\n\nFor more info see the Developer Tools console.";
    container.appendChild(notification);

    return false;
}

window.onerror = notifyError;

function followScroll(elem, threshold) {
    if (threshold === undefined) {
        threshold = 90;
    }

    const offset = (elem.scrollTop + elem.offsetHeight) / elem.scrollHeight * 100;
    if (!threshold || (offset >= threshold)) {
        elem.scrollTop = elem.scrollHeight;
    }
}

function fromSchema(source, schema) {
    if (schema.length !== source.length) {
        throw `Schema mismatch! Expected length ${schema.length} vs. ${source.length}`;
    }

    var target = {};
    schema.forEach((key, index) => {
        target[key] = source[index];
    });

    return target;
}

function keepTime() {
    document.querySelector("span[data-key='ago']").textContent = Ago;
    ++Ago;

    if (0 === Now) {
        return;
    }

    let text = (new Date(Now * 1000))
        .toISOString().substring(0, 19)
        .replace("T", " ");

    document.querySelector("span[data-key='now']").textContent = text;
    ++Now;
}

function setUptime(value) {
    let uptime = parseInt(value, 10);

    let seconds = uptime % 60;
    uptime = parseInt(uptime / 60, 10);

    let minutes = uptime % 60;
    uptime = parseInt(uptime / 60, 10);

    let hours = uptime % 24;
    uptime = parseInt(uptime / 24, 10);

    let days = uptime;

    let container = document.querySelector("span[data-key='uptime']");
    container.textContent = days + "d " + zeroPad(hours, 2) + "h " + zeroPad(minutes, 2) + "m " + zeroPad(seconds, 2) + "s";

    Ago = 0;
}

function zeroPad(number, positions) {
    return number.toString().padStart(positions, "0");
}

function validatePassword(password) {
    // http://www.the-art-of-web.com/javascript/validate-password/
    // at least one lowercase and one uppercase letter or number
    // at least eight characters (letters, numbers or special characters)

    // MUST be 8..63 printable ASCII characters. See:
    // https://en.wikipedia.org/wiki/Wi-Fi_Protected_Access#Target_users_(authentication_key_distribution)
    // https://github.com/xoseperez/espurna/issues/1151

    const Pattern = /^(?=.*[A-Z\d])(?=.*[a-z])[\w~!@#$%^&*()<>,.?;:{}[\]\\|]{8,63}t/;
    return (
        (password !== undefined)
        && (typeof password === "string")
        && (password.length > 0)
        && Pattern.test(password));
}

// Try to validate 'adminPass{0,1}', searching the first form containing both.
// In case we on normal settings page, avoid checking things when both fields were not changed
// Allow to enforce validation for the setup page
function validateFormPasswords(forms, required) {
    let [passwords] = Array.from(forms).filter(
        form => form.elements.adminPass0 && form.elements.adminPass1);

    if (passwords) {
        let first = passwords.elements.adminPass0;
        let second = passwords.elements.adminPass1;

        if (!required && !first.value.length && !second.value.length) {
            return true;
        }

        let firstValid = first.checkValidity() && validatePassword(first.value);
        let secondValid = second.checkValidity() && validatePassword(second.value);
        if (firstValid && secondValid) {
            if (first.value === second.value) {
                return true;
            }

            alert("Passwords are different!");
            return false;
        }

        alert("The password you have entered is not valid, it must be 8..63 characters and have at least 1 lowercase and 1 uppercase / number!");
    }

    return false;
}

// Same as above, but only applies to the general settings page.
// Find the first available form that contains 'hostname' input
function validateFormHostname(forms) {
    // per. [RFC1035](https://datatracker.ietf.org/doc/html/rfc1035)
    // Hostname may contain:
    // - the ASCII letters 'a' through 'z' (case-insensitive),
    // - the digits '0' through '9', and the hyphen.
    // Hostname labels cannot begin or end with a hyphen.
    // No other symbols, punctuation characters, or blank spaces are permitted.
    let [hostname] = Array.from(forms).filter(form => form.elements.hostname);
    if (!hostname) {
        return true;
    }

    // Validation pattern is attached to the element itself, so just check that.
    // (and, we also re-use the hostname for fallback SSID, thus limited to 1...32 chars instead of 1...63)

    hostname = hostname.elements.hostname;
    let result = hostname.value.length
        && (!isChangedElement(hostname) || hostname.checkValidity());

    if (!result) {
        alert("Hostname cannot be empty and may only contain the ASCII letters ('A' through 'Z' and 'a' through 'z'), the digits '0' through '9', and the hyphen ('-')! They can neither start or end with an hyphen.");
    }

    return result;
}

function validateForms(forms) {
    return validateFormPasswords(forms) && validateFormHostname(forms);
}

// Right now, group additions happen from:
// - WebSocket, likely to happen exactly once per connection through processData handler(s). Specific keys trigger functions that append into the container element.
// - User input. Same functions are triggered, but with an additional event for the container element that causes most recent element to be marked as changed.
// Removal only happens from user input. MutationObserver will refresh checkboxes and cause everything to be marked as changed.
//
// TODO: distinguish 'current' state to avoid sending keys when adding and immediatly removing the latest node?
// TODO: previous implementation relied on defaultValue and / or jquery $(...).val(), but this does not really work where 'line' only has <select>

function groupSettingsHandleUpdate(event) {
    if (!event.target.children.length) {
        return;
    }

    let last = event.target.children[event.target.children.length - 1];
    for (let target of settingsTargets(event.target)) {
        let elem = last.querySelector(`[name='${target}']`);
        if (elem) {
            setChangedElement(elem);
        }
    }
}

function groupSettingsOnAdd(elementId, listener) {
    document.getElementById(elementId).addEventListener("settings-group-add", listener);
}

var GroupSettingsObserver = new MutationObserver((mutations) => {
    mutations.forEach((mutation) => {
        if (isChangedElement(mutation.target) || mutation.removedNodes.length) {
            setChangedForNode(mutation.target);
        }

        if (mutation.removedNodes.length) {
            updateCheckboxes(mutation.target);
        }
    });
});

// When receiving / returning data, <select multiple=true> <option> values are treated as bitset (u32) indexes (i.e. individual bits that are set)
// For example 0b101 is translated to ["0", "2"], or 0b1111 is translated to ["0", "1", "2", "3"]

function bitsetToSelectedValues(bitset) {
    let values = [];
    for (let index = 0; index < 31; ++index) {
        if (bitset & (1 << index)) {
            values.push(index.toString());
        }
    }

    return values;
}

function bitsetFromSelectedValues(select) {
    let result = 0;
    for (let option of select.selectedOptions) {
        result |= 1 << parseInt(option.value);
    }

    return result;
}

// <select data-original="..."> is read / saved as:
// - multiple=false -> value string of the selected option
// - multiple=true -> comma-separated values of all selected options
//
// If selectedIndex is -1, it means we never selected anything
// (TODO: could this actually happen with defaultValue'ed <select>?)

function stringifySelectedValues(select) {
    if (select.multiple) {
        return Array.from(select.selectedOptions)
            .map(option => option.value)
            .join(",");
    } else if (select.selectedIndex >= 0) {
        return select.options[select.selectedIndex].value;
    }

    return "";
}

function elementSelectorListener(selector, event, listener) {
    for (let elem of document.querySelectorAll(selector)) {
        elem.addEventListener(event, listener);
    }
}

function elementSelectorOnClick(selector, listener) {
    elementSelectorListener(selector, "click", listener);
}

function isChangedElement(elem) {
    return "true" === elem.dataset["changed"];
}

function setChangedElement(elem) {
    elem.dataset["changed"] = "true";
}

function setChangedForNode(node) {
    setChangedElement(node);
    for (let elem of node.querySelectorAll("input,select")) {
        setChangedElement(elem);
    }
}

function resetChangedElement(elem) {
    elem.dataset["changed"] = "false";
}

function resetChangedGroups() {
    const elems = document.getElementsByClassName("settings-group");
    for (let elem of elems) {
        resetChangedElement(elem);
    }
}

function isGroupElement(elem) {
    return elem.dataset["settingsGroupElement"] !== undefined;
}

function isIgnoredElement(elem) {
    return elem.dataset["settingsIngore"] !== undefined;
}

function settingsTargets(elem) {
    let targets = elem.dataset["settingsTarget"];
    if (!targets) {
        return [];
    }

    return targets.split(" ");
}

function stringToBoolean(value) {
    return (value === "1")
        || (value === "y")
        || (value === "yes")
        || (value === "true")
        || (value === "on");
}

function booleanToString(value) {
    return value ? "true" : "false";
}

function setInputValue(input, value) {
    switch (input.type) {
    case "checkbox":
    case "radio":
        input.checked =
            (typeof(value) === "boolean") ? value :
            (typeof(value) === "string") ? stringToBoolean(value) :
            (typeof(value) === "number") ? (value !== 0) : false;
        break;
    case "text":
    case "password":
    case "number":
        input.value = value;
        break;
    }
}

function setSpanValue(span, value) {
    if (Array.isArray(value)) {
        value.forEach((text) => {
            setSpanValue(span, text);
            span.appendChild(document.createElement("br"));
        });
    } else {
        let content = "";
        if (span.attributes.pre) {
            content += span.attributes.pre.value;
        }
        content += value;
        if (span.attributes.post) {
            content += span.attributes.post.value;
        }
        span.textContent = content;
    }
}

function setSelectValue(select, value) {
    let values = select.multiple
        ? bitsetToSelectedValues(value)
        : [value.toString()];

    Array.from(select.options)
        .filter((option) => values.includes(option.value))
        .forEach((option) => {
            option.selected = true;
        });
}

function getDataForElement(element) {
    switch (element.tagName) {
    case "INPUT":
        switch (element.type) {
        case "checkbox":
        case "radio":
            return element.checked ? 1 : 0;
        case "number":
        case "text":
        case "password":
        case "hidden":
        case "range":
            // notice that we set directly to storage, thus strings are just fine
            return element.value;
        }
        break;
    case "SELECT":
        if (element.multiple) {
            return bitsetFromSelectedValues(element);
        } else if (element.selectedIndex >= 0) {
            return element.options[element.selectedIndex].value;
        }
    }

    return null;
}

function getData(forms, changed, cleanup) {
    // Populate two sets of data, ones that had been changed and ones that stayed the same
    var data = {};
    var changed_data = [];
    if (cleanup === undefined) {
        cleanup = true;
    }

    if (changed === undefined) {
        changed = true;
    }

    for (let form of forms) {
        for (let elem of form.elements) {
            if ((elem.tagName !== "SELECT") && (elem.tagName !== "INPUT")) {
                continue;
            }

            if (isIgnoredElement(elem)) {
                continue;
            }

            if (elem.name === undefined) {
                continue;
            }

            let name = elem.dataset["settingsRealName"] || elem.name;
            let value = getDataForElement(elem);
            if (null !== value) {
                var indexed = changed_data.indexOf(name) >= 0;
                if ((isChangedElement(elem) || !changed) && !indexed) {
                    changed_data.push(name);
                }

                // make sure to group keys from templates (or, manually flagged as such)
                if (isGroupElement(elem)) {
                    if (name in data) {
                        data[name].push(value);
                    } else {
                        data[name] = [value];
                    }
                } else {
                    data[name] = value;
                }
            }
        }
    }

    // Finally, filter out only fields that had changed.
    // Note: We need to preserve dynamic lists like schedules, wifi etc.
    // so we don't accidentally break when user deletes entry in the middle
    var resulting_data = {};
    for (let value in data) {
        if (changed_data.indexOf(value) >= 0) {
            resulting_data[value] = data[value];
        }
    }

    // Hack: clean-up leftover arrays.
    // When empty, the receiving side will prune all keys greater than the current one.
    if (cleanup) {
        for (let group of document.getElementsByClassName("settings-group")) {
            if (isChangedElement(group) && !group.children.length) {
                settingsTargets(group).forEach((target) => {
                    resulting_data[target] = [];
                });
            }
        }
    }

    return resulting_data;

}

function randomString(length, args) {
    if (typeof args === "undefined") {
        args = {
            lowercase: true,
            uppercase: true,
            numbers: true,
            special: true
        }
    }

    var mask = "";
    if (args.lowercase) { mask += "abcdefghijklmnopqrstuvwxyz"; }
    if (args.uppercase) { mask += "ABCDEFGHIJKLMNOPQRSTUVWXYZ"; }
    if (args.numbers || args.hex) { mask += "0123456789"; }
    if (args.hex) { mask += "ABCDEF"; }
    if (args.special) { mask += "~`!@#$%^&*()_+-={}[]:\";'<>?,./|\\"; }

    var source = new Uint32Array(length);
    var result = new Array(length);

    window.crypto.getRandomValues(source).forEach((value, i) => {
        result[i] = mask[value % mask.length];
    });

    return result.join("");
}

function generateApiKey() {
    let elem = document.forms["form-admin"].elements.apiKey;
    elem.value = randomString(16, {hex: true});
    setChangedElement(elem);
}

function generatePassword() {
    let password = "";
    do {
        password = randomString(10);
    } while (!validatePassword(password));

    return password;
}

function toggleVisiblePassword(event) {
    let elem = event.target.previousElementSibling;
    if (elem.type === "password") {
        elem.type = "text";
    } else {
        elem.type = "password";
    }
}

function doGeneratePassword() {
    let value = generatePassword();

    let input = document.getElementById("form-setup-password");
    for (let elem of [input.elements.adminPass0, input.elements.adminPass1]) {
        setChangedElement(elem);
        elem.type = "text";
        elem.value = value;
    }
}

function moduleVisible(module) {
    const elems = document.getElementsByClassName(`module-${module}`);
    for (let elem of elems) {
        if (module === "sch") {
            switch (elem.tagName) {
            case "LI":
                elem.style.display = "inherit";
                break;
            case "DIV":
                elem.style.display = "flex";
                break;
            }
        } else {
            elem.style.display = "inherit";
        }
    }
}

// <select> initialization from simple {id: ..., name: ...} that map as <option> value=... and textContent
// To avoid depending on order of incoming messages, always store real value inside of dataset["original"] and provide a way to re-initialize every 'enumerable' <select> element on the page
//
// Notice that <select multiple> input and output format is u32 number, but the 'original' string is comma-separated <option> value=... attributes

function initSelect(select, values) {
    for (let value of values) {
        let option = document.createElement("option");
        option.setAttribute("value", value["id"]);
        option.textContent = value["name"];
        select.appendChild(option);
    }
}

function initEnumerableSelect(select, callback) {
    for (let className of select.classList) {
        const prefix = "enumerable-";
        if (className.startsWith(prefix)) {
            const name = className.replace(prefix, "");
            if ((Enumerable[name] !== undefined) && Enumerable[name].length) {
                callback(select, Enumerable[name]);
            }
            break;
        }
    }
}

function refreshEnumerableSelect(name) {
    const selector = (name !== undefined)
        ? `select.enumerable.enumerable-${name}`
        : "select.enumerable";

    for (let select of document.querySelectorAll(selector)) {
        initEnumerableSelect(select, (_, enumerable) => {
            while (select.childElementCount) {
                select.removeChild(select.firstElementChild);
            }

            initSelect(select, enumerable);

            let original = select.dataset["original"];
            if (original && original.length) {
                let options = original.split(",");
                for (let option of options) {
                    select.options[option].selected = true;
                }
            }
        });
    }
}

function addEnumerables(name, enumerables) {
    Enumerable[name] = enumerables;
    refreshEnumerableSelect(name);
}

function addSimpleEnumerables(name, prettyName, count) {
    if (count) {
        let enumerables = [];
        for (let id = 0; id < count; ++id) {
            enumerables.push({"id": id, "name": `${prettyName} #${id}`});
        }

        addEnumerables(name, enumerables);
    }
}

// Handle plain kv pairs when they are already on the page, and don't need special template handlers
// Notice that <span> uses a custom data attribute data-key=..., instead of name=...

function initGenericKeyValueElement(key, value) {
    let span = document.querySelector(`span[data-key='${key}']`);
    if (span) {
        setSpanValue(span, value);
    }

    let inputs = [];
    for (let elem of document.querySelectorAll(`[name='${key}'`)) {
        switch (elem.tagName) {
        case "INPUT":
            setInputValue(elem, value);
            inputs.push(elem);
            break;
        case "SELECT":
            setSelectValue(elem, value);
            inputs.push(elem);
            break;
        }
    }

    setOriginalsFromValues(inputs);
}

// Or, handle special id-counted 'line' that was created from template
// Optional cfg kv object that will be used to pull values for input / select / span
//
// Additional handler for checkbox elements, since we need a document-wide unique id=...
// (also see {create,update}Checkboxes())

function fillTemplateLineFromCfg(line, id, cfg) {
    if (cfg === undefined) {
        cfg = {};
    }

    for (let elem of line.querySelectorAll("input,select,span")) {
        let key = elem.name || elem.dataset.key;
        if (key && (key in cfg)) {
            switch (elem.tagName) {
            case "INPUT":
                setInputValue(elem, cfg[key]);
                break;
            case "SELECT":
                setSelectValue(elem, cfg[key]);
                break;
            case "SPAN":
                setSpanValue(elem, cfg[key]);
                break;
            }
        }

        if (elem.tagName === "INPUT" && elem.type === "checkbox") {
            const realId = elem.name.concat(id);
            elem.id = realId;
            elem.nextElementSibling.htmlFor = realId;
        }
    }

    setOriginalsFromValuesForNode(line);
}


function delParent(event) {
    event.target.parentElement.remove();
}

function moreParentElem(parent) {
    for (let elem of parent.querySelectorAll(".more")) {
        elem.style.display = (elem.style.display === "")
            ? "inherit" : "";
    }
}

function moreParent(event) {
    moreParentElem(event.target.parentElement.parentElement);
}

function idForTemplateContainer(container) {
    let id = container.childElementCount;

    let settingsMax = container.dataset["settingsMax"];
    if (settingsMax === undefined) {
        return id;
    }

    let max = parseInt(settingsMax, 10);
    if (id < max) {
        return id;
    }
    
    alert(`Max number of ${container.id} has been reached (${id} out of ${max})`);
    return -1;
}

// -----------------------------------------------------------------------------
// Actions
// -----------------------------------------------------------------------------

function send(payload) {
    if (Debug) {
        console.log(payload);
    }
    Websock.send(payload);
}

// RPC-like actions

function sendAction(action, data) {
    send(JSON.stringify({action, data}));
}

// Settings kv as either {key: value} or {key: [value0, value1, ...etc...]}

function sendConfig(config) {
    send(JSON.stringify({config}));
}

// Track with <input> and <select> elements were received via the WebSocket
// Generic change handler below will compare the user input with the original value, allowing to send configuration in parts instead of all of it at once.

function setOriginalsFromValuesForNode(node, elems) {
    if (elems === undefined) {
        elems = [...node.querySelectorAll("input,select")];
    }

    for (let elem of elems) {
        switch (elem.tagName) {
        case "INPUT":
            if ((elem.type === "checkbox") || (elem.type === "radio")) {
                elem.dataset["original"] = booleanToString(elem.checked);
            } else {
                elem.dataset["original"] = elem.value;
            }
            break;
        case "SELECT":
            elem.dataset["original"] = stringifySelectedValues(elem);
            break;
        }
        resetChangedElement(elem);
    }
}

function setOriginalsFromValues(elems) {
    setOriginalsFromValuesForNode(document, elems);
}

function resetOriginals() {
    setOriginalsFromValues();
    resetChangedGroups();
    Settings.resetCounters();
    Settings.saved = false;
}

function doReload(milliseconds) {
    setTimeout(() => {
        window.location.reload();
    }, parseInt(milliseconds, 10));
}

// Check whether the file object contains some known bytes
// - handle magic numbers at the start
// - handle SDK flash modes (and compare with the current one)
//
// Result is handled through callback:
// - success as callback(true)
// - failure as callback(false)
function checkFirmware(file, callback) {

    var reader = new FileReader();

    reader.onloadend = function(evt) {
        if (FileReader.DONE === evt.target.readyState) {
            var magic = evt.target.result.charCodeAt(0);
            if ((0x1F === magic) && (0x8B === evt.target.result.charCodeAt(1))) {
                callback(true);
                return;
            }

            if (0xE9 !== magic) {
                alert("Binary image does not start with a magic byte");
                callback(false);
                return;
            }

            var modes = ['QIO', 'QOUT', 'DIO', 'DOUT'];
            var flash_mode = evt.target.result.charCodeAt(2);
            if (0x03 !== flash_mode) {
                var response = window.confirm("Binary image is using " + modes[flash_mode] + " flash mode! Make sure that the device supports it before proceeding.");
                callback(response);
            } else {
                callback(true);
            }
        }
    };

    var blob = file.slice(0, 3);
    reader.readAsBinaryString(blob);

}

function doUpgrade(event) {
    event.preventDefault();

    let upgrade = document.querySelector("input[name='upgrade']");
    let file = upgrade.files[0];
    if (typeof file === "undefined") {
        alert("First you have to select a file from your computer.");
        return;
    }

    if (file.size > FreeSize) {
        alert("Image it too large to fit in the available space for OTA. Consider doing a two-step update.");
        return;
    }

    checkFirmware(file, (ok) => {
        if (!ok) {
            return;
        }

        var data = new FormData();
        data.append("upgrade", file, file.name);

        var xhr = new XMLHttpRequest();

        var msg_ok = "Firmware image uploaded, board rebooting. This page will be refreshed in 5 seconds.";
        var msg_err = "There was an error trying to upload the new image, please try again: ";

        var network_error = function(e) {
            alert(msg_err + " xhr request " + e.type);
        };
        xhr.addEventListener("error", network_error, false);
        xhr.addEventListener("abort", network_error, false);

        let progress = document.getElementById("upgrade-progress");
        xhr.addEventListener("load", () => {
            progress.style.display = "none";
            if ("OK" === xhr.responseText) {
                alert(msg_ok);
                doReload(5000);
            } else {
                alert(msg_err + xhr.status.toString() + " " + xhr.statusText + ", " + xhr.responseText);
            }
        }, false);

        xhr.upload.addEventListener("progress", (event) => {
            progress.style.display = "inherit";
            if (event.lengthComputable) {
                progress.value = event.loaded;
                progress.max = event.total;
            }
        }, false);

        xhr.open("POST", Urls.upgrade.href);
        xhr.send(data);
    });
}

// Initial page, when webMode only allows to change the password
function doSetupPassword(event) {
    let forms = [document.forms["form-setup-password"]];
    if (validateFormPasswords(forms, true)) {
        sendConfig(getData(forms, true, false));
    }

    event.preventDefault();
}

function checkChanges() {
    if (Settings.counters.changed > 0) {
        var response = window.confirm("Some changes have not been saved yet, do you want to save them first?");
        if (response) {
            doUpdate();
        }
    }
}

function doAction(event, question, action) {
    event.preventDefault();

    checkChanges();
    if (question && !window.confirm(question)) {
        return;
    }

    sendAction(action, {});
    doReload(5000);
}

function doReboot(event) {
    doAction(event, "Are you sure you want to reboot the device?", "reboot");
}

function doReconnect(ask) {
    var question = (typeof ask === "undefined" || false === ask) ?
        null :
        "Are you sure you want to disconnect from the current WIFI network?";
    return doAction(question, "reconnect");
}

function doCheckOriginals() {
    var response;

    if (Settings.counters.reboot > 0) {
        response = window.confirm("You have to reboot the board for the changes to take effect, do you want to do it now?");
        if (response) {
            doReboot(false);
        }
    } else if (Settings.counters.reconnect > 0) {
        response = window.confirm("You have to reconnect to the WiFi for the changes to take effect, do you want to do it now?");
        if (response) { doReconnect(false); }
    } else if (Settings.counters.reload > 0) {
        response = window.confirm("You have to reload the page to see the latest changes, do you want to do it now?");
        if (response) { doReload(0); }
    }

    resetOriginals();
}

function waitForSave(){
    if (!Settings.saved) {
        setTimeout(waitForSave, 1000);
    } else {
        doCheckOriginals();
    }
}

function doUpdate() {
    // Since we have 2-page config, make sure we select the active one
    let forms = document.getElementsByClassName("form-settings");
    if (validateForms(forms)) {
        sendConfig(getData(forms));

//removeIf(!sensor)
        // Energy reset is handled via these keys
        // TODO: replace these with actions, not settings
        for (let elem of document.getElementsByClassName("pwrExpected")) {
            elem.value = 0;
        }

        for (let form of document.forms) {
            if (form.elements.snsResetCalibration) {
                form.elements.snsResetCalibration.checked = false;
            }
            if (form.elements.pwrResetCalibration) {
                form.elements.pwrResetCalibration.checked = false;
            }
            if (form.elements.pwrResetE) {
                form.elements.pwrResetE.checked = false;
            }
        }
//endRemoveIf(!sensor)

        Settings.counters.changed = 0;
        waitForSave();
    }

    return false;
}

function doBackup() {
    document.getElementById("downloader").click();
}

function onFileUpload(event) {
    var inputFiles = event.target.files;
    if (typeof inputFiles === "undefined" || inputFiles.length === 0) {
        return false;
    }
    var inputFile = inputFiles[0];
    event.target.value = "";

    var response = window.confirm("Previous settings will be overwritten. Are you sure you want to restore from this file?");
    if (!response) {
        return false;
    }

    var reader = new FileReader();
    reader.onload = function(e) {
        try {
            var data = JSON.parse(e.target.result);
            sendAction("restore", data);
        } catch (e) {
            window.alert(e);
        }
    };
    reader.readAsText(inputFile);

    event.preventDefault();
}

function doRestore(event) {
    if (typeof window.FileReader === "function") {
        document.getElementById("uploader").click();
    } else {
        alert("The file API isn't supported on this browser yet.");
    }
    event.preventDefault();
}

function doFactoryReset(event) {
    let response = window.confirm("Are you sure you want to restore to factory settings?");
    if (response) {
        sendAction("factory_reset", {});
        doReload(5000);
    }
    event.preventDefault();
}

function doScan(event) {
    let [results] = document.getElementById("scanResult").tBodies;
    while (results.rows.length) {
        results.deleteRow(0);
    }

    let loading = document.querySelector("div.scan.loading");
    loading.style.display = "inherit";

    for (let button of document.querySelectorAll(".button-wifi-scan")) {
        button.disabled = true;
    }

    sendAction("scan", {});
    event.preventDefault();
}

function doDebugCommand(command) {
    sendAction("dbgcmd", {command: command});
    followScroll(document.getElementById("weblog"), 0);
}

function doDebugClear(event) {
    let elem = document.getElementById("weblog");
    elem.textContent = "";
    event.preventDefault();
}

//removeIf(!rfm69)

function rfm69AddMapping(cfg) {
    addFromTemplate(document.getElementById("rfm69-mapping"), "rfm69-node", cfg);
}

function rfm69Messages() {
    let [body] = document.getElementById("rfm69-messages").tBodies;
    return body;
}

function rfm69AddMessage(message) {
    let timestamp = (new Date()).toLocaleTimeString("en-US", {hour12: false});

    let container = rfm69Messages();
    let row = container.insertRow();
    for (let value of [timestamp, ...message]) {
        let cell = row.insertCell();
        cell.appendChild(document.createTextNode(value));
        rfm69FilterRow(Rfm69.filters, row);
    }
}

function rfm69ClearCounters() {
    sendAction("rfm69Clear", {});
    return false;
}

function rfm69ClearMessages() {
    let container = rfm69Messages();
    while (container.rows.length) {
        container.deleteRow(0);
    }
    return false;
}

function rfm69FilterRow(filters, row) {
    row.style.display = "table-row";
    for (const [cell, filter] of Object.entries(filters)) {
        if (row.cells[cell].textContent !== filter) {
            row.style.display = "none";
        }
    }
}

function rfm69Filter(filters, rows) {
    for (let row of rows) {
        rfm69FilterRow(filters, row);
    }
}

function rfm69FilterEvent(event) {
    if (event.target.classList.contains("filtered")) {
        delete Rfm69.filters[event.target.cellIndex];
    } else {
        Rfm69.filters[event.target.cellIndex] = event.target.textContent;
    }
    event.target.classList.toggle("filtered");

    rfm69Filter(Rfm69.filters, rfm69Messages().rows);
}

function rfm69ClearFilters() {
    let container = rfm69Messages();
    for (let elem of container.querySelectorAll("filtered")) {
        elem.classList.remove("filtered");
    }

    Rfm69.filters = {};
    rfm69Filter(Rfm69.filters, container.rows);
}

//endRemoveIf(!rfm69)

// -----------------------------------------------------------------------------
// Visualization
// -----------------------------------------------------------------------------

function toggleMenu(event) {
    if (event !== undefined && event.cancelable) {
        event.preventDefault();
        event.stopPropagation();
    }

    for (const id of ["layout", "menu", "menu-link"]) {
        document.getElementById(id).classList.toggle("active");
    }
}

function showPanel(event) {
    for (let panel of document.querySelectorAll(".panel")) {
        panel.style.display = "none";
    }

    let layout = document.getElementById("layout");
    if ("active" in layout.classList) {
        toggleMenu();
    }

    let panel = event.target.dataset["panel"];
    document.getElementById(`panel-${panel}`).style.display = "inherit";

    event.preventDefault();
    event.stopPropagation();
}

function loadTemplate(name) {
    let template = document.getElementById(`template-${name}`);
    return document.importNode(template.content, true);
}

function loadConfigTemplate(id) {
    let template = loadTemplate(id);
    for (let elem of template.querySelectorAll("input,select")) {
        elem.dataset["settingsGroupElement"] = "true";
    }

    for (let elem of template.querySelectorAll("button.button-del-parent")) {
        elem.addEventListener("click", delParent);
    }

    for (let elem of template.querySelectorAll("button.button-more-parent")) {
        elem.addEventListener("click", moreParent);
    }

    for (let elem of template.querySelectorAll("select.enumerable")) {
        initEnumerableSelect(elem, initSelect);
    }

    createCheckboxes(template);
    return template;
}

function mergeTemplate(target, template) {
    for (let child of Array.from(template.children)) {
        target.appendChild(child);
    }
}

function addFromTemplate(container, template, cfg) {
    let line = loadConfigTemplate(template);
    fillTemplateLineFromCfg(line, container.childElementCount, cfg);

    mergeTemplate(container, line);
}

// -----------------------------------------------------------------------------
// Relays & magnitudes mapping
// -----------------------------------------------------------------------------

function createRelayList(values, container, template_name) {
    let target = document.getElementById(container);
    if (target.childElementCount > 0) { return; }

    let template = loadConfigTemplate(template_name);
    values.forEach((value, index) => {
        let line = template.cloneNode(true);
        line.querySelector("label").textContent += " #".concat(index)

        let input = line.querySelector("input");
        input.value = value;
        input.dataset["original"] = value;

        mergeTemplate(target, line);
    });
}

//removeIf(!sensor)
//
function createMagnitudeList(data, container, template_name) {
    let target = document.getElementById(container);
    if (target.childElementCount > 0) { return; }

    data.values.forEach((values) => {
        let [type, index_global, index_module] = values;

        let line = loadConfigTemplate(template_name);
        line.querySelector("label").textContent =
            MagnitudeNames[type].concat(" #").concat(parseInt(index_global, 10));
        line.querySelector("div.hint").textContent =
            Magnitudes[index_global].description;

        let input = line.querySelector("input");
        input.value = index_module;
        input.dataset["original"] = input.value;

        mergeTemplate(target, line);
    });
}

//endRemoveIf(!sensor)

// -----------------------------------------------------------------------------
// RPN Rules
// -----------------------------------------------------------------------------

function rpnAddRule(cfg) {
    addFromTemplate(document.getElementById("rpn-rules"), "rpn-rule", cfg);
}

function rpnAddTopic(cfg) {
    addFromTemplate(document.getElementById("rpn-topics"), "rpn-topic", cfg);
}

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

function networkAdd(cfg, showMore) {
    let container = document.getElementById("networks");

    let id = idForTemplateContainer(container);
    if (id < 0) {
        return;
    }

    if (showMore === undefined) {
        showMore = true;
    }

    let line = loadConfigTemplate("network-config");
    fillTemplateLineFromCfg(line, id, cfg);
    if (showMore) {
        moreParentElem(line);
    }

    mergeTemplate(container, line);
    return;
}

function networkScanResult(values) {
    let loading = document.querySelector("div.scan.loading");
    loading.style.display = "none";

    for (let button of document.querySelectorAll(".button-wifi-scan")) {
        button.disabled = false;
    }

    let table = document.getElementById("scanResult");
    table.style.display = "table";

    let [results] = table.tBodies;
    let row = results.insertRow();
    for (let value of values) {
        let cell = row.insertCell();
        cell.appendChild(document.createTextNode(value));
    }
}

// -----------------------------------------------------------------------------
// Simple LEDs configuration
// -----------------------------------------------------------------------------

function ledAdd(cfg) {
    let container = document.getElementById("leds");

    let id = idForTemplateContainer(container);
    if (id < 0) {
        return;
    }

    let line = loadConfigTemplate("led-config");
    line.querySelector("span").textContent = id;
    fillTemplateLineFromCfg(line, id, cfg);

    mergeTemplate(container, line);
}

// -----------------------------------------------------------------------------
// Date and time scheduler
// -----------------------------------------------------------------------------

function schAdd(cfg) {
    if (cfg.schType === undefined) {
        return;
    }

    let container = document.getElementById("schedules");

    let id = idForTemplateContainer(container);
    if (id < 0) {
        return;
    }

    let line = loadConfigTemplate("schedule-config");

    const type = (cfg.schType === 1) ? "relay" :
        (cfg.schType === 2) ? "light" :
        (cfg.schType === 3) ? "curtain" :
        "none";
    if (type !== "none") {
        mergeTemplate(line.querySelector(".schedule-action"),
            loadConfigTemplate("schedule-action-".concat(type)));
    }

    fillTemplateLineFromCfg(line, id, cfg);
    mergeTemplate(container, line);
    return;
}

// -----------------------------------------------------------------------------
// Relays
// -----------------------------------------------------------------------------

function relayToggle(event) {
    sendAction("relay", {
        id: parseInt(event.target.dataset["id"], 10),
        status: event.target.checked ? "1" : "0"});
    event.preventDefault();
}

function initRelayToggle(id, cfg) {
    let line = loadConfigTemplate("relay-control");

    let name = line.querySelector("span[data-key='relayName']");
    name.textContent = cfg.relayName;
    name.dataset["id"] = id;
    name.setAttribute("title", cfg.relayDesc);

    let realId = "relay".concat(id);

    let toggle = line.querySelector("input[type='checkbox']");
    toggle.checked = false;
    toggle.disabled = true;
    toggle.dataset["id"] = id;
    toggle.addEventListener("change", relayToggle);

    toggle.setAttribute("id", realId);
    toggle.nextElementSibling.setAttribute("for", realId);

    mergeTemplate(document.getElementById("relays"), line);
}

function updateRelays(data) {
    data.states.forEach((state, id) => {
        const relay = fromSchema(state, data.schema);

        let elem = document.querySelector(`input[name='relay'][data-id='${id}']`);
        elem.checked = relay.status;
        elem.disabled = ({
            0: false,
            1: !relay.status,
            2: relay.status
        })[relay.lock]; // TODO: specify lock statuses earlier?
    });
}

// TODO: per https://www.chromestatus.com/feature/6140064063029248, using <base target="_blank"> should be enough with recent browsers
// but, side menu needs to be reworked for it to correctly handle panel switching, since it uses <a href="#" ...>
// TODO: also could be done in htmlparser2 + gulp (even, preferably)

function initExternalLinks() {
    for (let elem of document.getElementsByClassName("external")) {
        if (elem.tagName === "A") {
            elem.setAttribute("target", "_blank");
            elem.setAttribute("rel", "noopener");
            elem.setAttribute("tabindex", "-1");
        }
    }
}

// TODO: ids are generated based on the current position of the element
// when removing nodes, id=... and label for=... must remain unique, thus everything needs to be reset
// based on the new number and position of nodes

function createCheckboxes(node) {
    let checkboxes = node.querySelectorAll("input[type='checkbox']");
    for (let checkbox of checkboxes) {
        checkbox.id = checkbox.name;
        checkbox.parentElement.classList.add("toggleWrapper");

        let label = document.createElement("label");
        label.classList.add("toggle");
        label.htmlFor = checkbox.id;

        let span = document.createElement("span");
        span.classList.add("toggle__handler");
        label.appendChild(span);

        checkbox.parentElement.appendChild(label);
    }
}

function updateCheckboxes(node) {
    for (let id = 0; id < node.childElementCount; ++id) {
        let schedule = node.children[id];
        for (let checkbox of schedule.querySelectorAll("input[type='checkbox']")) {
            let realId = checkbox.name.concat(id);
            checkbox.setAttribute("id", realId);
            checkbox.nextElementSibling.setAttribute("for", realId);
        }
    }
}

function initRelayConfig(id, cfg) {
    let line = loadConfigTemplate("relay-config");
    fillTemplateLineFromCfg(line, id, cfg);
    mergeTemplate(document.getElementById("relayConfig"), line);
}

// -----------------------------------------------------------------------------
// Sensors & Magnitudes
// -----------------------------------------------------------------------------

//removeIf(!sensor)

function initMagnitudes(data) {
    let container = document.getElementById("magnitudes");
    if (container.childElementCount > 0) {
        return;
    }

    data.types.values.forEach((cfg) => {
        const info = fromSchema(cfg, data.types.schema);
        MagnitudeNames[info.type] = info.name;
        MagnitudeTypePrefixes[info.type] = info.prefix;
        MagnitudePrefixTypes[info.prefix] = info.type;
    });

    data.errors.values.forEach((cfg) => {
        const error = fromSchema(cfg, data.errors.schema);
        MagnitudeErrors[error.type] = error.name;
    });

    data.magnitudes.values.forEach((cfg, index) => {
        const magnitude = fromSchema(cfg, data.magnitudes.schema);

        const prettyName = MagnitudeNames[magnitude.type]
            .concat(" #").concat(parseInt(magnitude.index_global, 10));
        Magnitudes.push({
            name: prettyName,
            units: magnitude.units,
            description: magnitude.description
        });

        let info = loadTemplate("magnitude-info");
        info.querySelector("label").textContent = prettyName;
        info.querySelector("input").dataset["id"] = index;
        info.querySelector("input").dataset["type"] = magnitude.type;
        info.querySelector("div.sns-desc").textContent = magnitude.description;
        info.querySelector("div.sns-info").style.display = "none";

        mergeTemplate(container, info);
    });
}

function updateMagnitudes(data) {
    data.values.forEach((cfg, id) => {
        const magnitude = fromSchema(cfg, data.schema);

        let input = document.querySelector(`input[name='magnitude'][data-id='${id}']`);
        input.value = (0 === magnitude.error)
            ? (magnitude.value + Magnitudes[id].units)
            : MagnitudeErrors[magnitude.error];

        if (magnitude.info.length) {
            let info = input.parentElement.parentElement.querySelector("div.sns-info");
            info.style.display = "inherit";
            info.textContent = magnitude.info;
        }

    });
}

//endRemoveIf(!sensor)

// -----------------------------------------------------------------------------
// Thermostat
// -----------------------------------------------------------------------------

//removeIf(!thermostat)

function thermostatCheckTempRange(event) {
    const min = document.getElementById("tempRangeMinInput");
    const max = document.getElementById("tempRangeMaxInput");

    if (event.target.id === max.id) {
        const maxValue = parseInt(max.value, 10) - 1;
        if (parseInt(min.value, 10) > maxValue) {
            min.value = maxValue;
        }
    } else {
        const minValue = parseInt(min.value, 10) + 1;
        if (parseInt(max.value, 10) < minValue) {
            max.value = minValue;
        }
    }
}

//endRemoveIf(!thermostat)

// -----------------------------------------------------------------------------
// Curtains
// -----------------------------------------------------------------------------

//removeIf(!curtain)

function curtainButtonHandler(event) {
    if (event.type !== "click") {
        return;
    }

    let code = -1;

    let list = event.target.classList;
    if (list.contains("button-curtain-pause")) {
        code = 0;
    } else if (list.contains("button-curtain-open")) {
        code = 1;
    } else if (list.contains("button-curtain-close")) {
        code = 2;
    }

    if (code >= 0) {
        sendAction("curtainAction", {button: code});
        event.target.style.background = "red";
    }

    event.preventDefault();
    event.stopPropagation();
}

function curtainSetHandler(event) {
    sendAction("curtainAction", {position: event.target.value});
}

//Create the controls for one curtain. It is called when curtain is updated (so created the first time)
//Let this there as we plan to have more than one curtain per switch
function initCurtain() {
    let container = document.getElementById("curtains");
    if (container.childElementCount > 0) {
        return;
    }

    // simple position slider
    document.getElementById("curtainSet").addEventListener("change", curtainSetHandler);

    // add and init curtain template, prepare multi switches
    let line = loadConfigTemplate("curtain-control");
    line.querySelector(".button-curtain-open").addEventListener("click", curtainButtonHandler);
    line.querySelector(".button-curtain-pause").addEventListener("click", curtainButtonHandler);
    line.querySelector(".button-curtain-close").addEventListener("click", curtainButtonHandler);
    mergeTemplate(container, line);

    addSimpleEnumerables("curtain", "Curtain", 1);
}

function setCurtainBackground(a, b) {
    let elem = document.getElementById("curtainGetPicture");
    elem.style.background = `linear-gradient(${a}, black ${b}%, #a0d6ff ${b}%)`;
}

function setCurtainBackgroundTwoSides(a, b) {
    let elem = document.getElementById("curtainGetPicture");
    elem.style.background = `linear-gradient(90deg, black ${a}%, #a0d6ff ${a}% ${b}%, black ${b}%)`;
}

function updateCurtain(value) {
    switch(value.type) {
    case '1': //One side left to right
        setCurtainBackground('90deg', value.get);
        break;
    case '2': //One side right to left
        setCurtainBackground('270deg', value.get);
        break;
    case '3': //Two sides
        setCurtainBackgroundTwoSides(value.get / 2, (100 - value.get/2));
        break;
    case '0': //Roller
    default:
        setCurtainBackground('180deg', value.get);
        break;
    }

    let set = document.getElementById("curtainSet");
    set.value = value.set;

    const backgroundMoving = 'rgb(192, 0, 0)';
    const backgroundStopped = 'rgb(64, 184, 221)';

    if (!value.moving) {
        let button = document.querySelector("button.curtain-button");
        button.style.background = backgroundStopped;
    } else {
        if (!value.button) {
            let pause = document.querySelector("button.curtain-pause");
            pause.style.background = backgroundMoving;
        } else {
            let open = document.querySelector("button.button-curtain-open");
            let close = document.querySelector("button.button-curtain-close");
            if (value.button === 1) {
                open.style.background = backgroundMoving;
                close.style.background = backgroundStopped;
            } else if (value.button === 2) {
                open.style.background = backgroundStopped;
                close.style.background = backgroundMoving;
            }
        }
    }
}

//endRemoveIf(!curtain)

// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

//removeIf(!light)

function colorToHsvString(color) {
    var h = String(Math.round(color.hsv.h));
    var s = String(Math.round(color.hsv.s));
    var v = String(Math.round(color.hsv.v));
    return h + "," + s + "," + v;
}

function hsvStringToColor(hsv) {
    var parts = hsv.split(",");
    return {
        h: parseInt(parts[0]),
        s: parseInt(parts[1]),
        v: parseInt(parts[2])
    };
}

function colorSlider(type) {
    return {component: iro.ui.Slider, options: {sliderType: type}};
}

function colorWheel() {
    return {component: iro.ui.Wheel, options: {}};
}

function colorBox() {
    return {component: iro.ui.Box, options: {}};
}

function updateColor(mode, value) {
    if (ColorPicker) {
        if (mode === "rgb") {
            ColorPicker.color.hexString = value;
        } else if (mode === "hsv") {
            ColorPicker.color.hsv = hsvStringToColor(value);
        }
        return;
    }

    // TODO: useRGB -> ltWheel?
    // TODO: always show wheel + sliders like before?
    var layout = []
    if (mode === "rgb") {
        layout.push(colorWheel());
        layout.push(colorSlider("value"));
    } else if (mode === "hsv") {
        layout.push(colorBox());
        layout.push(colorSlider("hue"));
    }

    var options = {
        color: (mode === "rgb") ? value : hsvStringToColor(value),
        layout: layout
    };

    // TODO: ref. #2451, this causes pretty fast updates.
    // since we immediatly start the transition, debug print's yield() may interrupt us mid initialization
    // api could also wait and hold the value for a bit, applying only some of the values between start and end, and then apply the last one
    ColorPicker = new iro.ColorPicker("#color", options);
    ColorPicker.on("input:change", (color) => {
        if (mode === "rgb") {
            sendAction("color", {rgb: color.hexString});
        } else if (mode === "hsv") {
            sendAction("color", {hsv: colorToHsvString(color)});
        }
    });
}

function onChannelSliderChange(event) {
    event.target.nextElementSibling.textContent = event.target.value;
    sendAction("channel", {id: event.target.dataset["id"], value: event.target.value});
}

function onBrightnessSliderChange(event) {
    event.target.nextElementSibling.textContent = event.target.value;
    sendAction("brightness", {value: event.target.value});
}

function updateMireds(value) {
    let mireds = document.getElementById("mireds");
    if (mireds !== null) {
        mireds.setAttribute("min", value.cold);
        mireds.setAttribute("max", value.warm);
        mireds.value = value.value;
        mireds.nextElementSibling.textContent = value.value;
        return;
    }

    let control = loadTemplate("mireds-control");
    control.getElementById("mireds").addEventListener("change", (event) => {
        event.target.nextElementSibling.textContent = event.target.value;
        sendAction("mireds", {mireds: event.target.value});
    });
    mergeTemplate(document.getElementById("cct"), control);
    updateMireds(value);
}

function updateBrightness(value) {
    let brightness = document.getElementById("brightness");
    if (brightness !== null) {
        brightness.value = value;
        brightness.nextElementSibling.textContent = value;
        return;
    }

    let template = loadTemplate("brightness-control");

    let slider = template.getElementById("brightness");
    slider.value = value;
    slider.nextElementSibling.textContent = value;
    slider.addEventListener("change", onBrightnessSliderChange);
    mergeTemplate(document.getElementById("light"), template);
}

function initChannels(container, channels) {
    channels.forEach((value, channel) => {
        let line = loadTemplate("channel-control");
        line.querySelector("span.slider").dataset["id"] = channel;

        let slider = line.querySelector("input.slider");
        slider.value = value;
        slider.nextElementSibling.textContent = value;
        slider.dataset["id"] = channel;
        slider.addEventListener("change", onChannelSliderChange);

        line.querySelector("label").textContent = "Channel #".concat(channel);
        mergeTemplate(container, line);
    });
}

function updateChannels(channels) {
    let container = document.getElementById("channels");
    if (container.childElementCount > 0) {
        channels.forEach((value, channel) => {
            let slider = container.querySelector(`input.slider[data-id='${channel}']`);
            if (!slider) {
                return;
            }

            // If there are RGB controls, no need for raw channel sliders
            if (ColorPicker && (channel < 3)) {
                slider.parentElement.style.display = "none";
            }

            // Or, when there are CCT controls
            if ((channel === 3) || (channel === 4)) {
                let cct = document.getElementById("cct");
                if (cct.childElementCount > 0) {
                    slider.parentElement.style.display = "none";
                }
            }

            slider.value = value;
            slider.nextElementSibling.textContent = value;
        });
        return;
    }

    initChannels(container, channels);
    updateChannels(channels);
}

//endRemoveIf(!light)

// -----------------------------------------------------------------------------
// RFBridge
// -----------------------------------------------------------------------------

//removeIf(!rfbridge)

function rfbAction(event) {
    const prefix = "button-rfb-";
    let [action] = Array.from(event.target.classList)
        .filter(x => x.startsWith(prefix));

    if (action) {
        let container = event.target.parentElement.parentElement;
        let input = container.querySelector("input");

        action = action.replace(prefix, "");
        sendAction(`rfb${action}`, {
            id: input.dataset["id"],
            status: input.dataset["status"]
        });
    }
}

function rfbAdd() {
    let container = document.getElementById("rfbNodes");

    const id = container.childElementCount;
    let line = loadTemplate("rfb-node");
    line.querySelector("span").textContent = id;

    for (let input of line.querySelectorAll("input")) {
        input.dataset["id"] = id;
    }

    elementSelectorOnClick(".button-rfb-learn", rfbAction);
    elementSelectorOnClick(".button-rfb-forget", rfbAction);
    elementSelectorOnClick(".button-rfb-send", rfbAction);

    mergeTemplate(container, line);

    return false;
}

function rfbSelector(id, status) {
    return `input[name='rfbcode'][data-id='${id}'][data-status='${status}']`;
}

function rfbHandleCodes(value) {
    value.codes.forEach((codes, id) => {
        let realId = id + value.start;
        let [off, on] = codes;
        document.querySelector(rfbSelector(realId, 0)).value = off;
        document.querySelector(rfbSelector(realId, 1)).value = on;
    });
}

//endRemoveIf(!rfbridge)

// -----------------------------------------------------------------------------
// LightFox
// -----------------------------------------------------------------------------

//removeIf(!lightfox)

function lightfoxLearn() {
    sendAction("lightfoxLearn", {});
}

function lightfoxClear() {
    sendAction("lightfoxClear", {});
}

//endRemoveIf(!lightfox)

// -----------------------------------------------------------------------------
// Processing
// -----------------------------------------------------------------------------

function processData(data) {
    if (Debug) {
        console.log(data);
    }

    // title
    if ("app_name" in data) {
        let title = data.app_name;
        if ("app_version" in data) {
            let span = document.querySelector("span[data-key='title']");
            span.textContent = data.app_version;
            title = title + " " + data.app_version;
        }
        if ("hostname" in data) {
            title = data.hostname + " - " + title;
        }
        document.title = title;
    }

    Object.keys(data).forEach((key) => {
        let value = data[key];

        // ---------------------------------------------------------------------
        // Web mode & modules
        // ---------------------------------------------------------------------

        if ("webMode" === key) {
            let initial = (1 === value);

            let layout = document.getElementById("layout");
            layout.style.display = initial ? "none" : "inherit";

            let password = document.getElementById("password");
            password.style.display = initial ? "inherit" : "none";

            return;
        }

        if (key.endsWith("Visible")) {
            // TODO: Move to another 'module' that saves the energy data and have a common setting?
            if ("pzemVisible" === key) {
                let save = document.querySelector("input[name='snsSave']");
                save.disabled = true;
            }

            let module = key.slice(0, -7);
            if (module.length) {
                moduleVisible(module);
            }
            return;
        }

        if ("gpioConfig" === key) {
            let types = [];

            for (const [type, id] of value.types) {
                types.push({
                    "id": id,
                    "name": type
                });

                let gpios = [{"id": 153, "name": "NONE"}];
                value[type].forEach((pin) => {
                    gpios.push({"id": pin, "name": `GPIO${pin}`});
                });
                addEnumerables(`gpio-${type}`, gpios);
            }

            addEnumerables("gpio-types", types);
            return;
        }

        // ---------------------------------------------------------------------
        // Actions
        // ---------------------------------------------------------------------

        if ("action" === key) {
            if ("reload" === data.action) {
                doReload(1000);
            }
            return;
        }

        // ---------------------------------------------------------------------
        // RFBridge
        // ---------------------------------------------------------------------

        //removeIf(!rfbridge)

        if ("rfbCount" === key) {
            for (let i = 0; i < data.rfbCount; ++i) {
                rfbAdd();
            }
            return;
        }

        if ("rfb" === key) {
            rfbHandleCodes(value);
            return;
        }

        //endRemoveIf(!rfbridge)

        // ---------------------------------------------------------------------
        // RFM69
        // ---------------------------------------------------------------------

        //removeIf(!rfm69)

        if ("rfm69" === key) {
            if (value.message !== undefined) {
                rfm69AddMessage(value.message);
                return;
            }

            if (value.mapping !== undefined) {
                value.mapping.forEach((mapping) => {
                    rfm69AddMapping(fromSchema(mapping, value.schema));
                });
                return;
            }
        }

        //endRemoveIf(!rfm69)

        // ---------------------------------------------------------------------
        // RPN Rules
        // ---------------------------------------------------------------------

        if ("rpnRules" === key) {
			for (let rule of value) {
				rpnAddRule({"rpnRule": rule});
            }
			return;
		}

        if ("rpnTopics" === key) {
            value.topics.forEach((topic) => {
                rpnAddTopic(fromSchema(topic, value.schema));
            });
			return;
        }

        // ---------------------------------------------------------------------
        // Curtains
        // ---------------------------------------------------------------------

        //removeIf(!curtain)

        if ("curtainState" === key) {
            initCurtain();
            updateCurtain(value);
            return;
        }

        //endRemoveIf(!curtain)

        // ---------------------------------------------------------------------
        // Lights
        // ---------------------------------------------------------------------

        //removeIf(!light)

        if ("lightstate" === key) {
            let color = document.getElementById("color");
            color.style.display = value ? "inherit" : "none";
            return;
        }

        if (("rgb" === key) || ("hsv" === key)) {
            updateColor(key, value);
            return;
        }

        if ("brightness" === key) {
            updateBrightness(value);
            return;
        }

        if ("channels" === key) {
            updateChannels(value);
            addSimpleEnumerables("channel", "Channel", value.length);
            return;
        }

        if ("mireds" === key) {
            updateMireds(value);
            return;
        }

        //endRemoveIf(!light)

        // ---------------------------------------------------------------------
        // Sensors & Magnitudes
        // ---------------------------------------------------------------------

        //removeIf(!sensor)

        if ("magnitudesConfig" === key) {
            initMagnitudes(value);
            return;
        }

        if ("magnitudes" === key) {
            updateMagnitudes(value);
            return;
        }

        //endRemoveIf(!sensor)

        // ---------------------------------------------------------------------
        // WiFi
        // ---------------------------------------------------------------------

        if ("wifiConfig" === key) {
            let container = document.getElementById("networks");
            container.dataset["settingsMax"] = value.max;

            value.networks.forEach((entries) => {
                networkAdd(fromSchema(entries, value.schema), false);
            });
            return;
        }

        if ("scanResult" === key) {
            networkScanResult(value);
            return;
        }

        // -----------------------------------------------------------------------------
        // Relays scheduler
        // -----------------------------------------------------------------------------

        if ("schConfig" === key) {
            let container = document.getElementById("schedules");
            container.dataset["settingsMax"] = value.max;

            value.schedules.forEach((entries) => {
                schAdd(fromSchema(entries, value.schema));
            });

            return;
        }

        // ---------------------------------------------------------------------
        // Relays
        // ---------------------------------------------------------------------

        if ("relayConfig" === key) {
            let container = document.getElementById("relays");
            if (container.childElementCount > 0) {
                return;
            }

            let relays = [];
            value.relays.forEach((entries, id) => {
                let cfg = fromSchema(entries, value.schema);
                if (!cfg.relayName || !cfg.relayName.length) {
                    cfg.relayName = `Switch #${id}`;
                }

                relays.push({
                    "id": id,
                    "name": `${cfg.relayName} (${cfg.relayDesc})`
                });

                initRelayToggle(id, cfg);
                initRelayConfig(id, cfg);
            });

            addEnumerables("relay", relays);
            return;
        }

        if ("relayState" === key) {
            updateRelays(value);
            return;
        }

        // ---------------------------------------------------------------------
        // LEDs
        // ---------------------------------------------------------------------

        if ("ledConfig" === key) {
            let container = document.getElementById("leds");
            if (container.childElementCount > 0) {
                return;
            }

            value.leds.forEach((entries) => {
                ledAdd(fromSchema(entries, value.schema));
            });

            addSimpleEnumerables("led", "LED", value.leds.length);
            return;
        }

        // ---------------------------------------------------------------------
        // Domoticz
        // ---------------------------------------------------------------------

        // Domoticz - Relays
        if ("dczRelays" === key) {
            createRelayList(value, "dczRelays", "dcz-relay");
            return;
        }

        // Domoticz - Magnitudes
        //removeIf(!sensor)
        if ("dczMagnitudes" === key) {
            createMagnitudeList(value, "dczMagnitudes", "dcz-magnitude");
            return;
        }
        //endRemoveIf(!sensor)

        // ---------------------------------------------------------------------
        // Thingspeak
        // ---------------------------------------------------------------------

        // Thingspeak - Relays
        if ("tspkRelays" === key) {
            createRelayList(value, "tspkRelays", "tspk-relay");
            return;
        }

        // Thingspeak - Magnitudes
        //removeIf(!sensor)
        if ("tspkMagnitudes" === key) {
            createMagnitudeList(value, "tspkMagnitudes", "tspk-magnitude");
            return;
        }
        //endRemoveIf(!sensor)

        // ---------------------------------------------------------------------
        // General
        // ---------------------------------------------------------------------

        if ("saved" === key) {
            Settings.saved = value;
            return;
        }

        if ("message" === key) {
            window.alert(value);
            return;
        }

        // TODO: squash into a single message, needs a reworked debug buffering
        if ("weblog" === key) {
            send("{}");

            let msg = value["msg"];
            let pre = value["pre"];

            let container = document.getElementById("weblog");
            for (let i = 0; i < msg.length; ++i) {
                if (pre[i]) {
                    container.appendChild(new Text(pre[i]));
                }
                container.appendChild(new Text(msg[i]));
            }

            followScroll(container);
            return;
        }

        if ("deviceip" === key) {
            let span = document.querySelector(`span[data-key='${key}']`);
            span.textContent = value;
            span.parentElement.setAttribute("href", "//".concat(value));
            span.parentElement.nextElementSibling.setAttribute("href", "telnet://".concat(value));
            return;
        }

        if ("uptime" === key) {
            setUptime(value);
            return;
        }

        if ("now" === key) {
            Now = parseInt(value, 10);
            return;
        }

        if ("free_size" === key) {
            FreeSize = parseInt(value, 10);
            initGenericKeyValueElement(key, value);
            return;
        }

        if ("mqttStatus" === key) {
            initGenericKeyValueElement(key, value ? "CONNECTED" : "DISCONNECTED");
            return;
        }

        if ("ntpStatus" === key) {
            initGenericKeyValueElement(key, value ? "SYNC'D" : "NOT SYNC'D");
            return;
        }

        initGenericKeyValueElement(key, value);
    });
}

function updateChanged(event) {
    let action = event.target.dataset["action"];
    if ("none" === action) {
        return;
    }

    let originalValue = event.target.dataset["original"];
    let newValue;

    if ((event.target.tagName === "INPUT") && (event.target.type === "checkbox")) {
        originalValue = stringToBoolean(originalValue);
        newValue = event.target.checked;
    } else if (event.target.tagName === "SELECT") {
        newValue = stringifySelectedValues(event.target);
    } else {
        newValue = event.target.value;
    }

    if (typeof originalValue === "undefined") {
        return;
    }

    let changed = isChangedElement(event.target);
    if (newValue !== originalValue) {
        if (!changed) {
            ++Settings.counters.changed;
            if (action in Settings.counters) {
                ++Settings.counters[action];
            }
        }
        setChangedElement(event.target);
    } else {
        if (changed) {
            --Settings.counters.changed;
            if (action in Settings.counters) {
                --Settings.counters[action];
            }
        }
        resetChangedElement(event.target);
    }
}

function listenChanged(selector) {
    for (let elem of document.querySelectorAll(selector)) {
        elem.addEventListener("change", updateChanged);
    }
}

// -----------------------------------------------------------------------------
// Init & connect
// -----------------------------------------------------------------------------

function onWebSocketMessage(event) {
    var data = {};
    try {
        data = JSON.parse(event.data
            .replace(/\n/g, "\\n")
            .replace(/\r/g, "\\r")
            .replace(/\t/g, "\\t"));
    } catch (e) {
        notifyError(null, null, 0, 0, e);
    }

    processData(data);
}

function webSocketPing() {
    sendAction("ping", {});
}

function onWebSocketOpen() {
    WebsockPingPong = setInterval(webSocketPing, 5000);
}

function onWebSocketClose() {
    clearInterval(WebsockPingPong);
    if (window.confirm("Connection lost with the device, click OK to refresh the page")) {
        let layout = document.getElementById("layout");
        layout.style.display = "none";
        window.location.reload();
    }
}

function connectToURL(url) {
    Urls = new UrlsBase(url);

    fetch(Urls.auth.href, {
        'method': 'GET',
        'cors': true,
        'credentials': 'same-origin'
    }).then((response) => {
        if (response.status === 200) {
            if (Websock) {
                Websock.close();
            }

            Websock = new WebSocket(Urls.ws.href);
            Websock.onmessage = onWebSocketMessage;
            Websock.onclose = onWebSocketClose;
            Websock.onopen = onWebSocketOpen;

            document.getElementById("downloader").href = Urls.config.href;
            return;
        }

        // Nothing to do, reload page and retry on errors
        doReload(5000);
    }).catch((error) => {
        notifyError(null, null, 0, 0, error);
        doReload(5000);
    });
}

function connect(host) {
    if (!host.startsWith("http:") && !host.startsWith("https:")) {
        host = "http://" + host;
    }
    connectToURL(new URL(host));
}

function connectToCurrentURL() {
    connectToURL(new URL(window.location));
}

// TODO: modularize initialization with separate files?

function main() {
    initExternalLinks();
    createCheckboxes(document);
    setInterval(() => keepTime(), 1000);

    elementSelectorOnClick(".menu-link", toggleMenu);
    elementSelectorOnClick(".password-reveal", toggleVisiblePassword);
    elementSelectorOnClick(".pure-menu-link", showPanel);

    elementSelectorOnClick(".button-wifi-scan", doScan);
    elementSelectorOnClick(".button-reboot", doReboot);
    elementSelectorOnClick(".button-reconnect", doReconnect);

    // while the settings are grouped using forms, actual submit is useless here
    // b/c the data is intended to be sent with the websocket connection and never separately
    for (let form of document.forms) {
        if (form.id === "form-dbg") {
            form.addEventListener("submit", (event) => {
                doDebugCommand(event.target.elements.dbgcmd.value);
                event.target.elements.dbgcmd.value = "";
                event.preventDefault();
            });
        } else {
            form.addEventListener("submit", (event) => {
                event.preventDefault();
                event.stopPropagation();
            });
        }
    }
    elementSelectorOnClick(".button-dbg-clear", doDebugClear);

    elementSelectorOnClick(".button-settings-backup", doBackup);
    elementSelectorOnClick(".button-settings-restore", doRestore);
    elementSelectorOnClick(".button-settings-factory", doFactoryReset);

    elementSelectorOnClick(".button-update", doUpdate);
    elementSelectorOnClick(".button-setup-password", doSetupPassword);
    elementSelectorOnClick(".button-generate-password", doGeneratePassword);

    elementSelectorOnClick(".button-upgrade", doUpgrade);
    document.getElementById("uploader").addEventListener("change", onFileUpload);

    elementSelectorOnClick(".button-apikey", generateApiKey);

    let upgrade = document.querySelector("input[name='upgrade']");
    elementSelectorOnClick(".button-upgrade-browse", () => {
        upgrade.click();
    });
    upgrade.addEventListener("change", (event) => {
        document.querySelector("input[name='filename']").value = event.target.files[0].name;
    });

    elementSelectorOnClick(".button-add-settings-group", (event) => {
        const prefix = "settingsGroupDetail";
        const elem = event.target;

        let eventInit = {detail: null};
        for (let key of Object.keys(elem.dataset)) {
            if (!key.startsWith(prefix)) {
                continue
            }
            if (eventInit.detail === null) {
                eventInit.detail = {};
            }

            let eventKey = key.replace(prefix, "");
            eventKey = eventKey[0].toLowerCase() + eventKey.slice(1);
            eventInit.detail[eventKey] = elem.dataset[key];
        }

        const group = document.getElementById(elem.dataset["settingsGroup"]);
        group.dispatchEvent(new CustomEvent("settings-group-add", eventInit));
    });
    //endRemoveIf(!curtain)

    //removeIf(!garland)
    elementSelectorListener(".checkbox-garland-enable", "change", (event) => {
        sendAction("garland_switch", {status: event.target.checked ? 1 : 0});
    });

    elementSelectorListener(".slider-garland-brightness", "change", (event) => {
        sendAction("garland_set_brightness", {brightness: event.target.value});
    });
    elementSelectorListener(".slider-garland-speed", "change", (event) => {
        sendAction("garland_set_speed", {speed: event.target.value});
    });

    elementSelectorOnClick(".button-garland-set-default", () => {
        sendAction("garland_set_default", {});
    });
    //endRemoveIf(!garland)

    //removeIf(!thermostat)
    elementSelectorOnClick(".button-thermostat-reset-counters", () => {
        doAction("Are you sure you want to reset burning counters?", "thermostat_reset_counters");
    });
    elementSelectorListener("#tempRangeMaxInput", "change", thermostatCheckTempRange);
    elementSelectorListener("#tempRangeMinInput", "change", thermostatCheckTempRange);
    //endRemoveIf(!thermostat)

    //removeIf(!lightfox)
    elementSelectorOnClick(".button-lightfox-learn", lightfoxLearn);
    elementSelectorOnClick(".button-lightfox-clear", lightfoxClear);
    //endRemoveIf(!lightfox)

    //removeIf(!rfm69)
    elementSelectorOnClick(".button-clear-counts", rfm69ClearCounters);
    elementSelectorOnClick(".button-clear-messages", rfm69ClearMessages);

    elementSelectorOnClick(".button-clear-filters", rfm69ClearFilters);
    elementSelectorOnClick("#rfm69-messages tbody", rfm69FilterEvent);

    groupSettingsOnAdd("rfm69-mapping", () => {
        rfm69AddMapping();
    });
    //endRemoveIf(!rfm69)

    listenChanged(".settings-group");
    listenChanged("input,select");

    resetOriginals();

    groupSettingsOnAdd("networks", () => {
        networkAdd();
    });
    groupSettingsOnAdd("leds", () => {
        ledAdd();
    });
    groupSettingsOnAdd("schedules", () => {
        const type = (event.detail.target === "switch") ? 1 :
            (event.detail.target === "light") ? 2 :
            (event.detail.target === "curtain") ? 3 : 0;

        if (type !== 0) {
            schAdd({schType: type});
            return;
        }

        event.preventDefault();
        event.stopPropagation();
    });

    groupSettingsOnAdd("rpn-rules", () => {
        rpnAddRule();
    });
    groupSettingsOnAdd("rpn-topics", () => {
        rpnAddTopic();
    });

    // TODO: make sure to never register new handlers for settings-group-add after this point
    for (let group of document.querySelectorAll(".settings-group")) {
        GroupSettingsObserver.observe(group, {childList: true});
        group.addEventListener("settings-group-add", groupSettingsHandleUpdate, false);
    }

    // don't autoconnect when opening from filesystem
    if (window.location.protocol === "file:") {
        processData({"webMode": 0});
        return;
    }

    // Check host param in query string
    const search = new URLSearchParams(window.location.search),
        host = search.get("host");

    if (host !== null) {
        connect(host);
    } else {
        connectToCurrentURL();
    }
}

document.addEventListener("DOMContentLoaded", main);
