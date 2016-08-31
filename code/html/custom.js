var update_timer = null;
var relaySlider;

function doUpdate() {
    var self = $(this);
    self.addClass("loading");
    $.ajax({
        'method': 'POST',
        'url': '/post',
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

function parseResponse(data) {

    // pre-process
    if ("network" in data) data.network = data.network.toUpperCase();
    if ("mqttStatus" in data) data.mqttStatus = data.mqttStatus ? "CONNECTED" : "NOT CONNECTED";

    // relay
    if ("relayStatus" in data) {
        $("input[name='relayStatus']")
            .prop("checked", data.relayStatus)
            .iphoneStyle("refresh");
    }

    // title
    if ("app" in data) {
        document.title = data.app;
        $(".pure-menu-heading").html(data.app);
    }

    // automatic assign
    Object.keys(data).forEach(function(key) {
        var id = "input[name=" + key + "]";
        if ($(id).length) $(id).val(data[key]);
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

    if ("updateInterval" in data) {
        if (update_timer) clearInterval(update_timer);
        if (data.updateInterval > 0) {
            update_timer = setInterval(update, data.updateInterval);
        }
    }

}

function update() {
    $.ajax({
        'method': 'GET',
        'url': '/status',
        'dataType': 'json'
    }).done(parseResponse);
}

function init() {
    $.ajax({
        'method': 'GET',
        'url': '/get',
        'dataType': 'json'
    }).done(parseResponse);
}

$(function() {
    $("#menuLink").on('click', toggleMenu);
    $(".button-update").on('click', doUpdate);
    $(".pure-menu-link").on('click', showPanel);
    relaySlider = $('#relayStatus').iphoneStyle({
        checkedLabel: 'ON',
        uncheckedLabel: 'OFF',
        onChange: function(elem, value) {
            $.ajax({
                'method': 'GET',
                'url': value ? '/relay/on' : '/relay/off',
                'dataType': 'json'
            });
            setTimeout(update, 200);
        }
    });
    init();
});
