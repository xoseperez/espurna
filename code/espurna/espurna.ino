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
    sprintf_P(identifier, PSTR("%s_%06X"), DEVICE, ESP.getChipId());
    return String(identifier);
}

void heartbeat() {

    static unsigned long last_uptime = 0;
    static unsigned char uptime_overflows = 0;

    if (millis() < last_uptime) ++uptime_overflows;
    last_uptime = millis();
    unsigned long uptime_seconds = uptime_overflows * (UPTIME_OVERFLOW / 1000) + (last_uptime / 1000);
    unsigned int free_heap = ESP.getFreeHeap();

    DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), (char *) ntpDateTime().c_str());
    if (!mqttConnected()) {
        DEBUG_MSG_P(PSTR("[MAIN] Uptime: %ld seconds\n"), uptime_seconds);
        DEBUG_MSG_P(PSTR("[MAIN] Free heap: %d bytes\n"), free_heap);
        #if ADC_VCC_ENABLED
            DEBUG_MSG_P(PSTR("[MAIN] Power: %d mV\n"), ESP.getVcc());
        #endif
    }


    #if (HEARTBEAT_REPORT_INTERVAL)
        mqttSend(MQTT_TOPIC_INTERVAL, HEARTBEAT_INTERVAL / 1000);
    #endif
    #if (HEARTBEAT_REPORT_APP)
        mqttSend(MQTT_TOPIC_APP, APP_NAME);
    #endif
    #if (HEARTBEAT_REPORT_VERSION)
        mqttSend(MQTT_TOPIC_VERSION, APP_VERSION);
    #endif
    #if (HEARTBEAT_REPORT_HOSTNAME)
        mqttSend(MQTT_TOPIC_HOSTNAME, getSetting("hostname").c_str());
    #endif
    #if (HEARTBEAT_REPORT_IP)
        mqttSend(MQTT_TOPIC_IP, getIP().c_str());
    #endif
    #if (HEARTBEAT_REPORT_MAC)
        mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str());
    #endif
    #if (HEARTBEAT_REPORT_RSSI)
        mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());
    #endif
    #if (HEARTBEAT_REPORT_UPTIME)
        mqttSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());
        #if INFLUXDB_SUPPORT
        influxDBSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());
        #endif
    #endif
    #if (HEARTBEAT_REPORT_FREEHEAP)
        mqttSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());
        #if INFLUXDB_SUPPORT
        influxDBSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());
        #endif
    #endif
    #if (HEARTBEAT_REPORT_RELAY)
        relayMQTT();
    #endif
    #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE) & (HEARTBEAT_REPORT_LIGHT)
        lightMQTT();
    #endif
    #if (HEARTBEAT_REPORT_VCC)
    #if ADC_VCC_ENABLED
        mqttSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());
    #endif
    #endif
    #if (HEARTBEAT_REPORT_STATUS)
        mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);
    #endif

}

void customReset(unsigned char status) {
    EEPROM.write(EEPROM_CUSTOM_RESET, status);
    EEPROM.commit();
}

unsigned char customReset() {
    static unsigned char status = 255;
    if (status == 255) {
        status = EEPROM.read(EEPROM_CUSTOM_RESET);
        if (status > 0) customReset(0);
        if (status > CUSTOM_RESET_MAX) status = 0;
    }
    return status;
}

