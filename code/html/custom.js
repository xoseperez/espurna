var websock;
var password = false;
var maxNetworks;
var maxSchedules;
var messages = [];
var free_size = 0;

var urls = {};

var numChanged = 0;
var numReboot = 0;
var numReconnect = 0;
var numReload = 0;

var useWhite = false;
var useCCT = false;

var now = 0;
var ago = 0;

<!-- removeIf(!rfm69)-->
var packets;
var filters = [];
<!-- endRemoveIf(!rfm69)-->

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
        "TMP3X", "HC-SR04", "SenseAir", "GeigerTicks", "GeigerCPM"
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
        "PM1.0", "PM2.5", "PM10", "CO2", "Lux", "UV", "Distance" , "HCHO",
        "Local Dose Rate", "Local Dose Rate",
        "Count"
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
    var zeros = "";
    for (var i = 0; i < positions; i++) {
        zeros += "0";
    }
    return (zeros + number).slice(-positions);
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
        var value = time_zones[i];
        var offset = value >= 0 ? value : -value;
        var text = "GMT" + (value >= 0 ? "+" : "-") +
            zeroPad(parseInt(offset / 60, 10), 2) + ":" +
            zeroPad(offset % 60, 2);
        $("select[name='ntpOffset']").append(
            $("<option></option>").
                attr("value",value).
                text(text));
    }

}

