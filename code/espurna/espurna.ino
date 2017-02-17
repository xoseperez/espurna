/*

ESPurna

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "config/all.h"

// -----------------------------------------------------------------------------
// GLOBALS
// -----------------------------------------------------------------------------

char apibuffer[64];

// -----------------------------------------------------------------------------
// METHODS
// -----------------------------------------------------------------------------

String getIdentifier() {
    char identifier[20];
    sprintf(identifier, "%s_%06X", DEVICE, ESP.getChipId());
    return String(identifier);
}

void hardwareSetup() {
    Serial.begin(SERIAL_BAUDRATE);
    SPIFFS.begin();
}

void hardwareLoop() {

    // Heartbeat
    static unsigned long last_heartbeat = 0;
    if ((millis() - last_heartbeat > HEARTBEAT_INTERVAL) || (last_heartbeat == 0)) {
        last_heartbeat = millis();
        mqttSend(MQTT_HEARTBEAT_TOPIC, "1");
        DEBUG_MSG("[BEAT] Free heap: %d\n", ESP.getFreeHeap());
        DEBUG_MSG("[NTP] Time: %s\n", (char *) NTP.getTimeDateString().c_str());
    }

}

// -----------------------------------------------------------------------------
// BOOTING
// -----------------------------------------------------------------------------

void welcome() {

    delay(2000);
    DEBUG_MSG("%s %s\n", (char *) APP_NAME, (char *) APP_VERSION);
    DEBUG_MSG("%s\n%s\n\n", (char *) APP_AUTHOR, (char *) APP_WEBSITE);
    //DEBUG_MSG("Device: %s\n", (char *) getIdentifier().c_str());
    DEBUG_MSG("ChipID: %06X\n", ESP.getChipId());
    DEBUG_MSG("CPU frequency: %d MHz\n", ESP.getCpuFreqMHz());
    DEBUG_MSG("Last reset reason: %s\n", (char *) ESP.getResetReason().c_str());
    DEBUG_MSG("Memory size: %d bytes\n", ESP.getFlashChipSize());
    DEBUG_MSG("Free heap: %d bytes\n", ESP.getFreeHeap());
    DEBUG_MSG("Firmware size: %d bytes\n", ESP.getSketchSize());
    DEBUG_MSG("Free firmware space: %d bytes\n", ESP.getFreeSketchSpace());
    FSInfo fs_info;
    if (SPIFFS.info(fs_info)) {
        DEBUG_MSG("File system total size: %d bytes\n", fs_info.totalBytes);
        DEBUG_MSG("            used size : %d bytes\n", fs_info.usedBytes);
        DEBUG_MSG("            block size: %d bytes\n", fs_info.blockSize);
        DEBUG_MSG("            page size : %d bytes\n", fs_info.pageSize);
        DEBUG_MSG("            max files : %d\n", fs_info.maxOpenFiles);
        DEBUG_MSG("            max length: %d\n", fs_info.maxPathLength);
    }
    DEBUG_MSG("\n\n");

}

void setup() {

    hardwareSetup();
    welcome();

    settingsSetup();
    if (getSetting("hostname").length() == 0) {
        setSetting("hostname", getIdentifier());
        saveSettings();
    }

    webSetup();
    relaySetup();
    buttonSetup();
    ledSetup();
    wifiSetup();
    otaSetup();
    mqttSetup();
    ntpSetup();

    #if ENABLE_I2C
        i2cSetup();
    #endif
    #if ENABLE_FAUXMO
        fauxmoSetup();
    #endif
    #if ENABLE_NOFUSS
        nofussSetup();
    #endif
    #if ENABLE_POW
        powSetup();
    #endif
    #if ENABLE_DS18B20
        dsSetup();
    #endif
    #if ENABLE_DHT
        dhtSetup();
    #endif
    #if ENABLE_RF
        rfSetup();
    #endif
    #if ENABLE_EMON
        powerMonitorSetup();
    #endif

}

void loop() {

    hardwareLoop();
    buttonLoop();
    ledLoop();
    wifiLoop();
    otaLoop();
    mqttLoop();
    ntpLoop();

    #if ENABLE_FAUXMO
        fauxmoLoop();
    #endif
    #ifndef SONOFF_DUAL
        settingsLoop();
    #endif
    #if ENABLE_NOFUSS
        nofussLoop();
    #endif
    #if ENABLE_POW
        powLoop();
    #endif
    #if ENABLE_DS18B20
        dsLoop();
    #endif
    #if ENABLE_DHT
        dhtLoop();
    #endif
    #if ENABLE_RF
        rfLoop();
    #endif
    #if ENABLE_EMON
        powerMonitorLoop();
    #endif

}