void hardwareSetup() {

    EEPROM.begin(EEPROM_SIZE);

    #if DEBUG_SERIAL_SUPPORT
        DEBUG_PORT.begin(SERIAL_BAUDRATE);
        if (customReset() == CUSTOM_RESET_HARDWARE) {
            DEBUG_PORT.setDebugOutput(true);
        }
    #elif defined(SERIAL_BAUDRATE)
        Serial.begin(SERIAL_BAUDRATE);
    #endif

    #if SPIFFS_SUPPORT
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

unsigned int sectors(size_t size) {
    return (int) (size + SPI_FLASH_SEC_SIZE - 1) / SPI_FLASH_SEC_SIZE;
}

void welcome() {

    DEBUG_MSG_P(PSTR("\n\n"));
    DEBUG_MSG_P(PSTR("%s %s\n"), (char *) APP_NAME, (char *) APP_VERSION);
    DEBUG_MSG_P(PSTR("%s\n%s\n\n"), (char *) APP_AUTHOR, (char *) APP_WEBSITE);
    DEBUG_MSG_P(PSTR("CPU chip ID: 0x%06X\n"), ESP.getChipId());
    DEBUG_MSG_P(PSTR("CPU frequency: %d MHz\n"), ESP.getCpuFreqMHz());
    DEBUG_MSG_P(PSTR("SDK version: %s\n"), ESP.getSdkVersion());
    DEBUG_MSG_P(PSTR("Core version: %s\n"), ESP.getCoreVersion().c_str());

    DEBUG_MSG_P(PSTR("\n"));
    FlashMode_t mode = ESP.getFlashChipMode();
    DEBUG_MSG_P(PSTR("Flash chip ID: 0x%06X\n"), ESP.getFlashChipId());
    DEBUG_MSG_P(PSTR("Flash speed: %u Hz\n"), ESP.getFlashChipSpeed());
    DEBUG_MSG_P(PSTR("Flash mode: %s\n"), mode == FM_QIO ? "QIO" : mode == FM_QOUT ? "QOUT" : mode == FM_DIO ? "DIO" : mode == FM_DOUT ? "DOUT" : "UNKNOWN");
    DEBUG_MSG_P(PSTR("\n"));
    DEBUG_MSG_P(PSTR("Flash sector size: %8u bytes\n"), SPI_FLASH_SEC_SIZE);
    DEBUG_MSG_P(PSTR("Flash size (SDK):  %8u bytes\n"), ESP.getFlashChipSize());
    DEBUG_MSG_P(PSTR("Flash size (CHIP): %8u bytes / %4d sectors\n"), ESP.getFlashChipRealSize(), sectors(ESP.getFlashChipRealSize()));
    DEBUG_MSG_P(PSTR("Firmware size:     %8u bytes / %4d sectors\n"), ESP.getSketchSize(), sectors(ESP.getSketchSize()));
    DEBUG_MSG_P(PSTR("OTA size:          %8u bytes / %4d sectors\n"), ESP.getFreeSketchSpace(), sectors(ESP.getFreeSketchSpace()));
    #if SPIFFS_SUPPORT
        FSInfo fs_info;
        bool fs = SPIFFS.info(fs_info);
        if (fs) {
            DEBUG_MSG_P(PSTR("SPIFFS size:       %8u bytes / %4d sectors\n"), fs_info.totalBytes, sectors(fs_info.totalBytes));
        }
    #else
        DEBUG_MSG_P(PSTR("SPIFFS size:       %8u bytes / %4d sectors\n"), 0, 0);
    #endif
    DEBUG_MSG_P(PSTR("EEPROM size:       %8u bytes / %4d sectors\n"), settingsMaxSize(), sectors(settingsMaxSize()));
    DEBUG_MSG_P(PSTR("Empty space:       %8u bytes /    4 sectors\n"), 4 * SPI_FLASH_SEC_SIZE);

    #if SPIFFS_SUPPORT
        if (fs) {
            DEBUG_MSG_P(PSTR("\n"));
            DEBUG_MSG_P(PSTR("SPIFFS total size: %8u bytes\n"), fs_info.totalBytes);
            DEBUG_MSG_P(PSTR("       used size:  %8u bytes\n"), fs_info.usedBytes);
            DEBUG_MSG_P(PSTR("       block size: %8u bytes\n"), fs_info.blockSize);
            DEBUG_MSG_P(PSTR("       page size:  %8u bytes\n"), fs_info.pageSize);
            DEBUG_MSG_P(PSTR("       max files:  %8u\n"), fs_info.maxOpenFiles);
            DEBUG_MSG_P(PSTR("       max length: %8u\n"), fs_info.maxPathLength);
        }
    #endif

    DEBUG_MSG_P(PSTR("\n"));
    unsigned char custom_reset = customReset();
    if (custom_reset > 0) {
        char buffer[32];
        strcpy_P(buffer, custom_reset_string[custom_reset-1]);
        DEBUG_MSG_P(PSTR("Last reset reason: %s\n"), buffer);
    } else {
        DEBUG_MSG_P(PSTR("Last reset reason: %s\n"), (char *) ESP.getResetReason().c_str());
    }
    DEBUG_MSG_P(PSTR("Free heap: %u bytes\n"), ESP.getFreeHeap());

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

    #if WEB_SUPPORT
        webSetup();
    #endif
    
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

    #ifdef ITEAD_SONOFF_RFBRIDGE
        rfbSetup();
    #endif

    #if I2C_SUPPORT
        i2cSetup();
    #endif
    #if ALEXA_SUPPORT
        alexaSetup();
    #endif
    #if NOFUSS_SUPPORT
        nofussSetup();
    #endif
    #if INFLUXDB_SUPPORT
        influxDBSetup();
    #endif
    #if HLW8012_SUPPORT
        hlw8012Setup();
    #endif
    #if DS18B20_SUPPORT
        dsSetup();
    #endif
    #if ANALOG_SUPPORT
        analogSetup();
    #endif
    #if DHT_SUPPORT
        dhtSetup();
    #endif
    #if RF_SUPPORT
        rfSetup();
    #endif
    #if EMON_SUPPORT
        powerMonitorSetup();
    #endif
    #if DOMOTICZ_SUPPORT
        domoticzSetup();
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

    #ifdef ITEAD_SONOFF_RFBRIDGE
        rfbLoop();
    #endif

    #if TERMINAL_SUPPORT
        settingsLoop();
    #endif
    #if ALEXA_SUPPORT
        alexaLoop();
    #endif
    #if NOFUSS_SUPPORT
        nofussLoop();
    #endif
    #if HLW8012_SUPPORT
        hlw8012Loop();
    #endif
    #if DS18B20_SUPPORT
        dsLoop();
    #endif
    #if ANALOG_SUPPORT
        analogLoop();
    #endif
    #if DHT_SUPPORT
        dhtLoop();
    #endif
    #if RF_SUPPORT
        rfLoop();
    #endif
    #if EMON_SUPPORT
        powerMonitorLoop();
    #endif

}