function validateForm(form) {

    // http://www.the-art-of-web.com/javascript/validate-password/
    // at least one lowercase and one uppercase letter or number
    // at least five characters (letters, numbers or special characters)
    var re_password = /^(?=.*[A-Z\d])(?=.*[a-z])[\w~!@#$%^&*\(\)<>,.\?;:{}\[\]\\|]{5,}$/;

    // password
    var adminPass1 = $("input[name='adminPass']", form).first().val();
    if (adminPass1.length > 0 && !re_password.test(adminPass1)) {
        alert("The password you have entered is not valid, it must have at least 5 characters, 1 lowercase and 1 uppercase or number!");
        return false;
    }

    var adminPass2 = $("input[name='adminPass']", form).last().val();
    if (adminPass1 !== adminPass2) {
        alert("Passwords are different!");
        return false;
    }

    // RFCs mandate that a hostname's labels may contain only
    // the ASCII letters 'a' through 'z' (case-insensitive),
    // the digits '0' through '9', and the hyphen.
    // Hostname labels cannot begin or end with a hyphen.
    // No other symbols, punctuation characters, or blank spaces are permitted.

    // Negative lookbehind does not work in Javascript
    // var re_hostname = new RegExp('^(?!-)[A-Za-z0-9-]{1,32}(?<!-)$');

    var re_hostname = new RegExp('^(?!-)[A-Za-z0-9-]{0,31}[A-Za-z0-9]$');

    var hostname = $("input[name='hostname']", form);
    var hasChanged = hostname.attr("hasChanged") || 0;
    if (0 === hasChanged) {
        return true;
    }

    if (!re_hostname.test(hostname.val())) {
        alert("Hostname cannot be empty and may only contain the ASCII letters ('A' through 'Z' and 'a' through 'z'), the digits '0' through '9', and the hyphen ('-')! They can neither start or end with an hyphen.");
        return false;
    }

    return true;

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

    // These fields will always be a list of values
    var is_group = [
        "ssid", "pass", "gw", "mask", "ip", "dns",
        "schEnabled", "schSwitch","schAction","schType","schHour","schMinute","schWDs","schUTC",
        "relayBoot", "relayPulse", "relayTime",
        "btnActDblCl", "btnActLngCl", "btnActLngLngCl", "btnActPres", "btnActRelCl", "btnActTplCl", 
        "btnDefaultHigh", "btnMode", "btnPullup", "btnRelay",
        "mqttGroup", "mqttGroupInv", "relayOnDisc",
        "dczRelayIdx", "dczMagnitude",
        "tspkRelay", "tspkMagnitude",
        "ledMode",
        "adminPass",
        "node", "key", "topic"
    ];

    if (name in data) {
        if (!Array.isArray(data[name])) {
            data[name] = [data[name]];
        }
        data[name].push(value);
    } else if (is_group.indexOf(name) >= 0) {
        data[name] = [value];
    } else {
        data[name] = value;
    }

}

function getData(form) {

    var data = {};

    // Populate data
    $("input,select", form).each(function() {
        var name = $(this).attr("name");
        var value = getValue(this);
        if (null !== value) {
            addValue(data, name, value);
        }
    });

    // Post process
    addValue(data, "schSwitch", 0xFF);
    delete data["filename"];
    delete data["rfbcode"];

    return data;

}

function randomString(length, chars) {
    var mask = "";
    if (chars.indexOf("a") > -1) { mask += "abcdefghijklmnopqrstuvwxyz"; }
    if (chars.indexOf("A") > -1) { mask += "ABCDEFGHIJKLMNOPQRSTUVWXYZ"; }
    if (chars.indexOf("#") > -1) { mask += "0123456789"; }
    if (chars.indexOf("@") > -1) { mask += "ABCDEF"; }
    if (chars.indexOf("!") > -1) { mask += "~`!@#$%^&*()_+-={}[]:\";'<>?,./|\\"; }
    var result = "";
    for (var i = length; i > 0; --i) {
        result += mask[Math.round(Math.random() * (mask.length - 1))];
    }
    return result;
}

function generateAPIKey() {
    var apikey = randomString(16, "@#");
    $("input[name='apiKey']").val(apikey);
    return false;
}

function getJson(str) {
    try {
        return JSON.parse(str);
    } catch (e) {
        return false;
    }
}

// -----------------------------------------------------------------------------
// Actions
// -----------------------------------------------------------------------------

function sendAction(action, data) {
    websock.send(JSON.stringify({action: action, data: data}));
}

function sendConfig(data) {
    websock.send(JSON.stringify({config: data}));
}

function resetOriginals() {
    $("input,select").each(function() {
        $(this).attr("original", $(this).val());
    });
    numReboot = numReconnect = numReload = 0;
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
            if (0xE9 !== evt.target.result.charCodeAt(0)) callback(false);
            if (0x03 !== evt.target.result.charCodeAt(2)) {
                var response = window.confirm("Binary image is not using DOUT flash mode. This might cause resets in some devices. Press OK to continue.");
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
            alert("The file does not seem to be a valid firmware image.");
            return;
        }

        var data = new FormData();
        data.append("upgrade", file, file.name);

        $.ajax({

            // Your server script to process the upload
            url: urls.upgrade.href,
            type: "POST",

            // Form data
            data: data,

            // Tell jQuery not to process data or worry about content-type
            // You *must* include these options!
            cache: false,
            contentType: false,
            processData: false,

            success: function(data, text) {
                $("#upgrade-progress").hide();
                if ("OK" === data) {
                    alert("Firmware image uploaded, board rebooting. This page will be refreshed in 5 seconds.");
                    doReload(5000);
                } else {
                    alert("There was an error trying to upload the new image, please try again (" + data + ").");
                }
            },

            // Custom XMLHttpRequest
            xhr: function() {
                $("#upgrade-progress").show();
                var myXhr = $.ajaxSettings.xhr();
                if (myXhr.upload) {
                    // For handling the progress of the upload
                    myXhr.upload.addEventListener("progress", function(e) {
                        if (e.lengthComputable) {
                            $("progress").attr({ value: e.loaded, max: e.total });
                        }
                    } , false);
                }
                return myXhr;
            }

        });

    });

    return false;

}

