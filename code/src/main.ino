/*

ESPurna
Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

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

#include <Arduino.h>
#include "config/all.h"

// -----------------------------------------------------------------------------
// PROTOTYPES
// -----------------------------------------------------------------------------

#include <NtpClientLib.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include "FS.h"
String getSetting(const String& key, String defaultValue = "");

// -----------------------------------------------------------------------------
// METHODS
// -----------------------------------------------------------------------------

String getIdentifier() {
    char identifier[20];
    sprintf(identifier, "%s_%06X", DEVICE, ESP.getChipId());
    return String(identifier);
}

void blink(unsigned long delayOff, unsigned long delayOn) {
    static unsigned long next = millis();
    static bool status = HIGH;
    if (next < millis()) {
        status = !status;
        digitalWrite(LED_PIN, status);
        next += ((status) ? delayOff : delayOn);
    }
}

void showStatus() {
    if (wifiConnected()) {
        if (WiFi.getMode() == WIFI_AP) {
            blink(2000, 2000);
        } else {
            blink(5000, 500);
        }
    } else {
        blink(500, 500);
    }
}

void hardwareSetup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    SPIFFS.begin();
}

void getFSVersion(char * buffer) {
    File h = SPIFFS.open(FS_VERSION_FILE, "r");
    if (!h) {
        DEBUG_MSG("[SPIFFS] Could not open file system version file.\n");
        strcpy(buffer, APP_VERSION);
        return;
    }
    size_t size = h.size();
    h.readBytes(buffer, size - 1);
    h.close();
}

void hardwareLoop() {

    showStatus();

    // Heartbeat
    static unsigned long last_heartbeat = 0;
    if (mqttConnected()) {
        if ((millis() - last_heartbeat > HEARTBEAT_INTERVAL) || (last_heartbeat == 0)) {
            last_heartbeat = millis();
            mqttSend((char *) MQTT_HEARTBEAT_TOPIC, (char *) "1");
            DEBUG_MSG("[BEAT] Free heap: %d\n", ESP.getFreeHeap());
            DEBUG_MSG("[NTP] Time: %s\n", (char *) NTP.getTimeDateString().c_str());
        }
    }

}

// -----------------------------------------------------------------------------
// BOOTING
// -----------------------------------------------------------------------------

void welcome() {

    delay(2000);
    Serial.printf("%s %s\n", (char *) APP_NAME, (char *) APP_VERSION);
    Serial.printf("%s\n%s\n\n", (char *) APP_AUTHOR, (char *) APP_WEBSITE);
    //Serial.printf("Device: %s\n", (char *) getIdentifier().c_str());
    Serial.printf("ChipID: %06X\n", ESP.getChipId());
    Serial.printf("Last reset reason: %s\n", (char *) ESP.getResetReason().c_str());
    Serial.printf("Memory size: %d bytes\n", ESP.getFlashChipSize());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    FSInfo fs_info;
    if (SPIFFS.info(fs_info)) {
        Serial.printf("File system total size: %d bytes\n", fs_info.totalBytes);
        Serial.printf("            used size : %d bytes\n", fs_info.usedBytes);
        Serial.printf("            block size: %d bytes\n", fs_info.blockSize);
        Serial.printf("            page size : %d bytes\n", fs_info.pageSize);
        Serial.printf("            max files : %d\n", fs_info.maxOpenFiles);
        Serial.printf("            max length: %d\n", fs_info.maxPathLength);
    }
    Serial.println();
    Serial.println();

}

void setup() {

    hardwareSetup();
    buttonSetup();

    welcome();

    settingsSetup();
    if (getSetting("hostname").length() == 0) {
        setSetting("hostname", String() + getIdentifier());
        saveSettings();
    }

    relaySetup();
    wifiSetup();
    otaSetup();
    mqttSetup();
    webSetup();
    ntpSetup();

    #if ENABLE_FAUXMO
        fauxmoSetup();
    #endif
    #if ENABLE_NOFUSS
        nofussSetup();
    #endif
    #if ENABLE_POW
        powSetup();
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
    settingsLoop();
    wifiLoop();
    otaLoop();
    mqttLoop();
    ntpLoop();

    #if ENABLE_NOFUSS
        nofussLoop();
    #endif
    #if ENABLE_POW
        powLoop();
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

    delay(1);

}
