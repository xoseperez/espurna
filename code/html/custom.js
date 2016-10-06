var websock;

function doUpdate() {
    var self = $(this);
    self.addClass("loading");
    $.ajax({
        'method': 'POST',
        'url': '/save',
        'dataType': 'json',
        'data': $("#formSave").serializeArray()
    }).done(function(data) {
        self.removeClass("loading");
    }).fail(function() {
        self.removeClass("loading");
    });
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
                onChange: function(elem, value) {
                    $.ajax({
                        'method': 'GET',
                        'url': value ? '/relay/on' : '/relay/off',
                        'dataType': 'json'
                    });
                }
            })
            .iphoneStyle("refresh");
    }

    // title
    if ("app" in data) {
        document.title = data.app;
        $(".pure-menu-heading").html(data.app);
    }

    // automatic assign
    Object.keys(data).forEach(function(key) {

        // Look for INPUTs
        var element = $("input[name=" + key + "]");
        if (element.length > 0) {
            if (element.attr('type') == 'checkbox') {
                element.prop("checked", data[key] == 1)
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
    $(".pure-menu-link").on('click', showPanel);

    var host = window.location.hostname;
    websock = new WebSocket('ws://' + host + ':81/');
    websock.onopen = function(evt) {};
    websock.onclose = function(evt) {};
    websock.onerror = function(evt) {};
    websock.onmessage = function(evt) {
        var data = getJson(evt.data);
        if (data) processData(data);
    };

}

$(init);