function doUpdatePassword() {
    var form = $("#formPassword");
    if (validateForm(form)) {
        sendConfig(getData(form));
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

function doUpdate() {

    var form = $("#formSave");
    if (validateForm(form)) {

        // Get data
        sendConfig(getData(form));

        // Empty special fields
        $(".pwrExpected").val(0);
        $("input[name='pwrResetCalibration']").prop("checked", false);
        $("input[name='pwrResetE']").prop("checked", false);

        // Change handling
        numChanged = 0;
        setTimeout(function() {

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

        }, 1000);

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
    if (response === false) {
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
    $("#haConfig").html("");
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
        line.appendTo("#" + container);
    }

}

<!-- removeIf(!sensor)-->
function createMagnitudeList(data, container, template_name) {

    var current = $("#" + container + " > div").length;
    if (current > 0) { return; }

    var template = $("#" + template_name + " .pure-g")[0];
    for (var i in data) {
        var magnitude = data[i];
        var line = $(template).clone();
        $("label", line).html(magnitudeType(magnitude.type) + " #" + parseInt(magnitude.index, 10));
        $("div.hint", line).html(magnitude.name);
        $("input", line).attr("tabindex", 40 + i).val(magnitude.idx);
        line.appendTo("#" + container);
    }

}
<!-- endRemoveIf(!sensor)-->

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
    $(line).find("button").on('click', delMapping);
    line.appendTo("#mapping");
}

function delMapping() {
    var parent = $(this).parent().parent();
    $(parent).remove();
}
<!-- endRemoveIf(!rfm69)-->

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

function delNetwork() {
    var parent = $(this).parents(".pure-g");
    $(parent).remove();
}

function moreNetwork() {
    var parent = $(this).parents(".pure-g");
    $(".more", parent).toggle();
}

function addNetwork() {

    var numNetworks = $("#networks > div").length;
    if (numNetworks >= maxNetworks) {
        alert("Max number of networks reached");
        return null;
    }

    var tabindex = 200 + numNetworks * 10;
    var template = $("#networkTemplate").children();
    var line = $(template).clone();
    $(line).find("input").each(function() {
        $(this).attr("tabindex", tabindex);
        tabindex++;
    });
    $(line).find(".button-del-network").on("click", delNetwork);
    $(line).find(".button-more-network").on("click", moreNetwork);
    line.appendTo("#networks");

    return line;

}

// -----------------------------------------------------------------------------
// Relays scheduler
// -----------------------------------------------------------------------------

function delSchedule() {
    var parent = $(this).parents(".pure-g");
    $(parent).remove();
}

function moreSchedule() {
    var parent = $(this).parents(".pure-g");
    $("div.more", parent).toggle();
}

function addSchedule(event) {

    var numSchedules = $("#schedules > div").length;
    if (numSchedules >= maxSchedules) {
        alert("Max number of schedules reached");
        return null;
    }
    var tabindex = 200 + numSchedules * 10;
    var template = $("#scheduleTemplate").children();
    var line = $(template).clone();

    var type = (1 === event.data.schType) ? "switch" : "light";

    template = $("#" + type + "ActionTemplate").children();
    var actionLine = template.clone();
    $(line).find("#schActionDiv").append(actionLine);

    $(line).find("input").each(function() {
        $(this).attr("tabindex", tabindex);
        tabindex++;
    });
    $(line).find(".button-del-schedule").on("click", delSchedule);
    $(line).find(".button-more-schedule").on("click", moreSchedule);
    $(line).find("input[name='schUTC']").prop("id", "schUTC" + (numSchedules + 1))
        .next().prop("for", "schUTC" + (numSchedules + 1));
    $(line).find("input[name='schEnabled']").prop("id", "schEnabled" + (numSchedules + 1))
        .next().prop("for", "schEnabled" + (numSchedules + 1));
    line.appendTo("#schedules");
    $(line).find("input[type='checkbox']").prop("checked", false);

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

        // Populate the relay SELECTs
        $("select.isrelay").append(
            $("<option></option>").attr("value",i).text("Switch #" + i));

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

    var current = $("#relayConfig > div").length;
    if (current > 0) { return; }

    var template = $("#relayConfigTemplate").children();
    for (var i in data) {
        var relay = data[i];
        var line = $(template).clone();
        $("span.gpio", line).html(relay.gpio);
        $("span.id", line).html(i);
        $("select[name='relayBoot']", line).val(relay.boot);
        $("select[name='relayPulse']", line).val(relay.pulse);
        $("input[name='relayTime']", line).val(relay.pulse_ms);
        $("input[name='mqttGroup']", line).val(relay.group);
        $("select[name='mqttGroupInv']", line).val(relay.group_inv);
        $("select[name='relayOnDisc']", line).val(relay.on_disc);
        line.appendTo("#relayConfig");
    }

}

// -----------------------------------------------------------------------------
// Buttons
// -----------------------------------------------------------------------------

function initButtonConfig(data) {
    var num = 1;

    var current = $("#btnConfig > div").length;
    if (current > 0) { return; }
    
    //copy select options
    $($("select[name='btnActPres']").children()).clone().appendTo(".isBtnAction");

    var template = $("#btnConfigTemplate").children();
    for (var i in data) {
        var btn = data[i];
        var line = $(template).clone();

        $("span.gpio", line).html(btn.gpio);
        $("span.id", line).html(i);
        $("select[name='btnMode']", line).val(btn.mode);
        //Show Pushbutton actions only if Pushbutton is selected!
        $("select[name='btnMode']", line).on("change", function() {
            var parent = $(this).parents(".pure-g");
            $(".pbonly", parent).toggle($(this).val() == 0);
          });
        $(".pbonly", line).toggle(btn.mode == 0);

        $("input[type='checkbox'][name='btnDefaultHigh']", line).prop("checked", btn.defaultHigh);
        $("input[type='checkbox'][name='btnPullup']", line).prop("checked", btn.pullup);
        $("select[name='btnRelay']", line).val(btn.relay);
        $("select[name='btnActPres']", line).val(btn.actPres);
        $("select[name='btnActRelCl']", line).val(btn.actRelCl);
        $("select[name='btnActDblCl']", line).val(btn.actDblCl);
        $("select[name='btnActLngCl']", line).val(btn.actLngCl);
        $("select[name='btnActLngLngCl']", line).val(btn.actLngLngCl);
        $("select[name='btnActTplCl']", line).val(btn.actTplCl);

        $("input[name='btnDefaultHigh']", line).prop("id", "btnDefaultHigh" + num)
            .next().prop("for", "btnDefaultHigh" + num);
        $("input[name='btnPullup']", line).prop("id", "btnPullup" + num)
            .next().prop("for", "btnPullup" + num);
        num++;

        line.appendTo("#btnConfig");
    }    
}

// -----------------------------------------------------------------------------
// Sensors & Magnitudes
// -----------------------------------------------------------------------------

<!-- removeIf(!sensor)-->
function initMagnitudes(data) {

    // check if already initialized
    var done = $("#magnitudes > div").length;
    if (done > 0) { return; }

    // add templates
    var template = $("#magnitudeTemplate").children();
    for (var i in data) {
        var magnitude = data[i];
        var line = $(template).clone();
        $("label", line).html(magnitudeType(magnitude.type) + " #" + parseInt(magnitude.index, 10));
        $("div.hint", line).html(magnitude.description);
        $("input", line).attr("data", i);
        line.appendTo("#magnitudes");
    }

}
<!-- endRemoveIf(!sensor)-->

// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

<!-- removeIf(!light)-->

function initColorRGB() {

    // check if already initialized
    var done = $("#colors > div").length;
    if (done > 0) { return; }

    // add template
    var template = $("#colorRGBTemplate").children();
    var line = $(template).clone();
    line.appendTo("#colors");

    // init color wheel
    $("input[name='color']").wheelColorPicker({
        sliders: "wrgbp"
    }).on("sliderup", function() {
        var value = $(this).wheelColorPicker("getValue", "css");
        sendAction("color", {rgb: value});
    });

    // init bright slider
    $("#brightness").on("change", function() {
        var value = $(this).val();
        var parent = $(this).parents(".pure-g");
        $("span", parent).html(value);
        sendAction("color", {brightness: value});
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

function initColorHSV() {

    // check if already initialized
    var done = $("#colors > div").length;
    if (done > 0) { return; }

    // add template
    var template = $("#colorHSVTemplate").children();
    var line = $(template).clone();
    line.appendTo("#colors");

    // init color wheel
    $("input[name='color']").wheelColorPicker({
        sliders: "whsvp"
    }).on("sliderup", function() {
        var color = $(this).wheelColorPicker("getColor");
        var value = parseInt(color.h * 360, 10) + "," + parseInt(color.s * 100, 10) + "," + parseInt(color.v * 100, 10);
        sendAction("color", {hsv: value});
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

    // add templates
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

    for (i=0; i<num; i++) {
        $("select.islight").append(
            $("<option></option>").attr("value",i).text("Channel #" + i));
    }

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
    var status = true;
    $("span", line).html(numNodes);
    $(line).find("input").each(function() {
        $(this).attr("data-id", numNodes);
        $(this).attr("data-status", status ? 1 : 0);
        status = !status;
    });
    $(line).find(".button-rfb-learn").on("click", rfbLearn);
    $(line).find(".button-rfb-forget").on("click", rfbForget);
    $(line).find(".button-rfb-send").on("click", rfbSend);
    line.appendTo("#rfbNodes");

    return line;
}
<!-- endRemoveIf(!rfbridge)-->

// -----------------------------------------------------------------------------
// Processing
// -----------------------------------------------------------------------------

function processData(data) {

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

        if ("rfbrawVisible" === key) {
            $("input[name='rfbcode']").attr("maxlength", 116);
        }

        if ("rfb" === key) {
            var nodes = data.rfb;
            for (i in nodes) {
                var node = nodes[i];
                $("input[name='rfbcode'][data-id='" + node.id + "'][data-status='" + node.status + "']").val(node.data);
            }
            return;
        }
        <!-- endRemoveIf(!rfbridge)-->

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
				    if ($(id, line).length) $(id, line).val(mapping[key]).attr("original", mapping[key]);
				});
			}
			return;
		}

        <!-- endRemoveIf(!rfm69)-->

        // ---------------------------------------------------------------------
        // Lights
        // ---------------------------------------------------------------------

        <!-- removeIf(!light)-->

        if ("rgb" === key) {
            initColorRGB();
            $("input[name='color']").wheelColorPicker("setValue", value, true);
            return;
        }

        if ("hsv" === key) {
            initColorHSV();
            // wheelColorPicker expects HSV to be between 0 and 1 all of them
            var chunks = value.split(",");
            var obj = {};
            obj.h = chunks[0] / 360;
            obj.s = chunks[1] / 100;
            obj.v = chunks[2] / 100;
            $("input[name='color']").wheelColorPicker("setColor", obj);
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
            $("#mireds").val(value);
            $("span.mireds").html(value);
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

        if ("magnitudes" === key) {
            initMagnitudes(value);
            for (i in value) {
                var magnitude = value[i];
                var error = magnitude.error || 0;
                var text = (0 === error) ?
                    magnitude.value + magnitude.units :
                    magnitudeError(error);
                var element = $("input[name='magnitude'][data='" + i + "']");
                element.val(text);
                $("div.hint", element.parent().parent()).html(magnitude.description);
            }
            return;
        }

        <!-- endRemoveIf(!sensor)-->

        // ---------------------------------------------------------------------
        // WiFi
        // ---------------------------------------------------------------------

        if ("maxNetworks" === key) {
            maxNetworks = parseInt(value, 10);
            return;
        }

        if ("wifi" === key) {
            for (i in value) {
                var wifi = value[i];
                var nwk_line = addNetwork();
                Object.keys(wifi).forEach(function(key) {
                    $("input[name='" + key + "']", nwk_line).val(wifi[key]);
                });
            }
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
            $("#haConfig").show();
        }

        // -----------------------------------------------------------------------------
        // Relays scheduler
        // -----------------------------------------------------------------------------

        if ("maxSchedules" === key) {
            maxSchedules = parseInt(value, 10);
            return;
        }

        if ("schedule" === key) {
            for (i in value) {
                var schedule = value[i];
                var sch_line = addSchedule({ data: {schType: schedule["schType"] }});

                Object.keys(schedule).forEach(function(key) {
                    var sch_value = schedule[key];
                    $("input[name='" + key + "']", sch_line).val(sch_value);
                    $("select[name='" + key + "']", sch_line).prop("value", sch_value);
                    $("input[type='checkbox'][name='" + key + "']", sch_line).prop("checked", sch_value);
                });
            }
            return;
        }

        // ---------------------------------------------------------------------
        // Relays
        // ---------------------------------------------------------------------

        if ("relayStatus" === key) {
            initRelays(value);
            for (i in value) {
                $("input[name='relay'][data='" + i + "']").prop("checked", value[i]);
            }
            return;
        }

        // Relay configuration
        if ("relayConfig" === key) {
            initRelayConfig(value);
            return;
        }

        // ---------------------------------------------------------------------
        // Buttons
        // ---------------------------------------------------------------------

        if ("btnConfig" === key) {
            initButtonConfig(value);
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
        // General
        // ---------------------------------------------------------------------

        // Messages
        if ("message" === key) {
            window.alert(messages[value]);
            return;
        }

        // Web log
        if ("weblog" === key) {
            $("#weblog").append(new Text(value));
            $("#weblog").scrollTop($("#weblog")[0].scrollHeight - $("#weblog").height());
            return;
        }

        // Enable options
        var position = key.indexOf("Visible");
        if (position > 0 && position === key.length - 7) {
            var module = key.slice(0,-7);
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

        // ---------------------------------------------------------------------
        // Matching
        // ---------------------------------------------------------------------

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
        }

        // Look for SPANs
        var span = $("span[name='" + key + "']");
        if (span.length > 0) {
            pre = span.attr("pre") || "";
            post = span.attr("post") || "";
            span.html(pre + value + post);
        }

        // Look for SELECTs
        var select = $("select[name='" + key + "']");
        if (select.length > 0) {
            select.val(value);
        }

    });

    // Auto generate an APIKey if none defined yet
    if ($("input[name='apiKey']").val() === "") {
        generateAPIKey();
    }

    resetOriginals();

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
    var hasChanged = $(this).attr("hasChanged") || 0;
    var action = $(this).attr("action");

    if (typeof originalValue === "undefined") { return; }
    if ("none" === action) { return; }

    if (newValue !== originalValue) {
        if (0 === hasChanged) {
            ++numChanged;
            if ("reconnect" === action) { ++numReconnect; }
            if ("reboot" === action) { ++numReboot; }
            if ("reload" === action) { ++numReload; }
            $(this).attr("hasChanged", 1);
        }
    } else {
        if (1 === hasChanged) {
            --numChanged;
            if ("reconnect" === action) { --numReconnect; }
            if ("reboot" === action) { --numReboot; }
            if ("reload" === action) { --numReload; }
            $(this).attr("hasChanged", 0);
        }
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

    $.ajax({
        'method': 'GET',
        'crossDomain': true,
        'url': urls.auth.href,
        'xhrFields': { 'withCredentials': true }
    }).done(function(data) {
        if (websock) { websock.close(); }
        websock = new WebSocket(urls.ws.href);
        websock.onmessage = function(evt) {
            var data = getJson(evt.data.replace(/\n/g, "\\n").replace(/\r/g, "\\r").replace(/\t/g, "\\t"));
            if (data) {
                processData(data);
            }
        };
    }).fail(function() {
        // Nothing to do, reload page and retry
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

function getParameterByName(name) {
    var match = RegExp('[?&]' + name + '=([^&]*)').exec(window.location.search);
    return match && decodeURIComponent(match[1].replace(/\+/g, ' '));
}

$(function() {

    initMessages();
    loadTimeZones();
    createCheckboxes();
    setInterval(function() { keepTime(); }, 1000);

    $("#menuLink").on("click", toggleMenu);
    $(".pure-menu-link").on("click", showPanel);
    $("progress").attr({ value: 0, max: 100 });

    $(".button-update").on("click", doUpdate);
    $(".button-update-password").on("click", doUpdatePassword);
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
    $(".button-add-switch-schedule").on("click", { schType: 1 }, addSchedule);
    <!-- removeIf(!light)-->
    $(".button-add-light-schedule").on("click", { schType: 2 }, addSchedule);
    <!-- endRemoveIf(!light)-->

    <!-- removeIf(!rfm69)-->
    $(".button-add-mapping").on('click', addMapping);
    $(".button-del-mapping").on('click', delMapping);
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

    $(document).on("change", "input", hasChanged);
    $(document).on("change", "select", hasChanged);

    // don't autoconnect when opening from filesystem
    if (window.location.protocol === "file:") { return; }

    // Check host param in query string
    if (host = getParameterByName('host')) {
        connect(host);
    } else {
        connectToCurrentURL();
    }

});
