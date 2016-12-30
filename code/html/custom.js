var websock;
var password = false;

function doUpdate() {
    var data = $("#formSave").serializeArray();
    websock.send(JSON.stringify({'config': data}));
    $(".powExpected").val(0);
    return false;
}

function doReset() {
    var response = window.confirm("Are you sure you want to reset the device?");
    if (response == false) return false;
    websock.send(JSON.stringify({'action': 'reset'}));
    return false;
}

function doReconnect() {
    var response = window.confirm("Are you sure you want to disconnect from the current WIFI network?");
    if (response == false) return false;
    websock.send(JSON.stringify({'action': 'reconnect'}));
    return false;
}

function doToggle(element, value) {
    var relayID = parseInt(element.attr("data"));
    websock.send(JSON.stringify({'action': value ? 'on' : 'off', 'relayID': relayID}));
    return false;
}

function randomString(length, chars) {
    var mask = '';
    if (chars.indexOf('a') > -1) mask += 'abcdefghijklmnopqrstuvwxyz';
    if (chars.indexOf('A') > -1) mask += 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
    if (chars.indexOf('#') > -1) mask += '0123456789';
    if (chars.indexOf('@') > -1) mask += 'ABCDEF';
    if (chars.indexOf('!') > -1) mask += '~`!@#$%^&*()_+-={}[]:";\'<>?,./|\\';
    var result = '';
    for (var i = length; i > 0; --i) result += mask[Math.round(Math.random() * (mask.length - 1))];
    return result;
}

function doGenerateAPIKey() {
    var apikey = randomString(16, '@#');
    $("input[name=\"apiKey\"]").val(apikey);
    return false;
}

function showPanel() {
    $(".panel").hide();
    $("#" + $(this).attr("data")).show();
    if ($("#layout").hasClass('active')) toggleMenu();
    $("input[type='checkbox']").iphoneStyle("calculateDimensions").iphoneStyle("refresh");
};

function toggleMenu() {
    $("#layout").toggleClass('active');
    $("#menu").toggleClass('active');
    $("#menuLink").toggleClass('active');
}

function createRelays(count) {

    var current = $("#relays > div").length;
    if (current > 0) return;

    var template = $("#relayTemplate .pure-g")[0];
    for (var relayID=0; relayID<count; relayID++) {
        var line = $(template).clone();
        $(line).find("input").each(function() {
            $(this).attr("data", relayID);
        });
        if (count > 1) $(".relay_id", line).html(" " + (relayID+1));
        line.appendTo("#relays");
        $(":checkbox", line).iphoneStyle({
            onChange: doToggle,
            resizeContainer: true,
            resizeHandle: true,
            checkedLabel: 'ON',
            uncheckedLabel: 'OFF'
        });

    }

}

function createIdxs(count) {

    var current = $("#idxs > div").length;
    if (current > 0) return;

    var template = $("#idxTemplate .pure-g")[0];
    for (var id=0; id<count; id++) {
        var line = $(template).clone();
        $(line).find("input").each(function() {
            $(this).attr("data", id).attr("tabindex", 43+id);
        });
        if (count > 1) $(".id", line).html(" " + id);
        line.appendTo("#idxs");
    }

}

function processData(data) {

    // title
    if ("app" in data) {
        var title = data.app;
		if ("version" in data) {
			title = title + " " + data.version;
		}
        $(".pure-menu-heading").html(title);
        if ("hostname" in data) {
            title = data.hostname + " - " + title;
        }
        document.title = title;
    }

    Object.keys(data).forEach(function(key) {

        // Actions
        if (key == "action") {

            if (data.action == "reload") {
                if (password) {

                    // Forget current authentication
                    $.ajax({
                        'method': 'GET',
                        'url': '/',
                        'async': false,
                        'username': "logmeout",
                        'password': "123456",
                        'headers': { "Authorization": "Basic xxx" }
                    }).done(function(data) {
                	    // If we don't get an error, we actually got an error as we expect an 401!
                	}).fail(function(){
                	    // We expect to get an 401 Unauthorized error! In this case we are successfully
                        // logged out and we redirect the user.
                	    window.location = "/";
                    });

                }
            }

            return;

        }

        // Wifi
        if (key == "wifi") {
            var groups = $("#panel-wifi .pure-g");
            for (var i in data.wifi) {
                var wifi = data.wifi[i];
                Object.keys(wifi).forEach(function(key) {
                    var id = "input[name=" + key + "]";
                    if ($(id, groups[i]).length) $(id, groups[i]).val(wifi[key]);
                });
            };
            return;
        }

        // Relay status
        if (key == "relayStatus") {

            var relays = data.relayStatus;
            createRelays(relays.length);

            for (var relayID in relays) {
                var element = $(".relayStatus[data=" + relayID + "]");
                if (element.length > 0) {
                    element
                        .prop("checked", relays[relayID])
                        .iphoneStyle("refresh");
                }
            }

            return;

        }

        // Domoticz
        if (key == "dczIdx") {
            var idxs = data.dczIdx;
            createIdxs(idxs.length);

            for (var i in idxs) {
                var element = $(".dczIdx[data=" + i + "]");
                if (element.length > 0) element.val(idxs[i]);
            }

            return;
        }


        // Messages
        if (key == "message") {
            window.alert(data.message);
            return;
        }

        // Enable options
        if (key.endsWith("Visible")) {
            var module = key.slice(0,-7);
            console.log(module);
            $(".module-" + module).show();
            return;
        }

        // Pre-process
        if (key == "network") {
            data.network = data.network.toUpperCase();
        }
        if (key == "mqttStatus") {
            data.mqttStatus = data.mqttStatus ? "CONNECTED" : "NOT CONNECTED";
        }

        // Look for INPUTs
        var element = $("input[name=" + key + "]");
        if (element.length > 0) {
            if (element.attr('type') == 'checkbox') {
                element
                    .prop("checked", data[key])
                    .iphoneStyle("refresh");
            } else {
                element.val(data[key]);
            }
            return;
        }

        // Look for SELECTs
        var element = $("select[name=" + key + "]");
        if (element.length > 0) {
            element.val(data[key]);
            return;
        }

    });

    // Auto generate an APIKey if none defined yet
    if ($("input[name='apiKey']").val() == "") {
        doGenerateAPIKey();
    }

}

function getJson(str) {
    try {
        return JSON.parse(str);
    } catch (e) {
        return false;
    }
}

function initWebSocket(host) {
    if (host === undefined) {
        host = window.location.hostname;
    }
    websock = new WebSocket('ws://' + host + '/ws');
    websock.onopen = function(evt) {};
    websock.onclose = function(evt) {};
    websock.onerror = function(evt) {};
    websock.onmessage = function(evt) {
        var data = getJson(evt.data);
        if (data) processData(data);
    };
}

function init() {

    $("#menuLink").on('click', toggleMenu);
    $(".button-update").on('click', doUpdate);
    $(".button-reset").on('click', doReset);
    $(".button-reconnect").on('click', doReconnect);
    $(".button-apikey").on('click', doGenerateAPIKey);
    $(".pure-menu-link").on('click', showPanel);

    $.ajax({
        'method': 'GET',
        'url': '/auth'
    }).done(function(data) {
        initWebSocket();
    });

}

$(init);
