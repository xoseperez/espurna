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

void heartbeat() {

    static unsigned long last_uptime = 0;
    static unsigned char uptime_overflows = 0;

    if (millis() < last_uptime) ++uptime_overflows;
    last_uptime = millis();
    unsigned long uptime_seconds = uptime_overflows * (UPTIME_OVERFLOW / 1000) + (last_uptime / 1000);
    unsigned int free_heap = ESP.getFreeHeap();

    DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), (char *) NTP.getTimeDateString().c_str());
    if (!mqttConnected()) {
        DEBUG_MSG_P(PSTR("[MAIN] Uptime: %ld seconds\n"), uptime_seconds);
        DEBUG_MSG_P(PSTR("[MAIN] Free heap: %d bytes\n"), free_heap);
        #if ENABLE_ADC_VCC
            DEBUG_MSG_P(PSTR("[MAIN] Power: %d mV\n"), ESP.getVcc());
        #endif
    }

    #if (MQTT_REPORT_INTERVAL)
        mqttSend(MQTT_TOPIC_INTERVAL, HEARTBEAT_INTERVAL / 1000);
    #endif
    #if (MQTT_REPORT_APP)
        mqttSend(MQTT_TOPIC_APP, APP_NAME);
    #endif
    #if (MQTT_REPORT_VERSION)
        mqttSend(MQTT_TOPIC_VERSION, APP_VERSION);
    #endif
    #if (MQTT_REPORT_HOSTNAME)
        mqttSend(MQTT_TOPIC_HOSTNAME, getSetting("hostname").c_str());
    #endif
    #if (MQTT_REPORT_IP)
        mqttSend(MQTT_TOPIC_IP, getIP().c_str());
    #endif
    #if (MQTT_REPORT_MAC)
        mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str());
    #endif
    #if (MQTT_REPORT_RSSI)
        mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());
    #endif
    #if (MQTT_REPORT_UPTIME)
        mqttSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());
    #endif
    #if (MQTT_REPORT_FREEHEAP)
        mqttSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());
    #endif
    #if (MQTT_REPORT_RELAY)
        relayMQTT();
    #endif
    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    #if (MQTT_REPORT_COLOR)
        mqttSend(MQTT_TOPIC_COLOR, lightColor().c_str());
    #endif
    #endif
    #if (MQTT_REPORT_VCC)
    #if ENABLE_ADC_VCC
        mqttSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());
    #endif
    #endif
    #if (MQTT_REPORT_STATUS)
        mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE);
    #endif

}

void hardwareSetup() {
    EEPROM.begin(4096);
    #ifdef DEBUG_PORT
        DEBUG_PORT.begin(SERIAL_BAUDRATE);
    #endif
    #if SONOFF_DUAL
        Serial.begin(SERIAL_BAUDRATE);
    #endif
    #if not EMBEDDED_WEB
        SPIFFS.begin();
    #endif
}

void hardwareLoop() {

    // Heartbeat
    static unsigned long last_uptime = 0;
    if ((millis() - last_uptime > HEARTBEAT_INTERVAL) || (last_uptime == 0)) {
        last_uptime = millis();
        heartbeat();
    }

}

// -----------------------------------------------------------------------------
// BOOTING
// -----------------------------------------------------------------------------

void welcome() {

    DEBUG_MSG_P(PSTR("%s %s\n"), (char *) APP_NAME, (char *) APP_VERSION);
    DEBUG_MSG_P(PSTR("%s\n%s\n\n"), (char *) APP_AUTHOR, (char *) APP_WEBSITE);
    DEBUG_MSG_P(PSTR("ChipID: %06X\n"), ESP.getChipId());
    DEBUG_MSG_P(PSTR("CPU frequency: %d MHz\n"), ESP.getCpuFreqMHz());
    DEBUG_MSG_P(PSTR("Last reset reason: %s\n"), (char *) ESP.getResetReason().c_str());
    DEBUG_MSG_P(PSTR("Memory size (SDK): %d bytes\n"), ESP.getFlashChipSize());
    DEBUG_MSG_P(PSTR("Memory size (CHIP): %d bytes\n"), ESP.getFlashChipRealSize());
    DEBUG_MSG_P(PSTR("Free heap: %d bytes\n"), ESP.getFreeHeap());
    DEBUG_MSG_P(PSTR("Firmware size: %d bytes\n"), ESP.getSketchSize());
    DEBUG_MSG_P(PSTR("Free firmware space: %d bytes\n"), ESP.getFreeSketchSpace());

    #if not EMBEDDED_WEB
        FSInfo fs_info;
        if (SPIFFS.info(fs_info)) {
            DEBUG_MSG_P(PSTR("File system total size: %d bytes\n"), fs_info.totalBytes);
            DEBUG_MSG_P(PSTR("            used size : %d bytes\n"), fs_info.usedBytes);
            DEBUG_MSG_P(PSTR("            block size: %d bytes\n"), fs_info.blockSize);
            DEBUG_MSG_P(PSTR("            page size : %d bytes\n"), fs_info.pageSize);
            DEBUG_MSG_P(PSTR("            max files : %d\n"), fs_info.maxOpenFiles);
            DEBUG_MSG_P(PSTR("            max length: %d\n"), fs_info.maxPathLength);
        }
    #endif

    DEBUG_MSG_P(PSTR("\n\n"));

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
    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightSetup();
    #endif
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
    #if ENABLE_ANALOG
        analogSetup();
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

    // Prepare configuration for version 2.0
    hwUpwardsCompatibility();

}

void loop() {

    hardwareLoop();
    buttonLoop();
    relayLoop();
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
    #if ENABLE_ANALOG
        analogLoop();
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
