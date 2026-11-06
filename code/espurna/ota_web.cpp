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

#if defined(LIBRETINY)
#include "arch/beken/ota_beken.h"
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

#if defined(LIBRETINY)
// Beken uploads that ESPurna writes itself instead of Update: espurna-beken.bin, the image for every bootloader
// (arch/beken/ota_beken.cpp), and matter-tail.bin, the Matter code of the Tuya layout (arch/beken/matter_beken.cpp).
// Each function returns nullptr or a PSTR() error
struct Writer {
    const char* name;
    const char* (*begin)();
    const char* (*write)(const uint8_t*, size_t);
    const char* (*end)();
    void (*abort)();
};

const Writer BekenImage {"image", bekenImageBegin, bekenImageWrite, bekenImageEnd, bekenImageAbort};
#if MATTER_TAIL_SUPPORT
const Writer MatterTail {"Matter code", matterTailBegin, matterTailWrite, matterTailEnd, matterTailAbort};
#endif

const Writer* writer { nullptr };

void setWriterError(AsyncWebServerRequest* request, const char* error) {
    writer = nullptr;
    setStatus(request, 500, String(FPSTR(error)));
}
#endif

void onDisconnect(AsyncWebServerRequest* request) {
    if (owner != request) {
        return;
    }

    owner = nullptr;
#if defined(LIBRETINY)
    if (writer) {
        writer->abort();
        writer = nullptr;
        pending = false;
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

// The web server drops a connection after 3 s without received data (WebServer.cpp). While the image goes
// into the flash, the receive buffers can be full and the sender waits for its TCP retransmission timer,
// which grows past that: the upload stopped half-way (nginx answered 502 to the browser)
constexpr uint32_t UploadRxTimeout { 30 };

void uploadRxTimeout(AsyncWebServerRequest* request) {
    request->client()->setRxTimeout(UploadRxTimeout);
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

#if defined(LIBRETINY)
        if (writer) {
            const auto* current = writer;
            writer = nullptr;
            const auto* error = complete(request)
                ? current->end()
                : PSTR("ERROR: Incomplete upload");
            if (error) {
                current->abort();
                sendResponse(request, 400, String(FPSTR(error)));
                return;
            }

            // Tuya layout: the image is in, its Matter code comes with the same file after the reboot
            sendResponse(request, 200,
                ((current == &BekenImage) && bekenImageAgain()) ? F("OK, again") : F("OK"));
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

#if defined(LIBRETINY)
#if MATTER_TAIL_SUPPORT
        // Matter code is where the OTA image goes: one reboot without Matter first, then upload again
        if (matterTailBlocksOta()) {
            setStatus(request, 409, F("ERROR: Matter is running from the OTA partition. Rebooting without Matter, upload again in a minute"));
            systemRunInLoop([]() {
                matterRebootForUpload();
            });
            return;
        }
#endif

        writer = nullptr;
        if (bekenImageIsUpload(data, len)) {
            writer = &BekenImage;
#if MATTER_TAIL_SUPPORT
        } else if (matterTailIsUpload(data, len)) {
            writer = &MatterTail;
#endif
        }

        if (writer) {
            const auto* error = writer->begin();
            if (error) {
                setWriterError(request, error);
                return;
            }

            DEBUG_MSG_P(PSTR("[UPGRADE] Start (%s): %s\n"), writer->name, filename.c_str());
            owner = request;
            uploadRxTimeout(request);
            request->onDisconnect([request]() {
                onDisconnect(request);
            });
        } else {
            const auto* error = bekenUf2Allowed();
            if (error) {
                setStatus(request, 400, String(FPSTR(error)));
                return;
            }
        }
    }

    if (writer) {
        if (request->_tempObject) {
            return;
        }

        const auto* error = writer->write(data, len);
        if (error) {
            writer->abort();
            setWriterError(request, error);
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
        uploadRxTimeout(request);
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

#if defined(ARDUINO_ARCH_ESP8266)
// Install from GitHub (fwinstall.mjs): the board downloads the file itself, ota_fetch.cpp
bool authenticated(AsyncWebServerRequest* request) {
    if (!webAuthenticate(request)) {
        request->requestAuthentication(systemHostname().c_str());
        return false;
    }

    return true;
}

void onFetchStatus(AsyncWebServerRequest* request) {
    if (authenticated(request)) {
        request->send(200, "application/json", otaFetchStatus());
    }
}

String formValue(AsyncWebServerRequest* request, const __FlashStringHelper* name) {
    const auto* param = request->getParam(String(name), true);
    return param ? param->value() : String();
}

// url + sha256 (of firmware.json). "OK, fetch" - restarting, this image downloads it at boot; "OK, mini" - stored
// for the minimal image (1 MB boards), the browser uploads espurna-esp8266-1m-mini.bin.gz to /upgrade next
void onFetch(AsyncWebServerRequest* request) {
    if (!authenticated(request)) {
        return;
    }

    if (Update.isRunning()) {
        sendResponse(request, 409, F("ERROR: Upgrade in progress"));
        return;
    }

    String error;
    if (!otaFetchRequest(formValue(request, F("url")), formValue(request, F("sha256")), error)) {
        sendResponse(request, 400, String(F("ERROR: ")) + error);
        return;
    }

    if (otaFetchSelf()) {
        sendResponse(request, 200, F("OK, fetch"));
        prepareReset(CustomResetReason::Web);
        return;
    }

    sendResponse(request, 200, F("OK, mini"));
}
#endif

} // namespace
} // namespace web
} // namespace ota

void otaWebSetup() {
#if defined(ARDUINO_ARCH_ESP8266)
    webServer().on("/ota/status", HTTP_GET, ota::web::onFetchStatus);
    webServer().on("/ota/fetch", HTTP_POST, ota::web::onFetch);
#endif
    webServer().
        on("/upgrade", HTTP_POST, ota::web::onUpgrade, ota::web::onFile);
    wsRegister().
        onVisible(ota::web::onVisible);
}

#endif // OTA_WEB_SUPPORT


