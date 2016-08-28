/*

ESPurna
OTA MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <ArduinoJson.h>
#include "ArduinoOTA.h"

// -----------------------------------------------------------------------------
// OTA
// -----------------------------------------------------------------------------

void otaSetup() {

    ArduinoOTA.setPort(OTA_PORT);
    ArduinoOTA.setHostname(getSetting("hostname").c_str());
    ArduinoOTA.setPassword((const char *) OTA_PASS);

    ArduinoOTA.onStart([]() {
        #if ENABLE_RF
            rfEnable(false);
        #endif
        #if DEBUG
            Serial.println(F("[OTA] Start"));
        #endif
    });

    ArduinoOTA.onEnd([]() {
        #if DEBUG
            Serial.println(F("[OTA] End"));
        #endif
        #if ENABLE_RF
            rfEnable(true);
        #endif
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        #if DEBUG
            Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
        #endif
    });

    ArduinoOTA.onError([](ota_error_t error) {
        #if DEBUG
            Serial.printf("[OTA] Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println(F("[OTA] Auth Failed"));
            else if (error == OTA_BEGIN_ERROR) Serial.println(F("[OTA] Begin Failed"));
            else if (error == OTA_CONNECT_ERROR) Serial.println(F("[OTA] Connect Failed"));
            else if (error == OTA_RECEIVE_ERROR) Serial.println(F("[OTA] Receive Failed"));
            else if (error == OTA_END_ERROR) Serial.println(F("[OTA] End Failed"));
        #endif
    });

    ArduinoOTA.begin();

}

void otaLoop() {
    ArduinoOTA.handle();
}
