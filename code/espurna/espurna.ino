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
#include <EEPROM.h>

// -----------------------------------------------------------------------------
// METHODS
// -----------------------------------------------------------------------------

String getIdentifier() {
    char identifier[20];
    sprintf(identifier, "%s_%06X", DEVICE, ESP.getChipId());
    return String(identifier);
}

void hardwareSetup() {
    EEPROM.begin(4096);
    Serial.begin(SERIAL_BAUDRATE);
    #if not EMBEDDED_WEB
        SPIFFS.begin();
    #endif
}

void hardwareLoop() {

    static unsigned long last_uptime = 0;
    static unsigned char uptime_overflows = 0;

    // Heartbeat
    if ((millis() - last_uptime > HEARTBEAT_INTERVAL) || (last_uptime == 0)) {

        if (millis() < last_uptime) ++uptime_overflows;
        last_uptime = millis();
        unsigned long uptime_seconds = uptime_overflows * (UPTIME_OVERFLOW / 1000) + (last_uptime / 1000);

        DEBUG_MSG("[MAIN] Time: %s\n", (char *) NTP.getTimeDateString().c_str());
        DEBUG_MSG("[MAIN] Uptime: %ld seconds\n", uptime_seconds);
        DEBUG_MSG("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
        #if ENABLE_ADC_VCC
            DEBUG_MSG("[MAIN] Power: %d mV\n", ESP.getVcc());
        #endif

        #if (MQTT_REPORTS | MQTT_STATUS_REPORT)
            mqttSend(MQTT_STATUS_TOPIC, "1");
        #endif
        #if (MQTT_REPORTS | MQTT_IP_REPORT)
            mqttSend(MQTT_IP_TOPIC, getIP().c_str());
        #endif
        #if (MQTT_REPORTS | MQTT_UPTIME_REPORT)
            mqttSend(MQTT_UPTIME_TOPIC, String(uptime_seconds).c_str());
        #endif
        #if (MQTT_REPORTS | MQTT_FREEHEAP_REPORT)
            mqttSend(MQTT_FREEHEAP_TOPIC, String(ESP.getFreeHeap()).c_str());
        #endif
        #if (MQTT_REPORTS | MQTT_VCC_REPORT)
        #if ENABLE_ADC_VCC
            mqttSend(MQTT_VCC_TOPIC, String(ESP.getVcc()).c_str());
        #endif
        #endif

    }

}

// -----------------------------------------------------------------------------
// BOOTING
// -----------------------------------------------------------------------------

void welcome() {

    DEBUG_MSG("%s %s\n", (char *) APP_NAME, (char *) APP_VERSION);
    DEBUG_MSG("%s\n%s\n\n", (char *) APP_AUTHOR, (char *) APP_WEBSITE);
    DEBUG_MSG("ChipID: %06X\n", ESP.getChipId());
    DEBUG_MSG("CPU frequency: %d MHz\n", ESP.getCpuFreqMHz());
    DEBUG_MSG("Last reset reason: %s\n", (char *) ESP.getResetReason().c_str());
    DEBUG_MSG("Memory size: %d bytes\n", ESP.getFlashChipSize());
    DEBUG_MSG("Free heap: %d bytes\n", ESP.getFreeHeap());
    DEBUG_MSG("Firmware size: %d bytes\n", ESP.getSketchSize());
    DEBUG_MSG("Free firmware space: %d bytes\n", ESP.getFreeSketchSpace());

    #if not EMBEDDED_WEB
        FSInfo fs_info;
        if (SPIFFS.info(fs_info)) {
            DEBUG_MSG("File system total size: %d bytes\n", fs_info.totalBytes);
            DEBUG_MSG("            used size : %d bytes\n", fs_info.usedBytes);
            DEBUG_MSG("            block size: %d bytes\n", fs_info.blockSize);
            DEBUG_MSG("            page size : %d bytes\n", fs_info.pageSize);
            DEBUG_MSG("            max files : %d\n", fs_info.maxOpenFiles);
            DEBUG_MSG("            max length: %d\n", fs_info.maxPathLength);
        }
    #endif

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

    delay(500);

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
