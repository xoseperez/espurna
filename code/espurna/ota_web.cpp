/*

Part of the WEBSERVER module

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if WEB_SUPPORT && OTA_WEB_SUPPORT

#include "espurna_ota.h"
#include "web.h"
#include "ws.h"

#if MATTER_TAIL_SUPPORT
#include "espurna_matter.h"
#endif

namespace ota {
namespace web {
namespace {

STRING_VIEW_INLINE(Prefix, "ota");

void onVisible(JsonObject& root) {
    wsPayloadModule(root, Prefix);
}

void sendResponse(AsyncWebServerRequest *request, int code, const String& payload = "") {
    auto *response = request->beginResponseStream(F("text/plain"), 256);

    webSecurityHeaders(response);
    response->addHeader(
        STRING_VIEW("Connection").toString(),
        STRING_VIEW("close").toString());

    response->setCode(code);

    if (payload.length()) {
        response->print(payload);
    } else {
        if (!Update.hasError()) {
            response->print(F("OK"));
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

// Last chunk of the file arrives before the body is fully parsed. When some of the upload never
// reached the HTTP parser, the request does not fail. Instead, contentLength() is quietly lowered
// to what did arrive and the request finishes as usual. Only commit the update after parsing is
// done, and only when the body is the size the client said it is
size_t pending_size { 0 };
bool pending { false };

// Upload that started the running Update. When its connection is closed before the upload is
// finished (client gone, request timed out), nothing else ends the Update. It would then stay
// 'in progress' and every following upload fails with 400 until the reboot
AsyncWebServerRequest* owner { nullptr };

#if MATTER_TAIL_SUPPORT
// Upload is matter-tail.bin, the Matter code of this build, not a firmware image (ref. arch/beken/matter_beken.cpp)
bool matter_tail { false };

void setTailError(AsyncWebServerRequest* request, const char* error) {
    matter_tail = false;
    setStatus(request, 500, String(FPSTR(error)));
}
#endif

void onDisconnect(AsyncWebServerRequest* request) {
    if (owner != request) {
        return;
    }

    owner = nullptr;
#if MATTER_TAIL_SUPPORT
    if (matter_tail) {
        matter_tail = false;
        pending = false;
        matterTailAbort();
        DEBUG_MSG_P(PSTR("[UPGRADE] Connection closed before the upload finished, aborted\n"));
        return;
    }
#endif
    if (Update.isRunning()) {
        pending = false;
        Update.end();
        eepromRotate(true);
        DEBUG_MSG_P(PSTR("[UPGRADE] Connection closed before the upload finished, aborted\n"));
    }
}

bool complete(AsyncWebServerRequest *request) {
    const auto& header = request->header(F("Content-Length"));
    return header.length()
        && (static_cast<size_t>(header.toInt()) == request->contentLength());
}

void onUpgrade(AsyncWebServerRequest *request) {
    // not in onFile(), holding up AsyncTCP for every uploaded chunk only slows the upload down
    ::espurna::system::AsyncGuard guard;
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(systemHostname().c_str());
    }

    if (request->_tempObject) {
        return;
    }

    if (pending) {
        pending = false;
        owner = nullptr;

#if MATTER_TAIL_SUPPORT
        if (matter_tail) {
            matter_tail = false;
            const auto* error = complete(request)
                ? matterTailEnd()
                : PSTR("ERROR: Incomplete upload");
            if (error) {
                matterTailAbort();
                sendResponse(request, 400, String(FPSTR(error)));
                return;
            }

            sendResponse(request, 200);
            systemRunInLoop([]() {
                prepareReset(CustomResetReason::Ota);
            });
            return;
        }
#endif

        if (!complete(request)) {
            DEBUG_MSG_P(PSTR("[UPGRADE] Incomplete upload, received %u of %s bytes\n"),
                request->contentLength(), request->header(F("Content-Length")).c_str());
            Update.end();
            eepromRotate(true);
            sendResponse(request, 400, F("ERROR: Incomplete upload"));
            return;
        }

        if (guard.locked()) {
            otaFinalize(pending_size, CustomResetReason::Ota, true);
        } else {
            // Update belongs to this upload and is finished right here, so that the response
            // reports the actual result. What otaFinalize() does after that (reset, EEPROM rotation,
            // debug output) belongs to loop() and happens there. Never the case on esp8266.
            const auto size = pending_size;
            const auto success = Update.isRunning() && Update.end(true);
            systemRunInLoop([size, success]() {
                if (success) {
                    DEBUG_MSG_P(PSTR("[OTA] Success: %7u bytes\n"), size);
                    prepareReset(CustomResetReason::Ota);
                    return;
                }

                otaPrintError();
                eepromRotate(true);
            });
        }
    }

    sendResponse(request, 200);
}

void onFile(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!webAuthenticate(request)) {
        return request->requestAuthentication(systemHostname().c_str());
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
        pending = false;

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

#if MATTER_TAIL_SUPPORT
        // Matter code is where the OTA image goes: one reboot without Matter first, then upload again
        if (matterTailBlocksOta()) {
            setStatus(request, 409, F("ERROR: Matter is running from the OTA partition. Rebooting without Matter, upload again in a minute"));
            systemRunInLoop([]() {
                matterRebootForUpload();
            });
            return;
        }

        matter_tail = matterTailIsUpload(data, len);
        if (matter_tail) {
            const auto* error = matterTailBegin();
            if (error) {
                setTailError(request, error);
                return;
            }

            DEBUG_MSG_P(PSTR("[UPGRADE] Matter code: %s\n"), filename.c_str());
            owner = request;
            request->onDisconnect([request]() {
                onDisconnect(request);
            });
        }
    }

    if (matter_tail) {
        if (request->_tempObject) {
            return;
        }

        const auto* error = matterTailWrite(data, len);
        if (error) {
            setTailError(request, error);
            return;
        }

        if (final) {
            pending_size = index + len;
            pending = true;
        } else {
            otaProgress(index + len);
        }

        return;
    }

    if (!index) {
#endif

        if (!otaVerifyHeader(data, len)) {
            setStatus(request, 400, F("ERROR: No magic byte / invalid flash config"));
            return;
        }

        // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
        eepromRotate(false);

        DEBUG_MSG_P(PSTR("[UPGRADE] Start: %s\n"), filename.c_str());
#if defined(ESP8266)
        Update.runAsync(true);
#endif

        // Note: cannot use request->contentLength() for multipart/form-data
#if defined(LIBRETINY)
        // .uf2 package is a lot bigger than the image it carries, Update checks the OTA partition itself
        const size_t size = UPDATE_SIZE_UNKNOWN;
#else
        const size_t size = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
#endif
        if (!Update.begin(size)) {
            setStatus(request, 500);
            eepromRotate(true);
            return;
        }

        owner = request;
        request->onDisconnect([request]() {
            onDisconnect(request);
        });
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
        // committed in onUpgrade(), see above
        pending_size = index + len;
        pending = true;
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


