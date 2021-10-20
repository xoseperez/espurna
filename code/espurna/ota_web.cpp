/*

Part of the WEBSERVER module

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if WEB_SUPPORT && OTA_WEB_SUPPORT

#include "ota.h"
#include "web.h"
#include "ws.h"

namespace ota {
namespace web {
namespace {

void onVisible(JsonObject& root) {
    wsPayloadModule(root, "ota");
}

void sendResponse(AsyncWebServerRequest *request, int code, const String& payload = "") {
    auto *response = request->beginResponseStream("text/plain", 256);
    response->addHeader("Connection", "close");
    response->addHeader("X-XSS-Protection", "1; mode=block");
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("X-Frame-Options", "deny");

    response->setCode(code);

    if (payload.length()) {
        response->printf("%s", payload.c_str());
    } else {
        if (!Update.hasError()) {
            response->print("OK");
        } else {
            #if defined(ARDUINO_ESP8266_RELEASE_2_3_0)
                Update.printError(reinterpret_cast<Stream&>(response));
            #else
                Update.printError(*response);
            #endif
        }
    }

    request->send(response);
}

void setStatus(AsyncWebServerRequest *request, int code, const String& payload = "") {
    sendResponse(request, code, payload);
    request->_tempObject = malloc(sizeof(bool));
}

void onUpgrade(AsyncWebServerRequest *request) {
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getHostname().c_str());
    }

    if (request->_tempObject) {
        return;
    }

    sendResponse(request, 200);
}

void onFile(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(getHostname().c_str());
    }

    // We set this after we are done with the request
    // It is still possible to re-enter this callback even after connection is already closed
    // 1.15.0: TODO: see https://github.com/me-no-dev/ESPAsyncWebServer/pull/660
    // remote close or request sending some data before finishing parsing of the body will leak 1460 bytes
    // waiting a bit for upstream. looks more and more we need to fork the server
    if (request->_tempObject) {
        return;
    }

    if (!index) {
        // TODO: stop network activity completely when handling Update through ArduinoOTA or `ota` command?
        if (Update.isRunning()) {
            setStatus(request, 400, F("ERROR: Upgrade in progress"));
            return;
        }

        // Check that header is correct and there is more data before anything is written to the flash
        if (final || !len) {
            setStatus(request, 400, F("ERROR: Invalid request"));
            return;
        }

        if (!otaVerifyHeader(data, len)) {
            setStatus(request, 400, F("ERROR: No magic byte / invalid flash config"));
            return;
        }

        // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
        eepromRotate(false);

        DEBUG_MSG_P(PSTR("[UPGRADE] Start: %s\n"), filename.c_str());
        Update.runAsync(true);

        // Note: cannot use request->contentLength() for multipart/form-data
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
            setStatus(request, 500);
            eepromRotate(true);
            return;
        }
    }

    if (request->_tempObject) {
        return;
    }

    // Any error will cancel the update, but request may still be alive
    if (!Update.isRunning()) {
        return;
    }

    if (Update.write(data, len) != len) {
        setStatus(request, 500);
        Update.end();
        eepromRotate(true);
        return;
    }

    if (final) {
        otaFinalize(index + len, CustomResetReason::Ota, true);
    } else {
        otaProgress(index + len);
    }
}

} // namespace
} // namespace web
} // namespace ota

void otaWebSetup() {
    webServer().
        on("/upgrade", HTTP_POST, ota::web::onUpgrade, ota::web::onFile);
    wsRegister().
        onVisible(ota::web::onVisible);
}

#endif // OTA_WEB_SUPPORT

