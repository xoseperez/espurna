var debug = false;
var websock;
var password = false;
var maxNetworks;
var messages = [];
var free_size = 0;

var urls = {};

var numChanged = 0;
var numReboot = 0;
var numReconnect = 0;
var numReload = 0;
var configurationSaved = false;
var ws_pingpong;

var useWhite = false;
var useCCT = false;

var now = 0;
var ago = 0;

<!-- removeIf(!rfm69)-->
var packets;
var filters = [];
<!-- endRemoveIf(!rfm69)-->

<!-- removeIf(!sensor)-->
var magnitudes = [];
<!-- endRemoveIf(!sensor)-->

// -----------------------------------------------------------------------------
// Messages
// -----------------------------------------------------------------------------

function initMessages() {
    messages[1]  = "Remote update started";
    messages[2]  = "OTA update started";
    messages[3]  = "Error parsing data!";
    messages[4]  = "The file does not look like a valid configuration backup or is corrupted";
    messages[5]  = "Changes saved. You should reboot your board now";
    messages[7]  = "Passwords do not match!";
    messages[8]  = "Changes saved";
    messages[9]  = "No changes detected";
    messages[10] = "Session expired, please reload page...";
}

<!-- removeIf(!sensor)-->
function sensorName(id) {
    var names = [
        "DHT", "Dallas", "Emon Analog", "Emon ADC121", "Emon ADS1X15",
        "HLW8012", "V9261F", "ECH1560", "Analog", "Digital",
        "Events", "PMSX003", "BMX280", "MHZ19", "SI7021",
        "SHT3X I2C", "BH1750", "PZEM004T", "AM2320 I2C", "GUVAS12SD",
        "T6613", "TMP3X", "Sonar", "SenseAir", "GeigerTicks", "GeigerCPM",
        "NTC", "SDS011", "MICS2710", "MICS5525", "VL53L1X", "VEML6075",
        "EZOPH"
    ];
    if (1 <= id && id <= names.length) {
        return names[id - 1];
    }
    return null;
}

function magnitudeType(type) {
    var types = [
        "Temperature", "Humidity", "Pressure",
        "Current", "Voltage", "Active Power", "Apparent Power",
        "Reactive Power", "Power Factor", "Energy", "Energy (delta)",
        "Analog", "Digital", "Event",
        "PM1.0", "PM2.5", "PM10", "CO2", "Lux", "UVA", "UVB", "UV Index", "Distance" , "HCHO",
        "Local Dose Rate", "Local Dose Rate",
        "Count",
        "NO2", "CO", "Resistance", "pH"
    ];
    if (1 <= type && type <= types.length) {
        return types[type - 1];
    }
    return null;
}

function magnitudeError(error) {
    var errors = [
        "OK", "Out of Range", "Warming Up", "Timeout", "Wrong ID",
        "Data Error", "I2C Error", "GPIO Error", "Calibration error"
    ];
    if (0 <= error && error < errors.length) {
        return errors[error];
    }
    return "Error " + error;
}
<!-- endRemoveIf(!sensor)-->

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------

$.fn.enterKey = function (fnc) {
    return this.each(function () {
        $(this).keypress(function (ev) {
            var keycode = parseInt(ev.keyCode ? ev.keyCode : ev.which, 10);
            if (13 === keycode) {
                return fnc.call(this, ev);
            }
        });
    });
};

function keepTime() {

    $("span[name='ago']").html(ago);
    ago++;

    if (0 === now) { return; }
    var date = new Date(now * 1000);
    var text = date.toISOString().substring(0, 19).replace("T", " ");
    $("input[name='now']").val(text);
    $("span[name='now']").html(text);
    now++;

}

function zeroPad(number, positions) {
    return number.toString().padStart(positions, "0");
}

function loadTimeZones() {

    var time_zones = [
        -720, -660, -600, -570, -540,
        -480, -420, -360, -300, -240,
        -210, -180, -120, -60, 0,
        60, 120, 180, 210, 240,
        270, 300, 330, 345, 360,
        390, 420, 480, 510, 525,
        540, 570, 600, 630, 660,
        720, 765, 780, 840
    ];

    for (var i in time_zones) {
        var tz = time_zones[i];
        var offset = tz >= 0 ? tz : -tz;
        var text = "GMT" + (tz >= 0 ? "+" : "-") +
            zeroPad(parseInt(offset / 60, 10), 2) + ":" +
            zeroPad(offset % 60, 2);
        $("select[name='ntpOffset']").append(
            $("<option></option>")
                .attr("value", tz)
                .text(text)
        );
    }

}

