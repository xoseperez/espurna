/*

Part of the OTA MODULE

Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if !WEB_SUPPORT && OTA_WEB_SUPPORT

// When there's no WEB_SUPPORT, enable a basic server with a form upload and /upgrade endpoint.
// Based on the async-web-server code and the ESP8266HTTPUpdateServer.h bundled with the Core
// effectively repeats what is done in the async variant, but does not involve async-web-server
// (...but, still suffers from a similar std::function API, forcing to self-reference the server object and manage various global state & result objects)
//

#include "ota.h"

#include <ESP8266WebServer.h>
#include <StreamString.h>

namespace espurna {
namespace ota {
namespace basic_web {
namespace {

const char HomePage[] PROGMEM = R"(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
</head>
<body>
<form method="POST" action="/upgrade" enctype="multipart/form-data">
<fieldset>
<legend>Firmware upgrade</legend>
<div>
<input type="file" name="filename">
</div>
<div>
<button type="submit">Upload</button>
</div>
</form>
</body>
</html>
)";

const char UpgradePageHead[] PROGMEM = R"(
<head>
<meta http-equiv="refresh" content="5; url=./">
</head>
)";

namespace handlers {
namespace internal {

struct Result {
    template <typename T>
    void set(int code, T&& value) {
        _code = code;
        _output = std::forward<T>(value);
    }

    void setFromUpdate() {
        if (Update.hasError()) {
            StreamString stream;
            Update.printError(stream);
            set(500, stream);
        }
    }

    template <typename Server>
    void send(Server& server) {
        String output;
        output.reserve(_output.length() + sizeof(UpgradePageHead) + 16);

        output.concat(UpgradePageHead, sizeof(UpgradePageHead) - 1);
        output += F("<code>");
        output += _output;
        output += F("</code>");

        server.send(_code, PSTR("text/html"), output);
    }

    void reset() {
        _code = 200;
        _output = "";
    }

private:
    int _code { 200 };
    String _output;
};

Result result;

} // namespace internal

template <typename Server>
void result(Server& server) {
    internal::result.send(server);
}

template <typename Server>
void upload(Server& server) {
    auto& upload = server.upload();

    switch (upload.status) {

    case UPLOAD_FILE_START: {
        if (Update.isRunning()) {
            server.client().stop();
            return;
        }

        internal::result.reset();
        const size_t Available { (ESP.getFreeSketchSpace() - 0x1000ul) & 0xfffff000ul };
        if (!Update.begin(Available, U_FLASH)) {
            server.client().stop();
            internal::result.set(500, F("Not enough available space"));
            eepromRotate(true);
        }
        break;
    }

    case UPLOAD_FILE_END:
        if (otaFinalize(upload.totalSize, CustomResetReason::Ota, true)) {
            internal::result.set(200, F("Rebooting..."));
        } else {
            internal::result.setFromUpdate();
        }
        break;

    case UPLOAD_FILE_WRITE:
        if (!Update.isRunning()) {
            return;
        }

        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            internal::result.set(500, F("Error during write()"));
            server.client().stop();
            Update.end();
            eepromRotate(true);
            return;
        }

        break;

    case UPLOAD_FILE_ABORTED:
        internal::result.set(500, F("Upload aborted"));
        Update.end();
        eepromRotate(true);
        break;

    }
}

template <typename Server>
void home(Server& server) {
    server.send_P(200, PSTR("text/html"), HomePage);
}

} // namespace handlers

template <typename Server>
void begin(Server& server, uint16_t port) {
    server.on("/", HTTP_GET, [&]() {
        handlers::home(server);
    });

    server.on("/upgrade", HTTP_POST,
        [&]() {
            handlers::result(server);
        },
        [&]() {
            handlers::upload(server);
        }
    );

    server.begin(port);
}

namespace build {

constexpr auto DefaultPort = uint16_t{
#if WEB_SSL_ENABLED
        443
#else
        80
#endif
};

constexpr uint16_t port() {
    return (WEB_PORT == 0)
        ? DefaultPort
        : (WEB_PORT);
}

} // namespace build

namespace settings {
namespace keys {

STRING_VIEW_INLINE(Port, "webPort");

} // namespace keys

uint16_t port() {
    return getSetting(keys::Port, build::port());
}

} // namespace settings

namespace internal {

ESP8266WebServer server;

} // namespace internal

void setup() {
    begin(internal::server, settings::port());
    ::espurnaRegisterLoop([]() {
        internal::server.handleClient();
    });
}

} // namespace
} // namespace basic_web
} // namespace ota
} // namespace espurna

void otaWebSetup() {
    espurna::ota::basic_web::setup();
}

#endif

