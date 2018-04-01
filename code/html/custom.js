var websock;
var password = false;
var maxNetworks;
var maxSchedules;
var messages = [];
var free_size = 0;
var webhost;

var numChanged = 0;
var numReboot = 0;
var numReconnect = 0;
var numReload = 0;

var useWhite = false;

var now = 0;
var ago = 0;

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

function sensorName(id) {
    var names = [
        "DHT", "Dallas", "Emon Analog", "Emon ADC121", "Emon ADS1X15",
        "HLW8012", "V9261F", "ECH1560", "Analog", "Digital",
        "Events", "PMSX003", "BMX280", "MHZ19", "SI7021",
        "SHT3X I2C", "BH1750", "PZEM004T", "AM2320 I2C", "GUVAS12SD"
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
        "Analog", "Digital", "Events",
        "PM1.0", "PM2.5", "PM10", "CO2", "Lux", "UV"
    ];
    if (1 <= type && type <= types.length) {
        return types[type - 1];
    }
    return null;
}

function magnitudeError(error) {
    var errors = [
        "OK", "Out of Range", "Warming Up", "Timeout", "Wrong ID",
        "Data Error", "I2C Error", "GPIO Error"
    ];
    if (0 <= error && error < errors.length) {
        return errors[error];
    }
    return "Error " + error;
}

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
    if (0 === now) { return; }
    var date = new Date(now * 1000);
    var text = date.toISOString().substring(0, 19).replace("T", " ");
    $("input[name='now']").val(text);
    $("span[name='now']").html(text);
    $("span[name='ago']").html(ago);
    now++;
    ago++;
}

// http://www.the-art-of-web.com/javascript/validate-password/
function checkPassword(str) {
    // at least one lowercase and one uppercase letter or number
    // at least five characters (letters, numbers or special characters)
    var re = /^(?=.*[A-Z\d])(?=.*[a-z])[\w~!@#$%^&*\(\)<>,.\?;:{}\[\]\\|]{5,}$/;
    return re.test(str);
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

    // password
    var adminPass1 = $("input[name='adminPass']", form).first().val();
    if (adminPass1.length > 0 && !checkPassword(adminPass1)) {
        alert("The password you have entered is not valid, it must have at least 5 characters, 1 lowercase and 1 uppercase or number!");
        return false;
    }

    var adminPass2 = $("input[name='adminPass']", form).last().val();
    if (adminPass1 !== adminPass2) {
        alert("Passwords are different!");
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
        "schEnabled", "schSwitch","schAction","schType","schHour","schMinute","schWDs",
        "relayBoot", "relayPulse", "relayTime",
        "mqttGroup", "mqttGroupInv", "relayOnDisc",
        "dczRelayIdx", "dczMagnitude",
        "tspkRelay", "tspkMagnitude",
        "ledMode",
        "adminPass"
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
            callback(0xE9 === evt.target.result.charCodeAt(0));
        }
    };

    var blob = file.slice(0, 1);
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
            url: webhost + "upgrade",
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
        $("input[name='pwrResetCalibration']").
            prop("checked", false).
            iphoneStyle("refresh");
        $("input[name='pwrResetE']").
            prop("checked", false).
            iphoneStyle("refresh");

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
    document.getElementById("downloader").src = webhost + "config";
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
            alert(messages[4]);
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
    websock.send(JSON.stringify({"action": "factory_reset"}));
    doReload(5000);
    return false;
}

function doToggle(element, value) {
    var id = parseInt(element.attr("data"), 10);
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
    $("#" + $(this).attr("data")).show().
        find("input[type='checkbox']").
        iphoneStyle("calculateDimensions").
        iphoneStyle("refresh");
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
    line.appendTo("#schedules");

    $(line).find("input[type='checkbox']").
        prop("checked", false).
        iphoneStyle("calculateDimensions").
        iphoneStyle("refresh");

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
        $("input", line).attr("data", i);
        line.appendTo("#relays");
        $("input[type='checkbox']", line).iphoneStyle({
            onChange: doToggle,
            resizeContainer: true,
            resizeHandle: true,
            checkedLabel: "ON",
            uncheckedLabel: "OFF"
        });

        // Populate the relay SELECTs
        $("select.isrelay").append(
            $("<option></option>").attr("value",i).text("Switch #" + i));

    }


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
// Sensors & Magnitudes
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

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
    var template = $("#channelTemplate").children();
    for (var i=0; i<max; i++) {

        var channel_id = start + i;
        var line = $(template).clone();
        $("span.slider", line).attr("data", channel_id);
        $("input.slider", line).attr("data", channel_id).on("change", onChannelSliderChange);
        $("label", line).html("Channel " + (channel_id + 1));

        line.appendTo("#channels");

        $("select.islight").append(
            $("<option></option>").attr("value",i).text("Channel #" + i));

    }

}

