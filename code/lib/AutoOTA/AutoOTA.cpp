#include "AutoOTA.h"
#include <functional>
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>

void AutoOTAClass::setServer(String server) {
    _server = server;
}

void AutoOTAClass::setModel(String model) {
    _model = model;
}

void AutoOTAClass::setVersion(String version) {
    _version = version;
}

void AutoOTAClass::onMessage(TMessageFunction fn) {
    _callback = fn;
}

String AutoOTAClass::getNewVersion() {
    return _newVersion;
}

String AutoOTAClass::getNewFirmware() {
    return _newFirmware;
}

String AutoOTAClass::getNewFileSystem() {
    return _newFileSystem;
}

int AutoOTAClass::getErrorNumber() {
    return _errorNumber;
}

String AutoOTAClass::getErrorString() {
    return _errorString;
}

String AutoOTAClass::_getPayload() {

    HTTPClient http;
    char url[100];
    String payload = "";

    _callback(AUTO_OTA_START);

    sprintf(url, "%s/%s/%s", _server.c_str(), _model.c_str(), _version.c_str());
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) payload = http.getString();
    http.end();

    return payload;

}

bool AutoOTAClass::_checkUpdates() {

    String payload = _getPayload();
    if (payload.length() == 0) {
        _callback(AUTO_OTA_NO_RESPONSE_ERROR);
        return false;
    }

    StaticJsonBuffer<500> jsonBuffer;
    JsonObject& response = jsonBuffer.parseObject(payload);

    if (!response.success()) {
        _callback(AUTO_OTA_PARSE_ERROR);
        return false;
    }

    if (response.size() == 0) {
        _callback(AUTO_OTA_UPTODATE);
        return false;
    }

    _newVersion = response.get<String>("version");
    _newFileSystem = response.get<String>("spiffs");
    _newFirmware = response.get<String>("firmware");

    _callback(AUTO_OTA_UPDATING);
    return true;

}

void AutoOTAClass::_doUpdate() {

    char url[100];
    bool error = false;
    uint8_t updates = 0;

    if (_newFileSystem.length() > 0) {

        // Update SPIFFS
        sprintf(url, "%s/%s", _server.c_str(), _newFileSystem.c_str());
        t_httpUpdate_return ret = ESPhttpUpdate.updateSpiffs(url);

        if (ret == HTTP_UPDATE_FAILED) {
            error = true;
            _errorNumber = ESPhttpUpdate.getLastError();
            _errorString = ESPhttpUpdate.getLastErrorString();
            _callback(AUTO_OTA_FILESYSTEM_UPDATE_ERROR);
        } else if (ret == HTTP_UPDATE_OK) {
            updates++;
            _callback(AUTO_OTA_FILESYSTEM_UPDATED);
        }

    }

    if (!error && (_newFirmware.length() > 0)) {

        // Update binary
        sprintf(url, "%s%s", _server.c_str(), _newFirmware.c_str());
        t_httpUpdate_return ret = ESPhttpUpdate.update(url);

        if (ret == HTTP_UPDATE_FAILED) {
            error = true;
            _errorNumber = ESPhttpUpdate.getLastError();
            _errorString = ESPhttpUpdate.getLastErrorString();
            _callback(AUTO_OTA_FIRMWARE_UPDATE_ERROR);
        } else if (ret == HTTP_UPDATE_OK) {
            updates++;
            _callback(AUTO_OTA_FIRMWARE_UPDATED);
        }

    }

    if (!error && (updates > 0)) {
        _callback(AUTO_OTA_RESET);
        ESP.restart();
    }

}

void AutoOTAClass::handle() {
    _callback(AUTO_OTA_START);
    if (_checkUpdates()) _doUpdate();
    _callback(AUTO_OTA_END);
}

AutoOTAClass AutoOTA;
