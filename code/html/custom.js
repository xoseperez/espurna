var websock;
var csrf;

function doUpdate() {
    var data = $("#formSave").serializeArray();
    websock.send(JSON.stringify({'csrf': csrf, 'config': data}));
    $(".powExpected").val(0);
    return false;
}

function doReset() {
    var response = window.confirm("Are you sure you want to reset the device?");
    if (response == false) return false;
    websock.send(JSON.stringify({'csrf': csrf, 'action': 'reset'}));
    return false;
}

function doReconnect() {
    var response = window.confirm("Are you sure you want to disconnect from the current WIFI network?");
    if (response == false) return false;
    websock.send(JSON.stringify({'csrf': csrf, 'action': 'reconnect'}));
    return false;
}

function doToggle(element, value) {
    websock.send(JSON.stringify({'csrf': csrf, 'action': value ? 'on' : 'off'}));
    return false;
}

function showPanel() {
    $(".panel").hide();
    $("#" + $(this).attr("data")).show();
    if ($("#layout").hasClass('active')) toggleMenu();
};

function toggleMenu() {
    $("#layout").toggleClass('active');
    $("#menu").toggleClass('active');
    $("#menuLink").toggleClass('active');
}

function processData(data) {

    // CSRF
    if ("csrf" in data) {
        csrf = data.csrf;
    }

    // messages
    if ("message" in data) {
        window.alert(data.message);
    }

    // pre-process
    if ("network" in data) {
        data.network = data.network.toUpperCase();
    }
    if ("mqttStatus" in data) {
        data.mqttStatus = data.mqttStatus ? "CONNECTED" : "NOT CONNECTED";
    }

    // relay
    if ("relayStatus" in data) {
        $("input[name='relayStatus']")
            .prop("checked", data.relayStatus)
            .iphoneStyle({
                checkedLabel: 'ON',
                uncheckedLabel: 'OFF',
                onChange: doToggle
            })
            .iphoneStyle("refresh");
    }

    // title
    if ("app" in data) {
        $(".pure-menu-heading").html(data.app);
        var title = data.app;
        if ("hostname" in data) {
            title = data.hostname + " - " + title;
        }
        document.title = title;
    }

    // automatic assign
    Object.keys(data).forEach(function(key) {

        // Enable options
        if (key.endsWith("Visible")) {
            var module = key.slice(0,-7);
            console.log(module);
            $(".module-" + module).show();
            return;
        }

        // Look for INPUTs
        var element = $("input[name=" + key + "]");
        if (element.length > 0) {
            if (element.attr('type') == 'checkbox') {
                element
                    .prop("checked", data[key] == 1)
                    .iphoneStyle({
                        resizeContainer: false,
                        resizeHandle: false,
                        checkedLabel: 'ON',
                        uncheckedLabel: 'OFF'
                    })
                    .iphoneStyle("refresh");
            } else {
                element.val(data[key]);
            }
        }

        // Look for SELECTs
        var element = $("select[name=" + key + "]");
        if (element.length > 0) {
            element.val(data[key]);
        }

    });

    // WIFI
    var groups = $("#panel-wifi .pure-g");
    for (var i in data.wifi) {
        var wifi = data.wifi[i];
        Object.keys(wifi).forEach(function(key) {
            var id = "input[name=" + key + "]";
            if ($(id, groups[i]).length) $(id, groups[i]).val(wifi[key]);
        });
    };

}

function getJson(str) {
    try {
        return JSON.parse(str);
    } catch (e) {
        return false;
    }
}

function init() {

    $("#menuLink").on('click', toggleMenu);
    $(".button-update").on('click', doUpdate);
    $(".button-reset").on('click', doReset);
    $(".button-reconnect").on('click', doReconnect);
    $(".pure-menu-link").on('click', showPanel);

    var host = window.location.hostname;
    //host = '192.168.1.115';
    websock = new WebSocket('ws://' + host + '/ws');
    websock.onopen = function(evt) {};
    websock.onclose = function(evt) {};
    websock.onerror = function(evt) {};
    websock.onmessage = function(evt) {
        var data = getJson(evt.data);
        if (data) processData(data);
    };

}

$(init);
