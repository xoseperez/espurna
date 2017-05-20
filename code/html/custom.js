var websock;
var password = false;
var maxNetworks;
var host;
var port;

// http://www.the-art-of-web.com/javascript/validate-password/
function checkPassword(str) {
    // at least one number, one lowercase and one uppercase letter
    // at least eight characters that are letters, numbers or the underscore
    var re = /^(?=.*\d)(?=.*[a-z])(?=.*[A-Z])\w{8,}$/;
    return re.test(str);
}

function validateForm(form) {

    // password
    var adminPass1 = $("input[name='adminPass1']", form).val();
    if (adminPass1.length > 0 && !checkPassword(adminPass1)) {
        alert("The password you have entered is not valid, it must have at least 8 characters, 1 lower and 1 uppercase and 1 number!");
        return false;
    }

    var adminPass2 = $("input[name='adminPass2']", form).val();
    if (adminPass1 != adminPass2) {
        alert("Passwords are different!");
        return false;
    }

    return true;

}

function doColor() {
    var color = $(this).wheelColorPicker('getValue', 'css');
    websock.send(JSON.stringify({'action': 'color', 'data' : color}));
}


function doUpdate() {
    var form = $("#formSave");
    if (validateForm(form)) {
        var data = form.serializeArray();
        delete(data['filename']);
        websock.send(JSON.stringify({'config': data}));
        $(".powExpected").val(0);
        $("input[name='powExpectedReset']")
            .prop("checked", false)
            .iphoneStyle("refresh");
    }
    return false;
}

function doUpgrade() {

    var contents = $("input[name='upgrade']")[0].files[0];
    if (typeof contents == 'undefined') {
        alert("First you have to select a file from your computer.");
        return false;
    }
    var filename = $("input[name='upgrade']").val().split('\\').pop();

    var data = new FormData();
    data.append('upgrade', contents, filename);

    $.ajax({

        // Your server script to process the upload
        url: 'http://' + host + ':' + port + '/upgrade',
        type: 'POST',

        // Form data
        data: data,

        // Tell jQuery not to process data or worry about content-type
        // You *must* include these options!
        cache: false,
        contentType: false,
        processData: false,

        success: function(data, text) {
            $("#upgrade-progress").hide();
            if (data == 'OK') {
                alert("Firmware image uploaded, board rebooting. This page will be refreshed in 5 seconds.");
                setTimeout(function() {
                    window.location = "/";
                }, 5000);
            } else {
                alert("There was an error trying to upload the new image, please try again.");
            }
        },

        // Custom XMLHttpRequest
        xhr: function() {
            $("#upgrade-progress").show();
            var myXhr = $.ajaxSettings.xhr();
            if (myXhr.upload) {
                // For handling the progress of the upload
                myXhr.upload.addEventListener('progress', function(e) {
                    if (e.lengthComputable) {
                        $('progress').attr({ value: e.loaded, max: e.total });
                    }
                } , false);
            }
            return myXhr;
        },

    });

    return false;

}

function doUpdatePassword() {
    var form = $("#formPassword");
    if (validateForm(form)) {
        var data = form.serializeArray();
        websock.send(JSON.stringify({'config': data}));
    }
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
    websock.send(JSON.stringify({'action': 'relay', 'data': { 'id': relayID, 'status': value ? 1 : 0 }}));
    return false;
}

function backupSettings() {
    document.getElementById('downloader').src = 'http://' + host + ':' + port + '/config';
    return false;
}

function onFileUpload(event) {

    var inputFiles = this.files;
    if (inputFiles == undefined || inputFiles.length == 0) return false;
    var inputFile = inputFiles[0];
    this.value = "";

    var response = window.confirm("Previous settings will be overwritten. Are you sure you want to restore this settings?");
    if (response == false) return false;

    var reader = new FileReader();
    reader.onload = function(e) {
        var data = getJson(e.target.result);
        if (data) {
            websock.send(JSON.stringify({'action': 'restore', 'data': data}));
        } else {
            alert("The file is not a configuration backup or is corrupted");
        }
    };
    reader.readAsText(inputFile);

    return false;

}

function restoreSettings() {
    if (typeof window.FileReader !== 'function') {
        alert("The file API isn't supported on this browser yet.");
    } else {
        $("#uploader").click();
    }
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
            $(this).attr("data", id).attr("tabindex", 40+id);
        });
        if (count > 1) $(".id", line).html(" " + id);
        line.appendTo("#idxs");
    }

}

function delNetwork() {
    var parent = $(this).parents(".pure-g");
    $(parent).remove();
}

function moreNetwork() {
    var parent = $(this).parents(".pure-g");
    $("div.more", parent).toggle();
}

function addNetwork() {

    var numNetworks = $("#networks > div").length;
    if (numNetworks >= maxNetworks) {
        alert("Max number of networks reached");
        return;
    }

    var tabindex = 200 + numNetworks * 10;
    var template = $("#networkTemplate").children();
    var line = $(template).clone();
    $(line).find("input").each(function() {
        $(this).attr("tabindex", tabindex++);
    });
    $(line).find(".button-del-network").on('click', delNetwork);
    $(line).find(".button-more-network").on('click', moreNetwork);
    line.appendTo("#networks");

    return line;

}