// -----------------------------------------------------------------------------
// RFBridge
// -----------------------------------------------------------------------------

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

        // ---------------------------------------------------------------------
        // Lights
        // ---------------------------------------------------------------------

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

        if ("useWhite" === key) {
            useWhite = value;
        }

        // ---------------------------------------------------------------------
        // Sensors & Magnitudes
        // ---------------------------------------------------------------------

        if ("magnitudes" === key) {
            initMagnitudes(value);
            for (i in value) {
                var magnitude = value[i];
                var error = magnitude.error || 0;
                var text = (0 === error) ?
                    magnitude.value + magnitude.units :
                    magnitudeError(error);
                $("input[name='magnitude'][data='" + i + "']").val(text);
            }
            return;
        }

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
                    $("input[type='checkbox'][name='" + key + "']", sch_line).
                        prop("checked", sch_value).
                        iphoneStyle("refresh");
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

                // Set the status for each relay
                $("input.relayStatus[data='" + i + "']").
                    prop("checked", value[i]).
                    iphoneStyle("refresh");

            }
            return;
        }

        // Relay configuration
        if ("relayConfig" === key) {
            initRelayConfig(value);
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
        if ("dczMagnitudes" === key) {
            createMagnitudeList(value, "dczMagnitudes", "dczMagnitudeTemplate");
            return;
        }

        // ---------------------------------------------------------------------
        // Thingspeak
        // ---------------------------------------------------------------------

        // Thingspeak - Relays
        if ("tspkRelays" === key) {
            createRelayList(value, "tspkRelays", "tspkRelayTemplate");
            return;
        }

        // Thingspeak - Magnitudes
        if ("tspkMagnitudes" === key) {
            createMagnitudeList(value, "tspkMagnitudes", "tspkMagnitudeTemplate");
            return;
        }

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
            $("#weblog").append(value);
            $("#weblog").scrollTop($("#weblog")[0].scrollHeight - $("#weblog").height());
            return;
        }

        // Enable options
        var position = key.indexOf("Visible");
        if (position > 0 && position === key.length - 7) {
            var module = key.slice(0,-7);
            $(".module-" + module).show();
            return;
        }

        if ("now" === key) {
            now = value;
            ago = 0;
            return;
        }

        if ("free_size" === key) {
            free_size = parseInt(value, 10);
        }

        // Pre-process
        if ("network" === key) {
            value = value.toUpperCase();
        }
        if ("mqttStatus" === key) {
            value = value ? "CONNECTED" : "NOT CONNECTED";
        }
        if ("ntpStatus" === key) {
            value = value ? "SYNC'D" : "NOT SYNC'D";
        }
        if ("uptime" === key) {
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
                input.
                    prop("checked", value).
                    iphoneStyle("refresh");
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

function connect(host) {

    if (typeof host === "undefined") {
        host = window.location.href.replace("#", "");
    } else {
        if (host.indexOf("http") !== 0) {
            host = "http://" + host + "/";
        }
    }
    if (host.indexOf("http") !== 0) { return; }

    webhost = host;
    var wshost = host.replace("http", "ws") + "ws";

    if (websock) { websock.close(); }
    websock = new WebSocket(wshost);
    websock.onmessage = function(evt) {
        var data = getJson(evt.data.replace(/\n/g, "\\n").replace(/\r/g, "\\r").replace(/\t/g, "\\t"));
        if (data) {
            processData(data);
        }
    };
}

$(function() {

    initMessages();
    loadTimeZones();
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
    $(".button-add-light-schedule").on("click", { schType: 2 }, addSchedule);

    $(document).on("change", "input", hasChanged);
    $(document).on("change", "select", hasChanged);

    connect();

});
