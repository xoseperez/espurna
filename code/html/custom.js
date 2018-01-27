var websock;
var password = false;
var maxNetworks;
var maxSchedules;
var messages = [];
var webhost;

var numChanged = 0;
var numReboot = 0;
var numReconnect = 0;
var numReload = 0;

var useWhite = false;
var manifest;

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
        "SHT3X I2C", "BH1750"
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
        "PM1.0", "PM2.5", "PM10", "CO2", "Lux"
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

function keepTime() {
    if (now === 0) { return; }
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
        var value = parseInt(time_zones[i], 10);
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

// These fields will always be a list of values
var is_group = [
    "ssid", "pass", "gw", "mask", "ip", "dns",
    "schEnabled", "schSwitch","schAction","schHour","schMinute","schWDs",
    "relayBoot", "relayPulse", "relayTime",
    "mqttGroup", "mqttGroupInv",
    "dczRelayIdx", "dczMagnitude",
    "tspkRelay", "tspkMagnitude",
    "ledMode",
    "adminPass"
];

function getData(form) {

    var data = {};

    // Populate data
    $("input,select", form).each(function() {
        var name = $(this).attr("name");
        if (name) {
            var value = "";

            // Do not report these fields
            if (name === "filename" || name === "rfbcode" ) {
                return;
            }

            // Grab the value
            if ($(this).attr("type") === "checkbox") {
                value = $(this).is(":checked") ? 1 : 0;
            } else if ($(this).attr("type") === "radio") {
                if (!$(this).is(":checked")) {return;}
                value = $(this).val();
            } else {
                value = $(this).val();
            }

            // Build the object
            if (name in data) {
                if (!Array.isArray(data[name])) data[name] = [data[name]];
                data[name].push(value);
            } else if (is_group.indexOf(name) >= 0) {
                data[name] = [value];
            } else {
                data[name] = value;
            }

        }
    });

    // Post process
    if ("schSwitch" in data) {
        data["schSwitch"].push(0xFF);
    } else {
        data["schSwitch"] = [0xFF];
    }

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

function resetOriginals() {
    $("input,select").each(function() {
        $(this).attr("original", $(this).val());
    });
    numReboot = numReconnect = numReload = 0;
}

function doReload(milliseconds) {
    milliseconds = (typeof milliseconds == "undefined") ?
        0 :
        parseInt(milliseconds, 10);
    setTimeout(function() {
        window.location.reload();
    }, milliseconds);
}

function doUpgrade() {

    var contents = $("input[name='upgrade']")[0].files[0];
    if (typeof contents === "undefined") {
        alert("First you have to select a file from your computer.");
        return false;
    }
    var filename = $("input[name='upgrade']").val().split("\\").pop();

    var data = new FormData();
    data.append("upgrade", contents, filename);

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
            if (data === "OK") {
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

    return false;

}

function doUpdatePassword() {
    var form = $("#formPassword");
    if (validateForm(form)) {
        var data = getData(form);
        websock.send(JSON.stringify({"config": data}));
    }
    return false;
}

function doReboot(ask) {

    var response;

    ask = (typeof ask == "undefined") ? true : ask;

    if (numChanged > 0) {
        response = window.confirm("Some changes have not been saved yet, do you want to save them first?");
        if (response === true) {
          return doUpdate();
        }
    }

    if (ask) {
        response = window.confirm("Are you sure you want to reboot the device?");
        if (response === false) {
          return false;
        }
    }

    websock.send(JSON.stringify({"action": "reboot"}));
    doReload(5000);
    return false;

}

function doReconnect(ask) {
    var response;

    ask = (typeof ask == "undefined") ? true : ask;

    if (numChanged > 0) {
        response = window.confirm("Some changes have not been saved yet, do you want to save them first?");
        if (response === true) {
          return doUpdate();
        }
    }

    if (ask) {
        response = window.confirm("Are you sure you want to disconnect from the current WIFI network?");
        if (response === false) {
          return false;
        }
    }

    websock.send(JSON.stringify({"action": "reconnect"}));
    doReload(5000);
    return false;

}

function doUpdate() {

    var form = $("#formSave");
    if (validateForm(form)) {

        // Get data
        var data = getData(form);
        websock.send(JSON.stringify({"config": data}));

        // Empty special fields
        $(".pwrExpected").val(0);
        $("input[name='pwrResetCalibration']").
            prop("checked", false).
            iphoneStyle("refresh");

        // Change handling
        numChanged = 0;
        setTimeout(function() {

            var response;

            if (numReboot > 0) {
                response = window.confirm("You have to reboot the board for the changes to take effect, do you want to do it now?");
                if (response === true) { doReboot(false); }
            } else if (numReconnect > 0) {
                response = window.confirm("You have to reconnect to the WiFi for the changes to take effect, do you want to do it now?");
                if (response === true) { doReconnect(false); }
            } else if (numReload > 0) {
                response = window.confirm("You have to reload the page to see the latest changes, do you want to do it now?");
                if (response === true) { doReload(); }
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
    if (inputFiles === undefined || inputFiles.length === 0) {
      return false;
    }
    var inputFile = inputFiles[0];
    this.value = "";

    var response = window.confirm("Previous settings will be overwritten. Are you sure you want to restore this settings?");
    if (response === false) {
      return false;
    }

    var reader = new FileReader();
    reader.onload = function(e) {
        var data = getJson(e.target.result);
        if (data) {
            websock.send(JSON.stringify({"action": "restore", "data": data}));
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

function doToggle(element, value) {
    var relayID = parseInt(element.attr("data"), 10);
    websock.send(JSON.stringify({"action": "relay", "data": { "id": relayID, "status": value ? 1 : 0 }}));
    return false;
}

function doScan() {
    $("#scanResult").html("");
    $("div.scan.loading").show();
    websock.send(JSON.stringify({"action": "scan", "data": {}}));
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
    $("#" + $(this).attr("data")).show();
    if ($("#layout").hasClass("active")) { toggleMenu(); }
    $("input[type='checkbox']").
        iphoneStyle("calculateDimensions").
        iphoneStyle("refresh");
}

// -----------------------------------------------------------------------------
// Relays & magnitudes mapping
// -----------------------------------------------------------------------------

function createRelayList(data, container, template_name) {

    var current = $("#" + container + " > div").length;
    if (current > 0) {
      return;
    }

    var template = $("#" + template_name + " .pure-g")[0];
    for (var i=0; i<data.length; i++) {
        var line = $(template).clone();
        $("label", line).html("Switch #" + i);
        $("input", line).attr("tabindex", 40 + i).val(data[i]);
        line.appendTo("#" + container);
    }

}

function createMagnitudeList(data, container, template_name) {

    var current = $("#" + container + " > div").length;
    if (current > 0) {
      return;
    }

    var template = $("#" + template_name + " .pure-g")[0];
    for (var i=0; i<data.length; i++) {
        var line = $(template).clone();
        $("label", line).html(magnitudeType(data[i].type) + " #" + parseInt(data[i].index, 10));
        $("div.hint", line).html(data[i].name);
        $("input", line).attr("tabindex", 40 + i).val(data[i].idx);
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

function addSchedule() {
    var numSchedules = $("#schedules > div").length;
    if (numSchedules >= maxSchedules) {
        alert("Max number of schedules reached");
        return null;
    }
    var tabindex = 200 + numSchedules * 10;
    var template = $("#scheduleTemplate").children();
    var line = $(template).clone();
    $(line).find("input").each(function() {
        $(this).attr("tabindex", tabindex);
        tabindex++;
    });
    $(line).find(".button-del-schedule").on("click", delSchedule);
    $(line).find(".button-more-schedule").on("click", moreSchedule);
    line.appendTo("#schedules");
    return line;
}

// -----------------------------------------------------------------------------
// Relays
// -----------------------------------------------------------------------------

function initRelays(data) {

    var current = $("#relays > div").length;
    if (current > 0) {
      return;
    }

    var template = $("#relayTemplate .pure-g")[0];
    for (var i=0; i<data.length; i++) {

        // Add relay fields
        var line = $(template).clone();
        $(".id", line).html(i);
        $("input", line).attr("data", i);
        line.appendTo("#relays");
        $(":checkbox", line).iphoneStyle({
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
    if (current > 0) {
      return;
    }

    var template = $("#relayConfigTemplate").children();
    for (var i=0; i < data.length; i++) {
        var line = $(template).clone();
        $("span.gpio", line).html(data[i].gpio);
        $("span.id", line).html(i);
        $("select[name='relayBoot']", line).val(data[i].boot);
        $("select[name='relayPulse']", line).val(data[i].pulse);
        $("input[name='relayTime']", line).val(data[i].pulse_ms);
        $("input[name='mqttGroup']", line).val(data[i].group);
        $("select[name='mqttGroupInv']", line).val(data[i].group_inv);
        line.appendTo("#relayConfig");
    }

}

// -----------------------------------------------------------------------------
// Sensors & Magnitudes
// -----------------------------------------------------------------------------

function initMagnitudes(data) {

    // check if already initialized
    var done = $("#magnitudes > div").length;
    if (done > 0) {
      return;
    }

    // add templates
    var template = $("#magnitudeTemplate").children();
    for (var i=0; i<data.length; i++) {
        var line = $(template).clone();
        $("label", line).html(magnitudeType(data[i].type) + " #" + parseInt(data[i].index, 10));
        $("div.hint", line).html(data[i].description);
        $("input", line).attr("data", i);
        line.appendTo("#magnitudes");
    }

}

function getManifest(sensor_id) {
    for (var i in manifest) {
        if (manifest[i].sensor_id === sensor_id) {
          return manifest[i];
        }
    }
    return null;
}

// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

function initColorRGB() {

    // check if already initialized
    var done = $("#colors > div").length;
    if (done > 0) {
      return;
    }

    // add template
    var template = $("#colorRGBTemplate").children();
    var line = $(template).clone();
    line.appendTo("#colors");

    // init color wheel
    $("input[name='color']").wheelColorPicker({
        sliders: "wrgbp"
    }).on("sliderup", function() {
        var value = $(this).wheelColorPicker("getValue", "css");
        websock.send(JSON.stringify({"action": "color", "data" : {"rgb": value}}));
    });

    // init bright slider
    $("#brightness").on("change", function() {
        var value = $(this).val();
        var parent = $(this).parents(".pure-g");
        $("span", parent).html(value);
        websock.send(JSON.stringify({"action": "color", "data" : {"brightness": value}}));
    });

}

function initColorHSV() {

    // check if already initialized
    var done = $("#colors > div").length;
    if (done > 0) {
      return;
    }

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
        websock.send(JSON.stringify({"action": "color", "data" : {"hsv": value}}));
    });

}

function initChannels(num) {

    // check if already initialized
    var done = $("#channels > div").length > 0;
    if (done) {
      return;
    }

    // does it have color channels?
    var colors = $("#colors > div").length > 0;

    // calculate channels to create
    var max = num;
    if (colors) {
        max = num % 3;
        if ((max > 0) & useWhite) max--;
    }
    var start = num - max;

    // add templates
    var template = $("#channelTemplate").children();
    for (var i=0; i<max; i++) {

        var channel_id = start + i;
        var line = $(template).clone();
        $("span.slider", line).attr("data", channel_id);
        $("input.slider", line).attr("data", channel_id).on("change", function() {
            var id = $(this).attr("data");
            var value = $(this).val();
            var parent = $(this).parents(".pure-g");
            $("span", parent).html(value);
            websock.send(JSON.stringify({"action": "channel", "data" : { "id": id, "value": value }}));
        });
        $("label", line).html("Channel " + (channel_id + 1));

        line.appendTo("#channels");

    }

}

// -----------------------------------------------------------------------------
// RFBridge
// -----------------------------------------------------------------------------

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

function rfbLearn() {
    var parent = $(this).parents(".pure-g");
    var input = $("input", parent);
    websock.send(JSON.stringify({"action": "rfblearn", "data" : {"id" : input.attr("data-id"), "status": input.attr("data-status")}}));
}

function rfbForget() {
    var parent = $(this).parents(".pure-g");
    var input = $("input", parent);
    websock.send(JSON.stringify({"action": "rfbforget", "data" : {"id" : input.attr("data-id"), "status": input.attr("data-status")}}));
}

function rfbSend() {
    var parent = $(this).parents(".pure-g");
    var input = $("input", parent);
    websock.send(JSON.stringify({"action": "rfbsend", "data" : {"id" : input.attr("data-id"), "status": input.attr("data-status"), "data": input.val()}}));
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

        // ---------------------------------------------------------------------
        // Web mode
        // ---------------------------------------------------------------------

        if (key ==="webMode") {
            password = data.webMode == 1;
            $("#layout").toggle(data.webMode === 0);
            $("#password").toggle(data.webMode === 1);
        }

        // ---------------------------------------------------------------------
        // Actions
        // ---------------------------------------------------------------------

        if (key === "action") {
            if (data.action === "reload") doReload(1000);
            return;
        }

        // ---------------------------------------------------------------------
        // RFBridge
        // ---------------------------------------------------------------------

        if (key === "rfbCount") {
            for (i=0; i<data.rfbCount; i++) addRfbNode();
            return;
        }

        if (key === "rfbrawVisible") {
            $("input[name='rfbcode']").attr("maxlength", 116);
        }

        if (key === "rfb") {
            var nodes = data.rfb;
            for (i in nodes) {
                var node = nodes[i];
                $("input[name='rfbcode'][data-id='" + node["id"] + "'][data-status='" + node["status"] + "']").val(node["data"]);
            }
            return;
        }

        // ---------------------------------------------------------------------
        // Lights
        // ---------------------------------------------------------------------

        if (key === "rgb") {
            initColorRGB();
            $("input[name='color']").wheelColorPicker("setValue", data[key], true);
            return;
        }

        if (key === "hsv") {
            initColorHSV();
            // wheelColorPicker expects HSV to be between 0 and 1 all of them
            var chunks = data[key].split(",");
            var obj = {};
            obj.h = chunks[0] / 360;
            obj.s = chunks[1] / 100;
            obj.v = chunks[2] / 100;
            $("input[name='color']").wheelColorPicker("setColor", obj);
            return;
        }

        if (key === "brightness") {
            $("#brightness").val(data[key]);
            $("span.brightness").html(data[key]);
            return;
        }

        if (key === "channels") {
            var len = data[key].length;
            initChannels(len);
            for (i=0; i<len; i++) {
                $("input.slider[data=" + i + "]").val(data[key][i]);
                $("span.slider[data=" + i + "]").html(data[key][i]);
            }
            return;
        }

        if (key === "useWhite") {
            useWhite = data[key];
        }

        // ---------------------------------------------------------------------
        // Sensors & Magnitudes
        // ---------------------------------------------------------------------

        if (key === "magnitudes") {
            initMagnitudes(data[key]);
            for (i=0; i<data[key].length; i++) {
                var error = data[key][i].error || 0;
                var text = (error === 0) ?
                    data[key][i].value + data[key][i].units :
                    magnitudeError(error);
                $("input[name='magnitude'][data='" + i + "']").val(text);
            }
            return;
        }

        if (key === "manifest") {
            manifest = data[key];
        }

        // ---------------------------------------------------------------------
        // WiFi
        // ---------------------------------------------------------------------

        if (key === "maxNetworks") {
            maxNetworks = parseInt(data.maxNetworks, 10);
            return;
        }

        if (key === "wifi") {
            for (i in data.wifi) {
                var nwk_line = addNetwork();
                var wifi = data.wifi[i];
                Object.keys(wifi).forEach(function(key) {
                    $("input[name='" + key + "']", nwk_line).val(wifi[key]);
                });
            }
            return;
        }

        if (key == "scanResult") {
            $("div.scan.loading").hide();
        }

        // -----------------------------------------------------------------------------
        // Relays scheduler
        // -----------------------------------------------------------------------------

        if (key === "maxSchedules") {
            maxSchedules = parseInt(data.maxSchedules, 10);
            return;
        }

        if (key === "schedule") {
            for (i in data.schedule) {
                var sch_line = addSchedule();
                var schedule = data.schedule[i];
                Object.keys(schedule).forEach(function(key) {
                    $("input[name='" + key + "']", sch_line).val(schedule[key]);
                    $("select[name='" + key + "']", sch_line).prop("value", schedule[key]);
                    $(":checkbox", sch_line).prop("checked", schedule[key]);
                });
            }
            return;
        }

        // ---------------------------------------------------------------------
        // Relays
        // ---------------------------------------------------------------------

        if (key === "relayStatus") {
            initRelays(data[key]);
            for (i in data[key]) {

                // Set the status for each relay
                $("input.relayStatus[data='" + i + "']").
                    prop("checked", data[key][i]).
                    iphoneStyle("refresh");

            }
            return;
        }

        // Relay configuration
        if (key === "relayConfig") {
            initRelayConfig(data[key]);
            return;
        }

        // ---------------------------------------------------------------------
        // Domoticz
        // ---------------------------------------------------------------------

        // Domoticz - Relays
        if (key === "dczRelays") {
            createRelayList(data[key], "dczRelays", "dczRelayTemplate");
            return;
        }

        // Domoticz - Magnitudes
        if (key === "dczMagnitudes") {
            createMagnitudeList(data[key], "dczMagnitudes", "dczMagnitudeTemplate");
            return;
        }

        // ---------------------------------------------------------------------
        // Thingspeak
        // ---------------------------------------------------------------------

        // Thingspeak - Relays
        if (key === "tspkRelays") {
            createRelayList(data[key], "tspkRelays", "tspkRelayTemplate");
            return;
        }

        // Thingspeak - Magnitudes
        if (key === "tspkMagnitudes") {
            createMagnitudeList(data[key], "tspkMagnitudes", "tspkMagnitudeTemplate");
            return;
        }

        // ---------------------------------------------------------------------
        // General
        // ---------------------------------------------------------------------

        // Messages
        if (key === "message") {
            window.alert(messages[data.message]);
            return;
        }

        // Enable options
        var position = key.indexOf("Visible");
        if (position > 0 && position === key.length - 7) {
            var module = key.slice(0,-7);
            $(".module-" + module).show();
            return;
        }

        if (key === "now") {
            now = data[key];
            ago = 0;
            return;
        }

        // Pre-process
        if (key === "network") {
            data.network = data.network.toUpperCase();
        }
        if (key === "mqttStatus") {
            data.mqttStatus = data.mqttStatus ? "CONNECTED" : "NOT CONNECTED";
        }
        if (key === "ntpStatus") {
            data.ntpStatus = data.ntpStatus ? "SYNC'D" : "NOT SYNC'D";
        }
        if (key === "uptime") {
            var uptime  = parseInt(data[key], 10);
            var seconds = uptime % 60; uptime = parseInt(uptime / 60, 10);
            var minutes = uptime % 60; uptime = parseInt(uptime / 60, 10);
            var hours   = uptime % 24; uptime = parseInt(uptime / 24, 10);
            var days    = uptime;
            data[key] = days + "d " + zeroPad(hours, 2) + "h " + zeroPad(minutes, 2) + "m " + zeroPad(seconds, 2) + "s";
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
                    prop("checked", data[key]).
                    iphoneStyle("refresh");
            } else if (input.attr("type") === "radio") {
                input.val([data[key]]);
            } else {
                pre = input.attr("pre") || "";
                post = input.attr("post") || "";
                input.val(pre + data[key] + post);
            }
        }

        // Look for SPANs
        var span = $("span[name='" + key + "']");
        if (span.length > 0) {
            pre = span.attr("pre") || "";
            post = span.attr("post") || "";
            span.html(pre + data[key] + post);
        }

        // Look for SELECTs
        var select = $("select[name='" + key + "']");
        if (select.length > 0) {
            select.val(data[key]);
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
        originalValue = $(this).attr("original") == "true";
    } else {
        newValue = $(this).val();
        originalValue = $(this).attr("original");
    }
    var hasChanged = $(this).attr("hasChanged") || 0;
    var action = $(this).attr("action");

    if (typeof originalValue === "undefined") { return; }
    if (action === "none") { return; }

    if (newValue !== originalValue) {
        if (hasChanged === 0) {
            ++numChanged;
            if (action === "reconnect") ++numReconnect;
            if (action === "reboot") ++numReboot;
            if (action === "reload") ++numReload;
            $(this).attr("hasChanged", 1);
        }
    } else {
        if (hasChanged === 1) {
            --numChanged;
            if (action === "reconnect") --numReconnect;
            if (action === "reboot") --numReboot;
            if (action === "reload") --numReload;
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
    if (host.indexOf("http") !== 0) {return;}

    webhost = host;
    wshost = host.replace("http", "ws") + "ws";

    if (websock) websock.close();
    websock = new WebSocket(wshost);
    websock.onmessage = function(evt) {
        var data = getJson(evt.data);
        if (data) processData(data);
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
    $(".button-settings-backup").on("click", doBackup);
    $(".button-settings-restore").on("click", doRestore);
    $("#uploader").on("change", onFileUpload);
    $(".button-upgrade").on("click", doUpgrade);

    $(".button-apikey").on("click", generateAPIKey);
    $(".button-upgrade-browse").on("click", function() {
        $("input[name='upgrade']")[0].click();
        return false;
    });
    $("input[name='upgrade']").change(function (){
        var fileName = $(this).val();
        $("input[name='filename']").val(fileName.replace(/^.*[\\\/]/, ""));
    });
    $(".button-add-network").on("click", function() {
        $(".more", addNetwork()).toggle();
    });
    $(".button-add-schedule").on("click", addSchedule);

    $(document).on("change", "input", hasChanged);
    $(document).on("change", "select", hasChanged);

    connect();

});