function forgetCredentials() {
    $.ajax({
        'method': 'GET',
        'url': '/',
        'async': false,
        'username': "logmeout",
        'password': "123456",
        'headers': { "Authorization": "Basic xxx" }
    }).done(function(data) {
        return false;
        // If we don't get an error, we actually got an error as we expect an 401!
    }).fail(function(){
        // We expect to get an 401 Unauthorized error! In this case we are successfully
        // logged out and we redirect the user.
        return true;
    });
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

        // Web Modes
        if (key == "webMode") {
            password = data.webMode == 1;
            $("#layout").toggle(data.webMode == 0);
            $("#password").toggle(data.webMode == 1);
            $("#credentials").hide();
        }

        // Actions
        if (key == "action") {

            if (data.action == "reload") {
                if (password) forgetCredentials();
                setTimeout(function() {
                    window.location = "/";
                }, 1000);
            }

            return;

        }

        if (key == "color") {
            $("input[name='color']").wheelColorPicker('setValue', data[key], true);
            return;
        }

        if (key == "maxNetworks") {
            maxNetworks = parseInt(data.maxNetworks);
            return;
        }

        // Wifi
        if (key == "wifi") {

            var networks = data.wifi;

            for (var i in networks) {

                // add a new row
                var line = addNetwork();

                // fill in the blanks
                var wifi = data.wifi[i];
                Object.keys(wifi).forEach(function(key) {
                    var element = $("input[name=" + key + "]", line);
                    if (element.length) element.val(wifi[key]);
                });

            }

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
        if (key == "dczRelayIdx") {
            var idxs = data.dczRelayIdx;
            createIdxs(idxs.length);

            for (var i in idxs) {
                var element = $(".dczRelayIdx[data=" + i + "]");
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
        if (key == "ntpStatus") {
            data.ntpStatus = data.ntpStatus ? "SYNC'D" : "NOT SYNC'D";
        }
        if (key == "tmpUnits") {
            $("span#tmpUnit").html(data[key] == 1 ? "ºF" : "ºC");
        }

        // Look for INPUTs
        var element = $("input[name=" + key + "]");
        if (element.length > 0) {
            if (element.attr('type') == 'checkbox') {
                element
                    .prop("checked", data[key])
                    .iphoneStyle("refresh");
            } else if (element.attr('type') == 'radio') {
                element.val([data[key]]);
            } else {
                element.val(data[key]);
            }
            return;
        }

        // Look for SPANs
        var element = $("span[name=" + key + "]");
        if (element.length > 0) {
            element.html(data[key]);
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

function connect(h, p) {

    if (typeof h === 'undefined') {
        h = window.location.hostname;
    }
    if (typeof p === 'undefined') {
        p = location.port;
    }
    host = h;
    port = p;

    if (websock) websock.close();
    websock = new WebSocket('ws://' + host + ':' + port + '/ws');
    websock.onopen = function(evt) {
        console.log("Connected");
    };
    websock.onclose = function(evt) {
        console.log("Disconnected");
    };
    websock.onerror = function(evt) {
        console.log("Error: ", evt);
    };
    websock.onmessage = function(evt) {
        var data = getJson(evt.data);
        if (data) processData(data);
    };
}

function init() {

    $("#menuLink").on('click', toggleMenu);
    $(".button-update").on('click', doUpdate);
    $(".button-update-password").on('click', doUpdatePassword);
    $(".button-reset").on('click', doReset);
    $(".button-reconnect").on('click', doReconnect);
    $(".button-settings-backup").on('click', backupSettings);
    $(".button-settings-restore").on('click', restoreSettings);
    $('#uploader').on('change', onFileUpload);
    $(".button-apikey").on('click', doGenerateAPIKey);
    $(".button-upgrade").on('click', doUpgrade);
    $(".button-upgrade-browse").on('click', function() {
        $("input[name='upgrade']")[0].click();
        return false;
    });
    $("input[name='upgrade']").change(function (){
        var fileName = $(this).val();
        $("input[name='filename']").val(fileName.replace(/^.*[\\\/]/, ''));
    });
    $('progress').attr({ value: 0, max: 100 });
    $(".pure-menu-link").on('click', showPanel);
    $(".button-add-network").on('click', function() {
        $("div.more", addNetwork()).toggle();
    });
    $('input[name="color"]').wheelColorPicker({
        sliders: 'wsvp'
    }).on('sliderup', doColor);

    var host = window.location.hostname;
    var port = location.port;

    $.ajax({
        'method': 'GET',
        'url': 'http://' + host + ':' + port + '/auth'
    }).done(function(data) {
        connect();
    }).fail(function(){
        $("#credentials").show();
    });

}

$(init);