function validatePassword(password) {
    // http://www.the-art-of-web.com/javascript/validate-password/
    // at least one lowercase and one uppercase letter or number
    // at least eight characters (letters, numbers or special characters)

    // MUST be 8..63 printable ASCII characters. See:
    // https://en.wikipedia.org/wiki/Wi-Fi_Protected_Access#Target_users_(authentication_key_distribution)
    // https://github.com/xoseperez/espurna/issues/1151

    var re_password = /^(?=.*[A-Z\d])(?=.*[a-z])[\w~!@#$%^&*\(\)<>,.\?;:{}\[\]\\|]{8,63}$/;
    return (
        (password !== undefined)
        && (typeof password === "string")
        && (password.length > 0)
        && re_password.test(password)
    );
}

function validateFormPasswords(form) {
    var passwords = $("input[name='adminPass1'],input[name='adminPass2']", form);
    var adminPass1 = passwords.first().val(),
        adminPass2 = passwords.last().val();

    var formValidity = passwords.first()[0].checkValidity();
    if (formValidity && (adminPass1.length === 0) && (adminPass2.length === 0)) {
        return true;
    }

    var validPass1 = validatePassword(adminPass1),
        validPass2 = validatePassword(adminPass2);

    if (formValidity && validPass1 && validPass2) {
        return true;
    }

    if (!formValidity || (adminPass1.length > 0 && !validPass1)) {
        alert("The password you have entered is not valid, it must be 8..63 characters and have at least 1 lowercase and 1 uppercase / number!");
    }

    if (adminPass1 !== adminPass2) {
        alert("Passwords are different!");
    }

    return false;
}

function validateFormHostname(form) {
    // RFCs mandate that a hostname's labels may contain only
    // the ASCII letters 'a' through 'z' (case-insensitive),
    // the digits '0' through '9', and the hyphen.
    // Hostname labels cannot begin or end with a hyphen.
    // No other symbols, punctuation characters, or blank spaces are permitted.

    // Negative lookbehind does not work in Javascript
    // var re_hostname = new RegExp('^(?!-)[A-Za-z0-9-]{1,31}(?<!-)$');

    var re_hostname = new RegExp('^(?!-)[A-Za-z0-9-]{0,30}[A-Za-z0-9]$');

    var hostname = $("input[name='hostname']", form);
    if ("true" !== hostname.attr("hasChanged")) {
        return true;
    }

    if (re_hostname.test(hostname.val())) {
        return true;
    }

    alert("Hostname cannot be empty and may only contain the ASCII letters ('A' through 'Z' and 'a' through 'z'), the digits '0' through '9', and the hyphen ('-')! They can neither start or end with an hyphen.");

    return false;
}

function validateForm(form) {
    return validateFormPasswords(form) && validateFormHostname(form);
}

// Observe all group settings to selectively update originals based on the current data
var groupSettingsObserver = new MutationObserver(function(mutations) {
    mutations.forEach(function(mutation) {
        // If any new elements are added, set "settings-target" element as changed to forcibly send the data
        var targets = $(mutation.target).attr("data-settings-target");
        if (targets !== undefined) {
            mutation.addedNodes.forEach(function(node) {
                var overrides = [];
                targets.split(" ").forEach(function(target) {
                    var elem = $("[name='" + target + "']", node);
                    if (!elem.length) return;

                    var value = getValue(elem);
                    if ((value === null) || (value === elem[0].defaultValue)) {
                        overrides.push(elem);
                    }
                });
                setOriginalsFromValues($("input,select", node));
                overrides.forEach(function(elem) {
                    elem.attr("hasChanged", "true");
                    if (elem.prop("tagName") === "SELECT") {
                        elem.prop("value", 0);
                    }
                });
            });
        }

        // If anything was removed, forcibly send **all** of the group to avoid having any outdated keys
        // TODO: hide instead of remove?
        var changed = $(mutation.target).attr("hasChanged") === "true";
        if (changed || mutation.removedNodes.length) {
            $(mutation.target).attr("hasChanged", "true");
            $("input,select", mutation.target.childNodes).attr("hasChanged", "true");
        }
    });
});

// These fields will always be a list of values
function isGroupValue(value) {
    var names = [
        "ssid", "pass", "gw", "mask", "ip", "dns",
        "schEnabled", "schSwitch","schAction","schType","schHour","schMinute","schWDs","schUTC",
        "relayBoot", "relayPulse", "relayTime", "relayLastSch",
        "mqttGroup", "mqttGroupSync", "relayOnDisc",
        "dczRelayIdx", "dczMagnitude",
        "tspkRelay", "tspkMagnitude",
        "ledGPIO", "ledMode", "ledRelay",
        "adminPass",
        "node", "key", "topic",
        "rpnRule", "rpnTopic", "rpnName"
    ];
    return names.indexOf(value) >= 0;
}

function getValue(element) {

    if ($(element).attr("type") === "checkbox") {
        return $(element).prop("checked") ? 1 : 0;
    } else if ($(element).attr("type") === "radio") {
        if (!$(element).prop("checked")) {
            return null;
        }
    }

    return $(element).val();

}

function addValue(data, name, value) {

    if (name in data) {
        if (!Array.isArray(data[name])) {
            data[name] = [data[name]];
        }
        data[name].push(value);
    } else if (isGroupValue(name)) {
        data[name] = [value];
    } else {
        data[name] = value;
    }

}

function getData(form, changed, cleanup) {

    // Populate two sets of data, ones that had been changed and ones that stayed the same
    var data = {};
    var changed_data = [];
    if (cleanup === undefined) {
        cleanup = true;
    }

    if (changed === undefined) {
        changed = true;
    }

    $("input,select", form).each(function() {
        if ($(this).attr("data-settings-ignore") === "true") {
            return;
        }

        var name = $(this).attr("name");

        var real_name = $(this).attr("data-settings-real-name");
        if (real_name !== undefined) {
            name = real_name;
        }

        var value = getValue(this);
        if (null !== value) {
            var haschanged = ("true" === $(this).attr("hasChanged"));
            var indexed = changed_data.indexOf(name) >= 0;

            if ((haschanged || !changed) && !indexed) {
                changed_data.push(name);
            }

            addValue(data, name, value);
        }
    });

    // Finally, filter out only fields that had changed.
    // Note: We need to preserve dynamic lists like schedules, wifi etc.
    // so we don't accidentally break when user deletes entry in the middle
    var resulting_data = {};
    for (var value in data) {
        if (changed_data.indexOf(value) >= 0) {
            resulting_data[value] = data[value];
        }
    }

    // Hack: clean-up leftover arrays.
    // When empty, the receiving side will prune all keys greater than the current one.
    if (cleanup) {
        $(".group-settings").each(function() {
            var haschanged = ("true" === $(this).attr("hasChanged"));
            if (haschanged && !this.children.length) {
                var targets = this.dataset.settingsTarget;
                if (targets === undefined) return;

                targets.split(" ").forEach(function(target) {
                    resulting_data[target] = [];
                });
            }
        });
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

    window.crypto.getRandomValues(source).forEach(function(value, i) {
        result[i] = mask[value % mask.length];
    });

    return result.join("");
}

function generateAPIKey() {
    var apikey = randomString(16, {hex: true});
    $("input[name='apiKey']")
        .val(apikey)
        .attr("original", "-".repeat(16))
        .attr("haschanged", "true");
    return false;
}

function generatePassword() {
    var password = "";
    do {
        password = randomString(10);
    } while (!validatePassword(password));

    return password;
}

function toggleVisiblePassword() {
    var elem = this.previousElementSibling;
    if (elem.type === "password") {
        elem.type = "text";
    } else {
        elem.type = "password";
    }
    return false;
}

function doGeneratePassword() {
    var elems = $("input", $("#formPassword"));
    elems
        .val(generatePassword())
        .attr("haschanged", "true")
        .each(function() {
            this.type = "text";
        });
    return false;
}

function getJson(str) {
    try {
        return JSON.parse(str);
    } catch (e) {
        return false;
    }
}

<!-- removeIf(!thermostat)-->
function checkTempRangeMin() {
    var min = parseInt($("#tempRangeMinInput").val(), 10);
    var max = parseInt($("#tempRangeMaxInput").val(), 10);
    if (min > max - 1) {
        $("#tempRangeMinInput").val(max - 1);
    }
}

function checkTempRangeMax() {
    var min = parseInt($("#tempRangeMinInput").val(), 10);
    var max = parseInt($("#tempRangeMaxInput").val(), 10);
    if (max < min + 1) {
        $("#tempRangeMaxInput").val(min + 1);
    }
}

function doResetThermostatCounters(ask) {
    var question = (typeof ask === "undefined" || false === ask) ?
        null :
        "Are you sure you want to reset burning counters?";
    return doAction(question, "thermostat_reset_counters");
}
<!-- endRemoveIf(!thermostat)-->

function initSelectGPIO(select) {
    // TODO: properly lock used GPIOs via locking and apply the mask here
    var mapping = [
        [153, "NONE"],
        [0, "0 (FLASH)"],
        [1, "1 (U0TXD)"],
        [2, "2 (U1TXD)"],
        [3, "3 (U0RXD)"],
        [4, "4 (SDA)"],
        [5, "5 (SCL)"],
        [9, "9 (SDD2)"],
        [10, "10 (SDD3)"],
        [12, "12 (MTDI)"],
        [13, "13 (MTCK)"],
        [14, "14 (MTMS)"],
        [15, "15 (MTDO)"],
        [16, "16 (WAKE)"],
    ];
    for (n in mapping) {
        var elem = $('<option value="' + mapping[n][0] + '">');
        elem.html(mapping[n][1]);
        elem.appendTo(select);
    }
}


// -----------------------------------------------------------------------------
// Actions
// -----------------------------------------------------------------------------

function send(json) {
    if (debug) console.log(json);
    websock.send(json);
}

function sendAction(action, data) {
    send(JSON.stringify({action: action, data: data}));
}

function sendConfig(data) {
    send(JSON.stringify({config: data}));
}

function setOriginalsFromValues(elems) {
    if (typeof elems == "undefined") {
        elems = $("input,select");
    }
    elems.each(function() {
        var value;
        if ($(this).attr("type") === "checkbox") {
            value = $(this).prop("checked");
        } else {
            value = $(this).val();
        }
        $(this).attr("original", value);
        hasChanged.call(this);
    });
}

function resetOriginals() {
    setOriginalsFromValues();
    $(".group-settings").attr("haschanged", "false")
    numReboot = numReconnect = numReload = 0;
    configurationSaved = false;
}

function doReload(milliseconds) {
    setTimeout(function() {
        window.location.reload();
    }, parseInt(milliseconds, 10));
}

/**
 * Check a file object to see if it is a valid firmware image
 * The file first byte should be 0xE9
 * @param  {file}       file        File object
 * @param  {Function}   callback    Function to call back with the result
 */
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

function doUpgrade() {

    var file = $("input[name='upgrade']")[0].files[0];

    if (typeof file === "undefined") {
        alert("First you have to select a file from your computer.");
        return false;
    }

    if (file.size > free_size) {
        alert("Image it too large to fit in the available space for OTA. Consider doing a two-step update.");
        return false;
    }

    checkFirmware(file, function(ok) {

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

        xhr.addEventListener("load", function(e) {
            $("#upgrade-progress").hide();
            if ("OK" === xhr.responseText) {
                alert(msg_ok);
                doReload(5000);
            } else {
                alert(msg_err + xhr.status.toString() + " " + xhr.statusText + ", " + xhr.responseText);
            }
        }, false);

        xhr.upload.addEventListener("progress", function(e) {
            $("#upgrade-progress").show();
            if (e.lengthComputable) {
                $("progress").attr({ value: e.loaded, max: e.total });
            }
        }, false);

        xhr.open("POST", urls.upgrade.href);
        xhr.send(data);

    });

    return false;

}

function doUpdatePassword() {
    var form = $("#formPassword");
    if (validateFormPasswords(form)) {
        sendConfig(getData(form, true, false));
    }
    return false;
}

function checkChanges() {

    if (numChanged > 0) {
        var response = window.confirm("Some changes have not been saved yet, do you want to save them first?");
        if (response) {
            doUpdate();
        }
    }

}

function doAction(question, action) {

    checkChanges();

    if (question) {
        var response = window.confirm(question);
        if (false === response) {
            return false;
        }
    }

    sendAction(action, {});
    doReload(5000);
    return false;

}

function doReboot(ask) {

    var question = (typeof ask === "undefined" || false === ask) ?
        null :
        "Are you sure you want to reboot the device?";
    return doAction(question, "reboot");

}

function doReconnect(ask) {

    var question = (typeof ask === "undefined" || false === ask) ?
        null :
        "Are you sure you want to disconnect from the current WIFI network?";
    return doAction(question, "reconnect");

}

function doCheckOriginals() {
    var response;

    if (numReboot > 0) {
        response = window.confirm("You have to reboot the board for the changes to take effect, do you want to do it now?");
        if (response) { doReboot(false); }
    } else if (numReconnect > 0) {
        response = window.confirm("You have to reconnect to the WiFi for the changes to take effect, do you want to do it now?");
        if (response) { doReconnect(false); }
    } else if (numReload > 0) {
        response = window.confirm("You have to reload the page to see the latest changes, do you want to do it now?");
        if (response) { doReload(0); }
    }

    resetOriginals();
}

function waitForSave(){
    if (!configurationSaved) {
        setTimeout(waitForSave, 1000);
    } else {
        doCheckOriginals();
    }
}

function doUpdate() {

    var forms = $(".form-settings");
    if (validateForm(forms)) {

        // Get data
        sendConfig(getData(forms));

        // Empty special fields
        $(".pwrExpected").val(0);
        $("input[name='snsResetCalibration']").prop("checked", false);
        $("input[name='pwrResetCalibration']").prop("checked", false);
        $("input[name='pwrResetE']").prop("checked", false);

        // Change handling
        numChanged = 0;

        waitForSave();

    }

    return false;

}

function doBackup() {
    document.getElementById("downloader").src = urls.config.href;
    return false;
}

function onFileUpload(event) {

    var inputFiles = this.files;
    if (typeof inputFiles === "undefined" || inputFiles.length === 0) {
        return false;
    }
    var inputFile = inputFiles[0];
    this.value = "";

    var response = window.confirm("Previous settings will be overwritten. Are you sure you want to restore this settings?");
    if (!response) {
        return false;
    }

    var reader = new FileReader();
    reader.onload = function(e) {
        var data = getJson(e.target.result);
        if (data) {
            sendAction("restore", data);
        } else {
            window.alert(messages[4]);
        }
    };
    reader.readAsText(inputFile);

    return false;

}

function doRestore() {
    if (typeof window.FileReader !== "function") {
        alert("The file API isn't supported on this browser yet.");
    } else {
        $("#uploader").click();
    }
    return false;
}

function doFactoryReset() {
    var response = window.confirm("Are you sure you want to restore to factory settings?");
    if (!response) {
        return false;
    }
    sendAction("factory_reset", {});
    doReload(5000);
    return false;
}

function doToggle(id, value) {
    sendAction("relay", {id: id, status: value ? 1 : 0 });
    return false;
}

function doScan() {
    $("#scanResult").html("");
    $("div.scan.loading").show();
    sendAction("scan", {});
    return false;
}

function doHAConfig() {
    $("#haConfig")
        .text("")
        .height(0)
        .show();
    sendAction("haconfig", {});
    return false;
}

function doDebugCommand() {
    var el = $("input[name='dbgcmd']");
    var command = el.val();
    el.val("");
    sendAction("dbgcmd", {command: command});
    return false;
}

function doDebugClear() {
    $("#weblog").text("");
    return false;
}

<!-- removeIf(!rfm69)-->

function doClearCounts() {
    sendAction("clear-counts", {});
    return false;
}

function doClearMessages() {
    packets.clear().draw(false);
    return false;
}

function doFilter(e) {
    var index = packets.cell(this).index();
    if (index == 'undefined') return;
    var c = index.column;
    var column = packets.column(c);
    if (filters[c]) {
        filters[c] = false;
        column.search("");
        $(column.header()).removeClass("filtered");
    } else {
        filters[c] = true;
        var data = packets.row(this).data();
        if (e.which == 1) {
            column.search('^' + data[c] + '$', true, false );
        } else {
            column.search('^((?!(' + data[c] + ')).)*$', true, false );
        }
        $(column.header()).addClass("filtered");
    }
    column.draw();
    return false;
}

function doClearFilters() {
    for (var i = 0; i < packets.columns()[0].length; i++) {
        if (filters[i]) {
            filters[i] = false;
            var column = packets.column(i);
            column.search("");
            $(column.header()).removeClass("filtered");
            column.draw();
        }
    }
    return false;
}

<!-- endRemoveIf(!rfm69)-->

function delParent() {
    var parent = $(this).parent().parent();
    $(parent).remove();
}

// -----------------------------------------------------------------------------
// Visualization
// -----------------------------------------------------------------------------

function toggleMenu() {
    $("#layout").toggleClass("active");
    $("#menu").toggleClass("active");
    $("#menuLink").toggleClass("active");
}

function showPanel() {
    $(".panel").hide();
    if ($("#layout").hasClass("active")) { toggleMenu(); }
    $("#" + $(this).attr("data")).show();
}

// -----------------------------------------------------------------------------
// Relays & magnitudes mapping
// -----------------------------------------------------------------------------

function createRelayList(data, container, template_name) {

    var current = $("#" + container + " > div").length;
    if (current > 0) { return; }

    var template = $("#" + template_name + " .pure-g")[0];
    for (var i in data) {
        var line = $(template).clone();
        $("label", line).html("Switch #" + i);
        $("input", line).attr("tabindex", 40 + i).val(data[i]);
        setOriginalsFromValues($("input", line));
        line.appendTo("#" + container);
    }

}

<!-- removeIf(!sensor)-->
function createMagnitudeList(data, container, template_name) {

    var current = $("#" + container + " > div").length;
    if (current > 0) { return; }

    var template = $("#" + template_name + " .pure-g")[0];
    var size = data.size;

    for (var i=0; i<size; ++i) {
        var line = $(template).clone();
        $("label", line).html(magnitudeType(data.type[i]) + " #" + parseInt(data.index[i], 10));
        $("div.hint", line).html(magnitudes[i].description);
        $("input", line).attr("tabindex", 40 + i).val(data.idx[i]);
        setOriginalsFromValues($("input", line));
        line.appendTo("#" + container);
    }

}
<!-- endRemoveIf(!sensor)-->

// -----------------------------------------------------------------------------
// RPN Rules
// -----------------------------------------------------------------------------

function addRPNRule() {
    var template = $("#rpnRuleTemplate .pure-g")[0];
    var line = $(template).clone();
    var tabindex = $("#rpnRules > div").length + 100;
    $(line).find("input").each(function() {
        $(this).attr("tabindex", tabindex++);
    });
    $(line).find("button").on('click', delParent);
    setOriginalsFromValues($("input", line));
    line.appendTo("#rpnRules");
}

function addRPNTopic() {
    var template = $("#rpnTopicTemplate .pure-g")[0];
    var line = $(template).clone();
    var tabindex = $("#rpnTopics > div").length + 120;
    $(line).find("input").each(function() {
        $(this).attr("tabindex", tabindex++);
    });
    $(line).find("button").on('click', delParent);
    setOriginalsFromValues($("input", line));
    line.appendTo("#rpnTopics");
}

// -----------------------------------------------------------------------------
// RFM69
// -----------------------------------------------------------------------------

<!-- removeIf(!rfm69)-->

function addMapping() {
    var template = $("#nodeTemplate .pure-g")[0];
    var line = $(template).clone();
    var tabindex = $("#mapping > div").length * 3 + 50;
    $(line).find("input").each(function() {
        $(this).attr("tabindex", tabindex++);
    });
    $(line).find("button").on('click', delParent);
    setOriginalsFromValues($("input", line));
    line.appendTo("#mapping");
}

<!-- endRemoveIf(!rfm69)-->

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

function numNetworks() {
    return $("#networks > div").length;
}

function delNetwork() {
    var parent = $(this).parents(".pure-g");
    $(parent).remove();
}

function moreNetwork() {
    var parent = $(this).parents(".pure-g");
    $(".more", parent).toggle();
}

function addNetwork(network) {

    var number = numNetworks();
    if (number >= maxNetworks) {
        alert("Max number of networks reached");
        return null;
    }

    if (network === undefined) {
        network = {};
    }

    var tabindex = 200 + number * 10;
    var template = $("#networkTemplate").children();
    var line = $(template).clone();
    $(line).find("input").each(function() {
        $(this).attr("tabindex", tabindex);
        tabindex++;
    });
    $(".password-reveal", line).on("click", toggleVisiblePassword);
    $(line).find(".button-del-network").on("click", delNetwork);
    $(line).find(".button-more-network").on("click", moreNetwork);

    Object.entries(network).forEach(function(pair) {
        // XXX: UI deleting this network will only re-use stored values.
        var key = pair[0],
            val = pair[1];
        if (key === "stored") {
            $(line).find(".button-del-network").prop("disabled", val);
            return;
        }
        $("input[name='" + key + "']", line).val(val);
    });


    line.appendTo("#networks");

    return line;

}

// -----------------------------------------------------------------------------
// Relays scheduler
// -----------------------------------------------------------------------------

function numSchedules() {
    return $("#schedules > div").length;
}

function maxSchedules() {
    var value = $("#schedules").attr("data-settings-max");
    return parseInt(value === undefined ? 0 : value, 10);
}

function delSchedule() {
    var parent = $(this).parents(".pure-g");
    $(parent).remove();
}

function moreSchedule() {
    var parent = $(this).parents(".pure-g");
    $("div.more", parent).toggle();
}

function addSchedule(values) {

    var schedules = numSchedules();
    if (schedules >= maxSchedules()) {
        alert("Max number of schedules reached");
        return null;
    }

    if (values === undefined) {
        values = {};
    }

    var tabindex = 200 + schedules * 10;
    var template = $("#scheduleTemplate").children();
    var line = $(template).clone();

    var type = (1 === values.schType) ? "switch" : "light";

    template = $("#" + type + "ActionTemplate").children();
    $(line).find("#schActionDiv").append(template.clone());

    $(line).find("input").each(function() {
        $(this).attr("tabindex", tabindex);
        tabindex++;
    });
    $(line).find(".button-del-schedule").on("click", delSchedule);
    $(line).find(".button-more-schedule").on("click", moreSchedule);

    var schUTC_id = "schUTC" + schedules;
    $(line).find("input[name='schUTC']").prop("id", schUTC_id).next().prop("for", schUTC_id);

    var schEnabled_id = "schEnabled" + schedules;
    $(line).find("input[name='schEnabled']").prop("id", schEnabled_id).next().prop("for", schEnabled_id);

    $(line).find("input[type='checkbox']").prop("checked", false);

    Object.entries(values).forEach(function(kv) {
        var key = kv[0], value = kv[1];
        $("input[name='" + key + "']", line).val(value);
        $("select[name='" + key + "']", line).prop("value", value);
        $("input[type='checkbox'][name='" + key + "']", line).prop("checked", value);
    });
    line.appendTo("#schedules");

    return line;

}

// -----------------------------------------------------------------------------
// Relays
// -----------------------------------------------------------------------------

function initRelays(data) {

    var current = $("#relays > div").length;
    if (current > 0) { return; }

    var template = $("#relayTemplate .pure-g")[0];
    for (var i=0; i<data.length; i++) {

        // Add relay fields
        var line = $(template).clone();
        $(".id", line).html(i);
        $(":checkbox", line).prop('checked', data[i]).attr("data", i)
            .prop("id", "relay" + i)
            .on("change", function (event) {
                var id = parseInt($(event.target).attr("data"), 10);
                var status = $(event.target).prop("checked");
                doToggle(id, status);
            });
        $("label.toggle", line).prop("for", "relay" + i)
        line.appendTo("#relays");

    }

}

function updateRelays(data) {
    var size = data.size;
    for (var i=0; i<size; ++i) {
        var elem = $("input[name='relay'][data='" + i + "']");
        elem.prop("checked", data.status[i]);
        var lock = {
            0: false,
            1: !data.status[i],
            2: data.status[i]
        };
        elem.prop("disabled", lock[data.lock[i]]); // RELAY_LOCK_DISABLED=0
    }
}

function createCheckboxes() {

    $("input[type='checkbox']").each(function() {

        if($(this).prop("name"))$(this).prop("id", $(this).prop("name"));
        $(this).parent().addClass("toggleWrapper");
        $(this).after('<label for="' + $(this).prop("name") + '" class="toggle"><span class="toggle__handler"></span></label>')

    });

}

function initRelayConfig(data) {

    var current = $("#relayConfig > legend").length; // there is a legend per relay
    if (current > 0) { return; }

    var size = data.size;
    var start = data.start;

    var template = $("#relayConfigTemplate").children();

    for (var i=start; i<size; ++i) {
        var line = $(template).clone();

        $("span.id", line).html(i);
        $("span.gpio", line).html(data.gpio[i]);
        $("select[name='relayBoot']", line).val(data.boot[i]);
        $("select[name='relayPulse']", line).val(data.pulse[i]);
        $("input[name='relayTime']", line).val(data.pulse_time[i]);

        if ("sch_last" in data) {
            $("input[name='relayLastSch']", line)
                .prop('checked', data.sch_last[i])
                .attr("id", "relayLastSch" + i)
                .attr("name", "relayLastSch" + i)
                .next().attr("for","relayLastSch" + (i));
        }

        if ("group" in data) {
            $("input[name='mqttGroup']", line).val(data.group[i]);
        }
        if ("group_sync" in data) {
            $("select[name='mqttGroupSync']", line).val(data.group_sync[i]);
        }
        if ("on_disc" in data) {
            $("select[name='relayOnDisc']", line).val(data.on_disc[i]);
        }

        setOriginalsFromValues($("input,select", line));
        line.appendTo("#relayConfig");

        // Populate the relay SELECTs
        $("select.isrelay").append(
            $("<option></option>")
                .attr("value", i)
                .text("Switch #" + i)
        );

    }

}

// -----------------------------------------------------------------------------
// Sensors & Magnitudes
// -----------------------------------------------------------------------------

<!-- removeIf(!sensor)-->
function initMagnitudes(data) {

    // check if already initialized (each magnitude is inside div.pure-g)
    var done = $("#magnitudes > div").length;
    if (done > 0) { return; }

    var size = data.size;

    // add templates
    var template = $("#magnitudeTemplate").children();

    for (var i=0; i<size; ++i) {
        var magnitude = {
            "name": magnitudeType(data.type[i]) + " #" + parseInt(data.index[i], 10),
            "units": data.units[i],
            "description": data.description[i]
        };
        magnitudes.push(magnitude);

        var line = $(template).clone();
        $("label", line).html(magnitude.name);
        $("input", line).attr("data", i);
        $("div.sns-desc", line).html(magnitude.description);
        $("div.sns-info", line).hide();
        line.appendTo("#magnitudes");
    }

}
<!-- endRemoveIf(!sensor)-->

// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

<!-- removeIf(!light)-->

// wheelColorPicker accepts:
//   hsv(0...360,0...1,0...1)
//   hsv(0...100%,0...100%,0...100%)
// While we use:
//   hsv(0...360,0...100%,0...100%)

function _hsv_round(value) {
    return Math.round(value * 100) / 100;
}

function getPickerRGB(picker) {
    return $(picker).wheelColorPicker("getValue", "css");
}

function setPickerRGB(picker, value) {
    $(picker).wheelColorPicker("setValue", value, true);
}

// TODO: use pct values instead of doing conversion?
function getPickerHSV(picker) {
    var color = $(picker).wheelColorPicker("getColor");
    return String(Math.ceil(_hsv_round(color.h) * 360))
        + "," + String(Math.ceil(_hsv_round(color.s) * 100))
        + "," + String(Math.ceil(_hsv_round(color.v) * 100));
}

function setPickerHSV(picker, value) {
    if (value === getPickerHSV(picker)) return;
    var chunks = value.split(",");
    $(picker).wheelColorPicker("setColor", {
        h: _hsv_round(chunks[0] / 360),
        s: _hsv_round(chunks[1] / 100),
        v: _hsv_round(chunks[2] / 100)
    });
}

function initColor(cfg) {
    var rgb = false;
    if (typeof cfg === "object") {
        rgb = cfg.rgb;
    }

    // check if already initialized
    var done = $("#colors > div").length;
    if (done > 0) { return; }

    // add template
    var template = $("#colorTemplate").children();
    var line = $(template).clone();
    line.appendTo("#colors");

    // init color wheel
    $("input[name='color']").wheelColorPicker({
        sliders: (rgb ? "wrgbp" : "whsp")
    }).on("sliderup", function() {
        if (rgb) {
            sendAction("color", {rgb: getPickerRGB(this)});
        } else {
            sendAction("color", {hsv: getPickerHSV(this)});
        }
    });

}

function initCCT() {

  // check if already initialized
  var done = $("#cct > div").length;
  if (done > 0) { return; }

  $("#miredsTemplate").children().clone().appendTo("#cct");

  $("#mireds").on("change", function() {
    var value = $(this).val();
    var parent = $(this).parents(".pure-g");
    $("span", parent).html(value);
    sendAction("mireds", {mireds: value});
  });
}

function initChannels(num) {

    // check if already initialized
    var done = $("#channels > div").length > 0;
    if (done) { return; }

    // does it have color channels?
    var colors = $("#colors > div").length > 0;

    // calculate channels to create
    var max = num;
    if (colors) {
        max = num % 3;
        if ((max > 0) & useWhite) {
            max--;
            if (useCCT) {
              max--;
            }
        }
    }
    var start = num - max;

    var onChannelSliderChange = function() {
        var id = $(this).attr("data");
        var value = $(this).val();
        var parent = $(this).parents(".pure-g");
        $("span", parent).html(value);
        sendAction("channel", {id: id, value: value});
    };

    // add channel templates
    var i = 0;
    var template = $("#channelTemplate").children();
    for (i=0; i<max; i++) {

        var channel_id = start + i;
        var line = $(template).clone();
        $("span.slider", line).attr("data", channel_id);
        $("input.slider", line).attr("data", channel_id).on("change", onChannelSliderChange);
        $("label", line).html("Channel #" + channel_id);

        line.appendTo("#channels");

    }

    // Init channel dropdowns
    for (i=0; i<num; i++) {
        $("select.islight").append(
            $("<option></option>").attr("value",i).text("Channel #" + i));
    }

    // add brightness template
    var template = $("#brightnessTemplate").children();
    var line = $(template).clone();
    line.appendTo("#channels");

    // init bright slider
    $("#brightness").on("change", function() {
        var value = $(this).val();
        var parent = $(this).parents(".pure-g");
        $("span", parent).html(value);
        sendAction("brightness", {value: value});
    });

}
<!-- endRemoveIf(!light)-->

// -----------------------------------------------------------------------------
// RFBridge
// -----------------------------------------------------------------------------

<!-- removeIf(!rfbridge)-->

function rfbLearn() {
    var parent = $(this).parents(".pure-g");
    var input = $("input", parent);
    sendAction("rfblearn", {id: input.attr("data-id"), status: input.attr("data-status")});
}

function rfbForget() {
    var parent = $(this).parents(".pure-g");
    var input = $("input", parent);
    sendAction("rfbforget", {id: input.attr("data-id"), status: input.attr("data-status")});
}

function rfbSend() {
    var parent = $(this).parents(".pure-g");
    var input = $("input", parent);
    sendAction("rfbsend", {id: input.attr("data-id"), status: input.attr("data-status"), data: input.val()});
}

function addRfbNode() {

    var numNodes = $("#rfbNodes > legend").length;

    var template = $("#rfbNodeTemplate").children();
    var line = $(template).clone();
    $("span", line).html(numNodes);
    $(line).find("input").each(function() {
        this.dataset["id"] = numNodes;
    });
    $(line).find(".button-rfb-learn").on("click", rfbLearn);
    $(line).find(".button-rfb-forget").on("click", rfbForget);
    $(line).find(".button-rfb-send").on("click", rfbSend);
    line.appendTo("#rfbNodes");

    return line;
}
<!-- endRemoveIf(!rfbridge)-->

// -----------------------------------------------------------------------------
// LightFox
// -----------------------------------------------------------------------------

<!-- removeIf(!lightfox)-->

function lightfoxLearn() {
    sendAction("lightfoxLearn", {});
}

function lightfoxClear() {
    sendAction("lightfoxClear", {});
}

function initLightfox(data, relayCount) {

    var numNodes = data.length;

    var template = $("#lightfoxNodeTemplate").children();

    var i, j;
    for (i=0; i<numNodes; i++) {
        var $line = $(template).clone();
        $line.find("label > span").text(data[i]["id"]);
        $line.find("select").each(function() {
            $(this).attr("name", "btnRelay" + data[i]["id"]);
            for (j=0; j < relayCount; j++) {
                $(this).append($("<option >").attr("value", j).text("Switch #" + j));
            }
            $(this).val(data[i]["relay"]);
            status = !status;
        });
        setOriginalsFromValues($("input,select", $line));
        $line.appendTo("#lightfoxNodes");
    }

    var $panel = $("#panel-lightfox")
    $(".button-lightfox-learn").off("click").click(lightfoxLearn);
    $(".button-lightfox-clear").off("click").click(lightfoxClear);

}
<!-- endRemoveIf(!lightfox)-->

// -----------------------------------------------------------------------------
// Processing
// -----------------------------------------------------------------------------

function processData(data) {

    if (debug) console.log(data);

    // title
    if ("app_name" in data) {
        var title = data.app_name;
		if ("app_version" in data) {
			title = title + " " + data.app_version;
		}
        $("span[name=title]").html(title);
        if ("hostname" in data) {
            title = data.hostname + " - " + title;
        }
        document.title = title;
    }

    Object.keys(data).forEach(function(key) {

        var i;
        var value = data[key];

        // ---------------------------------------------------------------------
        // Web mode
        // ---------------------------------------------------------------------

        if ("webMode" === key) {
            password = (1 === value);
            $("#layout").toggle(!password);
            $("#password").toggle(password);
        }

        // ---------------------------------------------------------------------
        // Actions
        // ---------------------------------------------------------------------

        if ("action" === key) {
            if ("reload" === data.action) { doReload(1000); }
            return;
        }

        // ---------------------------------------------------------------------
        // RFBridge
        // ---------------------------------------------------------------------

        <!-- removeIf(!rfbridge)-->

        if ("rfbCount" === key) {
            for (i=0; i<data.rfbCount; i++) { addRfbNode(); }
            return;
        }

        if ("rfb" === key) {
            var rfb = data.rfb;

            var size = rfb.size;
            var start = rfb.start;

            var processOn = ((rfb.on !== undefined) && (rfb.on.length > 0));
            var processOff = ((rfb.off !== undefined) && (rfb.off.length > 0));

            for (var i=0; i<size; ++i) {
                if (processOn) $("input[name='rfbcode'][data-id='" + (i + start) + "'][data-status='1']").val(rfb.on[i]);
                if (processOff) $("input[name='rfbcode'][data-id='" + (i + start) + "'][data-status='0']").val(rfb.off[i]);
            }

            return;
        }

        <!-- endRemoveIf(!rfbridge)-->

        // ---------------------------------------------------------------------
        // LightFox
        // ---------------------------------------------------------------------

        <!-- removeIf(!lightfox)-->

        if ("lightfoxButtons" === key) {
            initLightfox(data["lightfoxButtons"], data["lightfoxRelayCount"]);
            return;
        }

        <!-- endRemoveIf(!lightfox)-->

        // ---------------------------------------------------------------------
        // RFM69
        // ---------------------------------------------------------------------

        <!-- removeIf(!rfm69)-->

        if (key == "packet") {
            var packet = data.packet;
            var d = new Date();
            packets.row.add([
                d.toLocaleTimeString('en-US', { hour12: false }),
                packet.senderID,
                packet.packetID,
                packet.targetID,
                packet.key,
                packet.value,
                packet.rssi,
                packet.duplicates,
                packet.missing,
            ]).draw(false);
            return;
        }

        if (key == "mapping") {
            for (var i in data.mapping) {

                // add a new row
                addMapping();

                // get group
                var line = $("#mapping .pure-g")[i];

                // fill in the blanks
                var mapping = data.mapping[i];
                Object.keys(mapping).forEach(function(key) {
                    var id = "input[name=" + key + "]";
                    if ($(id, line).length) $(id, line).val(mapping[key]);
                });

                setOriginalsFromValues($("input", line));
            }
            return;
        }

        <!-- endRemoveIf(!rfm69)-->

        // ---------------------------------------------------------------------
        // RPN Rules
        // ---------------------------------------------------------------------

        if (key == "rpnRules") {
			for (var i in data.rpnRules) {

				// add a new row
				addRPNRule();

				// get group
				var line = $("#rpnRules .pure-g")[i];

				// fill in the blanks
				var rule = data.rpnRules[i];
                $("input", line).val(rule);

                setOriginalsFromValues($("input", line));

            }
			return;
		}

        if (key == "rpnTopics") {
			for (var i in data.rpnTopics) {

				// add a new row
				addRPNTopic();

				// get group
				var line = $("#rpnTopics .pure-g")[i];

				// fill in the blanks
				var topic = data.rpnTopics[i];
				var name = data.rpnNames[i];
                $("input[name='rpnTopic']", line).val(topic);
                $("input[name='rpnName']", line).val(name);

                setOriginalsFromValues($("input", line));

            }
			return;
        }
        
        if (key == "rpnNames") return;

        // ---------------------------------------------------------------------
        // Lights
        // ---------------------------------------------------------------------

        <!-- removeIf(!light)-->

        if ("rgb" === key) {
            initColor({rgb: true});
            setPickerRGB($("input[name='color']"), value);
            return;
        }

        if ("hsv" === key) {
            initColor({hsv: true});
            setPickerHSV($("input[name='color']"), value);
            return;
        }

        if ("brightness" === key) {
            $("#brightness").val(value);
            $("span.brightness").html(value);
            return;
        }

        if ("channels" === key) {
            var len = value.length;
            initChannels(len);
            for (i in value) {
                var ch = value[i];
                $("input.slider[data=" + i + "]").val(ch);
                $("span.slider[data=" + i + "]").html(ch);
            }
            return;
        }

        if ("mireds" === key) {
            $("#mireds").attr("min", value["cold"]);
            $("#mireds").attr("max", value["warm"]);
            $("#mireds").val(value["value"]);
            $("span.mireds").html(value["value"]);
            return;
        }

        if ("useWhite" === key) {
            useWhite = value;
        }

        if ("useCCT" === key) {
            initCCT();
            useCCT = value;
        }

        <!-- endRemoveIf(!light)-->

        // ---------------------------------------------------------------------
        // Sensors & Magnitudes
        // ---------------------------------------------------------------------

        <!-- removeIf(!sensor)-->

        if ("magnitudesConfig" === key) {
            initMagnitudes(value);
        }

        if ("magnitudes" === key) {
            for (var i=0; i<value.size; ++i) {
                var inputElem = $("input[name='magnitude'][data='" + i + "']");
                var infoElem = inputElem.parent().parent().find("div.sns-info");

                var error = value.error[i] || 0;
                var text = (0 === error)
                    ? value.value[i] + magnitudes[i].units
                    : magnitudeError(error);
                inputElem.val(text);

                if (value.info !== undefined) {
                    var info = value.info[i] || 0;
                    infoElem.toggle(info != 0);
                    infoElem.text(info);
                }
            }
            return;
        }

        <!-- endRemoveIf(!sensor)-->

        // ---------------------------------------------------------------------
        // WiFi
        // ---------------------------------------------------------------------

        if ("wifi" === key) {
            maxNetworks = parseInt(value["max"], 10);
            value["networks"].forEach(function(network) {
                var schema = value["schema"];
                if (schema.length !== network.length) {
                    throw "WiFi schema mismatch!";
                }

                var _network = {};
                schema.forEach(function(key, index) {
                    _network[key] = network[index];
                });

                addNetwork(_network);
            });
            return;
        }

        if ("scanResult" === key) {
            $("div.scan.loading").hide();
            $("#scanResult").show();
        }

        // -----------------------------------------------------------------------------
        // Home Assistant
        // -----------------------------------------------------------------------------

        if ("haConfig" === key) {
            send("{}");
            $("#haConfig")
                .append(new Text(value))
                .height($("#haConfig")[0].scrollHeight);
            return;
        }

        // -----------------------------------------------------------------------------
        // Relays scheduler
        // -----------------------------------------------------------------------------

        if ("schedules" === key) {
            $("#schedules").attr("data-settings-max", value.max);
            for (var i=0; i<value.size; ++i) {
                // XXX: no
                var sch_map = {};
                Object.keys(value).forEach(function(key) {
                    if ("size" == key) return;
                    if ("max" == key) return;
                    sch_map[key] = value[key][i];
                });
                addSchedule(sch_map);
            }
            return;
        }

        // ---------------------------------------------------------------------
        // Relays
        // ---------------------------------------------------------------------

        if ("relayState" === key) {
            initRelays(value.status);
            updateRelays(value);
            return;
        }

        // Relay configuration
        if ("relayConfig" === key) {
            initRelayConfig(value);
            return;
        }

        // ---------------------------------------------------------------------
        // LEDs
        // ---------------------------------------------------------------------

        if ("led" === key) {
            if($("#ledConfig > div").length > 0) return;

            var schema = value["schema"];
            value["list"].forEach(function(led_data, index) {
                if (schema.length !== led_data.length) {
                    throw "LED schema mismatch!";
                }

                var led = {};
                schema.forEach(function(key, index) {
                    led[key] = led_data[index];
                });

                var line = $($("#ledConfigTemplate").children()).clone();

                $("span.id", line).html(index);
                $("select", line).attr("data", index);
                $("input", line).attr("data", index);

                $("select[name='ledGPIO']", line).val(led.GPIO);
                // XXX: checkbox implementation depends on unique id
                // $("input[name='ledInv']", line).val(led.Inv);
                $("select[name='ledMode']", line).val(led.Mode);
                $("input[name='ledRelay']", line).val(led.Relay);

                setOriginalsFromValues($("input,select", line));

                line.appendTo("#ledConfig");
            });

            return;
        }

        // ---------------------------------------------------------------------
        // Domoticz
        // ---------------------------------------------------------------------

        // Domoticz - Relays
        if ("dczRelays" === key) {
            createRelayList(value, "dczRelays", "dczRelayTemplate");
            return;
        }

        // Domoticz - Magnitudes
        <!-- removeIf(!sensor)-->
        if ("dczMagnitudes" === key) {
            createMagnitudeList(value, "dczMagnitudes", "dczMagnitudeTemplate");
            return;
        }
        <!-- endRemoveIf(!sensor)-->

        // ---------------------------------------------------------------------
        // Thingspeak
        // ---------------------------------------------------------------------

        // Thingspeak - Relays
        if ("tspkRelays" === key) {
            createRelayList(value, "tspkRelays", "tspkRelayTemplate");
            return;
        }

        // Thingspeak - Magnitudes
        <!-- removeIf(!sensor)-->
        if ("tspkMagnitudes" === key) {
            createMagnitudeList(value, "tspkMagnitudes", "tspkMagnitudeTemplate");
            return;
        }
        <!-- endRemoveIf(!sensor)-->

        // ---------------------------------------------------------------------
        // HTTP API
        // ---------------------------------------------------------------------

        // Auto generate an APIKey if none defined yet
        if ("apiVisible" === key) {
            if (data.apiKey === undefined || data.apiKey === "") {
                generateAPIKey();
            }
        }

        // ---------------------------------------------------------------------
        // General
        // ---------------------------------------------------------------------

        // Messages
        if ("message" === key) {
            if (value == 8) {
                configurationSaved = true;
            }
            window.alert(messages[value]);
            return;
        }

        // Web log
        if ("weblog" === key) {
            send("{}");

            var msg = value["msg"];
            var pre = value["pre"];

            for (var i=0; i < msg.length; ++i) {
                if (pre[i]) {
                    $("#weblog").append(new Text(pre[i]));
                }
                $("#weblog").append(new Text(msg[i]));
            }

            $("#weblog").scrollTop($("#weblog")[0].scrollHeight - $("#weblog").height());
            return;
        }

        // Enable options
        var position = key.indexOf("Visible");
        if (position > 0 && position === key.length - 7) {
            var module = key.slice(0,-7);
            if (module == "sch") {
                $("li.module-" + module).css("display", "inherit");
                $("div.module-" + module).css("display", "flex");
                return;
            }
            $(".module-" + module).css("display", "inherit");
            return;
        }

        if ("deviceip" === key) {
            var a_href = $("span[name='" + key + "']").parent();
            a_href.attr("href", "//" + value);
            a_href.next().attr("href", "telnet://" + value);
        }

        if ("now" === key) {
            now = value;
            return;
        }

        if ("free_size" === key) {
            free_size = parseInt(value, 10);
        }

        // Pre-process
        if ("mqttStatus" === key) {
            value = value ? "CONNECTED" : "NOT CONNECTED";
        }
        if ("ntpStatus" === key) {
            value = value ? "SYNC'D" : "NOT SYNC'D";
        }
        if ("uptime" === key) {
            ago = 0;
            var uptime  = parseInt(value, 10);
            var seconds = uptime % 60; uptime = parseInt(uptime / 60, 10);
            var minutes = uptime % 60; uptime = parseInt(uptime / 60, 10);
            var hours   = uptime % 24; uptime = parseInt(uptime / 24, 10);
            var days    = uptime;
            value = days + "d " + zeroPad(hours, 2) + "h " + zeroPad(minutes, 2) + "m " + zeroPad(seconds, 2) + "s";
        }
        <!-- removeIf(!thermostat)-->
        if ("tmpUnits" == key) {
            $("span.tmpUnit").html(data[key] == 3 ? "ºF" : "ºC");
        }
        <!-- endRemoveIf(!thermostat)-->

        // ---------------------------------------------------------------------
        // Matching
        // ---------------------------------------------------------------------
        var elems = [];

        var pre;
        var post;

        // Look for INPUTs
        var input = $("input[name='" + key + "']");
        if (input.length > 0) {
            if (input.attr("type") === "checkbox") {
                input.prop("checked", value);
            } else if (input.attr("type") === "radio") {
                input.val([value]);
            } else {
                pre = input.attr("pre") || "";
                post = input.attr("post") || "";
                input.val(pre + value + post);
            }
            elems.push(input);
        }

        // Look for SPANs
        var span = $("span[name='" + key + "']");
        if (span.length > 0) {
            if (Array.isArray(value)) {
                value.forEach(function(elem) {
                    span.append(elem);
                    span.append('</br>');
                    elems.push(span);
                });
            } else {
                pre = span.attr("pre") || "";
                post = span.attr("post") || "";
                span.html(pre + value + post);
                elems.push(span);
            }
        }

        // Look for SELECTs
        var select = $("select[name='" + key + "']");
        if (select.length > 0) {
            select.val(value);
            elems.push(select);
        }

        setOriginalsFromValues($(elems));

    });

}

function hasChanged() {

    var newValue, originalValue;
    if ($(this).attr("type") === "checkbox") {
        newValue = $(this).prop("checked");
        originalValue = ($(this).attr("original") === "true");
    } else {
        newValue = $(this).val();
        originalValue = $(this).attr("original");
    }
    var hasChanged = ("true" === $(this).attr("hasChanged"));
    var action = $(this).attr("action");

    if (typeof originalValue === "undefined") { return; }
    if ("none" === action) { return; }

    if (newValue !== originalValue) {
        if (!hasChanged) {
            ++numChanged;
            if ("reconnect" === action) { ++numReconnect; }
            if ("reboot" === action) { ++numReboot; }
            if ("reload" === action) { ++numReload; }
        }
        $(this).attr("hasChanged", true);
    } else {
        if (hasChanged) {
            --numChanged;
            if ("reconnect" === action) { --numReconnect; }
            if ("reboot" === action) { --numReboot; }
            if ("reload" === action) { --numReload; }
        }
        $(this).attr("hasChanged", false);
    }

}

// -----------------------------------------------------------------------------
// Init & connect
// -----------------------------------------------------------------------------

function initUrls(root) {

    var paths = ["ws", "upgrade", "config", "auth"];

    urls["root"] = root;
    paths.forEach(function(path) {
        urls[path] = new URL(path, root);
        urls[path].protocol = root.protocol;
    });

    if (root.protocol == "https:") {
        urls.ws.protocol = "wss:";
    } else {
        urls.ws.protocol = "ws:";
    }

}

function connectToURL(url) {

    initUrls(url);

    fetch(urls.auth.href, {
        'method': 'GET',
        'cors': true,
        'credentials': 'same-origin'
    }).then(function(response) {
        // Nothing to do, reload page and retry
        if (response.status != 200) {
            doReload(5000);
            return;
        }
        // update websock object
        if (websock) { websock.close(); }
        websock = new WebSocket(urls.ws.href);
        websock.onmessage = function(evt) {
            var data = getJson(evt.data.replace(/\n/g, "\\n").replace(/\r/g, "\\r").replace(/\t/g, "\\t"));
            if (data) {
                processData(data);
            }
        };
        websock.onclose = function(evt) {
            clearInterval(ws_pingpong);
            if (window.confirm("Connection lost with the device, click OK to refresh the page")) {
                $("#layout").toggle(false);
                window.location.reload();
            }
        }
        websock.onopen = function(evt) {
            ws_pingpong = setInterval(function() { sendAction("ping", {}); }, 5000);
        }
    }).catch(function(error) {
        console.log(error);
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

$(function() {

    initMessages();
    loadTimeZones();
    createCheckboxes();
    setInterval(function() { keepTime(); }, 1000);

    $(".password-reveal").on("click", toggleVisiblePassword);

    $("#menuLink").on("click", toggleMenu);
    $(".pure-menu-link").on("click", showPanel);
    $("progress").attr({ value: 0, max: 100 });

    $(".button-update").on("click", doUpdate);
    $(".button-update-password").on("click", doUpdatePassword);
    $(".button-generate-password").on("click", doGeneratePassword);
    $(".button-reboot").on("click", doReboot);
    $(".button-reconnect").on("click", doReconnect);
    $(".button-wifi-scan").on("click", doScan);
    $(".button-ha-config").on("click", doHAConfig);
    $(".button-dbgcmd").on("click", doDebugCommand);
    $("input[name='dbgcmd']").enterKey(doDebugCommand);
    $(".button-dbg-clear").on("click", doDebugClear);
    $(".button-settings-backup").on("click", doBackup);
    $(".button-settings-restore").on("click", doRestore);
    $(".button-settings-factory").on("click", doFactoryReset);
    $("#uploader").on("change", onFileUpload);
    $(".button-upgrade").on("click", doUpgrade);

    <!-- removeIf(!thermostat)-->
    $(".button-thermostat-reset-counters").on('click', doResetThermostatCounters);
    <!-- endRemoveIf(!thermostat)-->

    $(".button-apikey").on("click", generateAPIKey);
    $(".button-upgrade-browse").on("click", function() {
        $("input[name='upgrade']")[0].click();
        return false;
    });
    $("input[name='upgrade']").change(function (){
        var file = this.files[0];
        $("input[name='filename']").val(file.name);
    });
    $(".button-add-network").on("click", function() {
        $(".more", addNetwork()).toggle();
    });

    $(".button-add-switch-schedule").on("click", function() {
        addSchedule({schType: 1, schSwitch: -1});
    });
    <!-- removeIf(!light)-->
    $(".button-add-light-schedule").on("click", function() {
        addSchedule({schType: 2, schSwitch: -1});
    });
    <!-- endRemoveIf(!light)-->

    $(".button-add-rpnrule").on('click', addRPNRule);
    $(".button-add-rpntopic").on('click', addRPNTopic);

    $(".button-del-parent").on('click', delParent);

    <!-- removeIf(!rfm69)-->
    $(".button-add-mapping").on('click', addMapping);
    $(".button-clear-counts").on('click', doClearCounts);
    $(".button-clear-messages").on('click', doClearMessages);
    $(".button-clear-filters").on('click', doClearFilters);
    $('#packets tbody').on('mousedown', 'td', doFilter);
    packets = $('#packets').DataTable({
        "paging": false
    });
    for (var i = 0; i < packets.columns()[0].length; i++) {
        filters[i] = false;
    }
    <!-- endRemoveIf(!rfm69)-->

    $(".gpio-select").each(function(_, elem) {
        initSelectGPIO(elem)
    });

    $(document).on("change", "input", hasChanged);
    $(document).on("change", "select", hasChanged);

    $("textarea").on("dblclick", function() { this.select(); });

    resetOriginals();

    $(".group-settings").each(function() {
        groupSettingsObserver.observe(this, {childList: true});
    });

    // don't autoconnect when opening from filesystem
    if (window.location.protocol === "file:") {
        processData({"webMode": 0});
        processData({"hlwVisible":1,"pwrVisible":1,"tmpCorrection":0,"humCorrection":0,"luxCorrection":0,"snsRead":5,"snsReport":10,"snsSave":2,"magnitudesConfig":{"index":[0,0,0,0,0,0,0,0],"type":[4,5,6,8,7,9,11,10],"units":["A","V","W","VAR","VA","%","J","kWh"],"description":["HLW8012 @ GPIO(5,14,13)","HLW8012 @ GPIO(5,14,13)","HLW8012 @ GPIO(5,14,13)","HLW8012 @ GPIO(5,14,13)","HLW8012 @ GPIO(5,14,13)","HLW8012 @ GPIO(5,14,13)","HLW8012 @ GPIO(5,14,13)","HLW8012 @ GPIO(5,14,13)"],"size":8}});
        processData({"magnitudes":{"value":["0.079","218","37","0","17","100","96","0.001"],"error":[0,0,0,0,0,0,0,0],"info":[0,0,0,0,0,0,0,"Last saved: 2020-04-07 21:10:15"],"size":8}});
        return;
    }

    // Check host param in query string
    var search = new URLSearchParams(window.location.search),
        host = search.get("host");

    if (host !== null) {
        connect(host);
    } else {
        connectToCurrentURL();
    }

});
