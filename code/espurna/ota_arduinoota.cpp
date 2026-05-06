/*

ARDUINO OTA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if OTA_ARDUINOOTA_SUPPORT

#include "ota.h"
#include "system.h"
#include "ws.h"

#include <ArduinoOTA.h>

namespace espurna {
namespace ota {
namespace arduino {
namespace {

// TODO: Allocate ArduinoOTAClass on-demand, stop using global instance
// TODO: ArduinoOTAClass and MDNS are tightly coupled together, consider creating a MDNS-less version for internal use?
// TODO: Merge Updater as well, to provide a way to handle semi-arbitrary flash locations as partitions?
void configure() {
    ArduinoOTA.setPort(OTA_PORT);
#if USE_PASSWORD
    ArduinoOTA.setPassword(systemPassword().c_str());
#endif
#if defined(ESP8266)
    ArduinoOTA.begin(false);
#else
    ArduinoOTA.begin();
#endif
}

void loop() {
    ArduinoOTA.handle();
}

void start() {
    // Disabling EEPROM rotation to prevent writing to EEPROM after the upgrade
    // Because ArduinoOTA is synchronous and will block until either success or error, force backup right now instead of waiting for the next loop()
    eepromRotate(false);
    eepromBackup(0);

    DEBUG_MSG_P(PSTR("[OTA] Started...\n"));
#if WEB_SUPPORT
    wsSend([](JsonObject& root) {
        root[F("message")] = F("OTA update started.");
    });
#endif
}

void end() {
    // ArduinoOTA default behaviour is to reset the board after this callback returns.
    // Page reload will happen automatically, when WebUI will fail to receive the PING response.
    DEBUG_MSG_P(PSTR("[OTA] Done, restarting.\n"));
    customResetReason(CustomResetReason::Ota);
    espurna::time::blockingDelay(
        espurna::duration::Milliseconds(100));
}

void progress(unsigned int progress, unsigned int total) {
    // Removed to avoid websocket ping back during upgrade (see #1574)
    // TODO: implement as a custom payload that reports progress in non-text form?
#if WEB_SUPPORT
    if (wsConnected()) {
        return;
    }
#endif

#if DEBUG_SUPPORT
    static unsigned int last;
    unsigned int current = (progress / (total / 100));
    if (current != last) {
        DEBUG_MSG_P(PSTR("[OTA] Progress: %u%%\r"), current);
        last = current;
    }
#endif
}

void error(ota_error_t error) {
#if DEBUG_SUPPORT
    StringView reason = STRING_VIEW("Unknown");

    switch (error) {
    case OTA_AUTH_ERROR:
        reason = STRING_VIEW("Authentication");
        break;
    case OTA_BEGIN_ERROR:
        reason = STRING_VIEW("Begin");
        break;
    case OTA_CONNECT_ERROR:
        reason = STRING_VIEW("Connection");
        break;
    case OTA_RECEIVE_ERROR:
        reason = STRING_VIEW("Receive");
        break;
    case OTA_END_ERROR:
        reason = STRING_VIEW("End");
        break;
#if defined(ESP8266) && !defined(ARDUINO_ESP8266_RELEASE_2_7_2) \
&& !defined(ARDUINO_ESP8266_RELEASE_2_7_3) \
&& !defined(ARDUINO_ESP8266_RELEASE_2_7_4) \
&& !defined(ARDUINO_ESP8266_RELEASE_3_0_0) \
&& !defined(ARDUINO_ESP8266_RELEASE_3_0_1) \
&& !defined(ARDUINO_ESP8266_RELEASE_3_0_2) \
&& !defined(ARDUINO_ESP8266_RELEASE_3_1_0) \
&& !defined(ARDUINO_ESP8266_RELEASE_3_1_1) \
&& !defined(ARDUINO_ESP8266_RELEASE_3_1_2)
    case OTA_ERASE_SETTINGS_ERROR:
        reason = STRING_VIEW("Settings erase");
        break;
#endif
    }

    DEBUG_MSG_P(PSTR("[OTA] OTA Stopped: %.*s\n"),
        reason.length(), reason.data());
    otaPrintError();
#endif

    eepromRotate(true);
}

void setup() {
    espurnaRegisterLoop(loop);
    espurnaRegisterReload(configure);

    ArduinoOTA.onStart(start);
    ArduinoOTA.onEnd(end);
    ArduinoOTA.onError(error);
    ArduinoOTA.onProgress(progress);

    configure();
}

} // namespace
} // namespace arduino
} // namespace ota
} // namespace espurna

void otaArduinoSetup() {
    espurna::ota::arduino::setup();
}

#endif // OTA_ARDUINOOTA_SUPPORT
