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

extern "C" {
    #include "user_interface.h"
}

#include <Arduino.h>
#include "version.h"
#include "defaults.h"
#include "FS.h"
#include "Config.h"

// -----------------------------------------------------------------------------
// Methods
// -----------------------------------------------------------------------------

void getCompileTime(char * buffer) {

    int day, month, year, hour, minute, second;

    // parse date
    String tmp = String(__DATE__);
    day = tmp.substring(4,6).toInt();
    year = tmp.substring(7).toInt();
    tmp = tmp.substring(0,3);
    if (tmp.equals("Jan")) month = 1;
    if (tmp.equals("Feb")) month = 2;
    if (tmp.equals("Mar")) month = 3;
    if (tmp.equals("Apr")) month = 4;
    if (tmp.equals("May")) month = 5;
    if (tmp.equals("Jun")) month = 6;
    if (tmp.equals("Jul")) month = 7;
    if (tmp.equals("Aug")) month = 8;
    if (tmp.equals("Sep")) month = 9;
    if (tmp.equals("Oct")) month = 10;
    if (tmp.equals("Nov")) month = 11;
    if (tmp.equals("Dec")) month = 12;

    // parse time
    tmp = String(__TIME__);
    hour = tmp.substring(0,2).toInt();
    minute = tmp.substring(3,5).toInt();
    second = tmp.substring(6,8).toInt();

    sprintf(buffer, "%d%02d%02d%02d%02d%02d", year, month, day, hour, minute, second);
    buffer[14] = 0;

}

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
        blink(5000, 500);
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
        #ifdef DEBUG
            Serial.println(F("[SPIFFS] Could not open file system version file."));
        #endif
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
    if (millis() - last_heartbeat > HEARTBEAT_INTERVAL) {
        last_heartbeat = millis();
        mqttSend((char *) MQTT_HEARTBEAT_TOPIC, (char *) "1");
        #ifdef DEBUG
            Serial.print(F("[BEAT] Free heap: "));
            Serial.println(ESP.getFreeHeap());
        #endif
    }

}

// -----------------------------------------------------------------------------
// Booting
// -----------------------------------------------------------------------------

void welcome() {
    char buffer[BUFFER_SIZE];
    getCompileTime(buffer);
    Serial.println();
    Serial.println();
    Serial.print(APP_NAME);
    Serial.print(F(" "));
    Serial.print(APP_VERSION);
    Serial.print(F(" built "));
    Serial.println(buffer);
    Serial.println(APP_AUTHOR);
    Serial.println(APP_WEBSITE);
    Serial.println();
    Serial.print(F("Device: "));
    Serial.println(getIdentifier());
    Serial.print(F("Last reset reason: "));
    Serial.println(ESP.getResetReason());
    Serial.print(F("Memory size: "));
    Serial.print(ESP.getFlashChipSize());
    Serial.println(F(" bytes"));
    Serial.print(F("Free heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
    FSInfo fs_info;
    if (SPIFFS.info(fs_info)) {
        Serial.print(F("File system total size: "));
        Serial.print(fs_info.totalBytes);
        Serial.println(F(" bytes"));
        Serial.print(F("File system used size : "));
        Serial.print(fs_info.usedBytes);
        Serial.println(F(" bytes"));
    }

    Serial.println();
}

void setup() {

    hardwareSetup();
    relaySetup();
    buttonSetup();
    delay(1000);
    welcome();
    config.load();

    // At the moment I am overriding any possible hostname stored in EEPROM
    // with the generated one until I have a way to change them from the
    // configuration interface
    config.hostname = getIdentifier();
    wifi_station_set_hostname((char *) config.hostname.c_str());
    wifiSetup();

    otaSetup();
    mqttSetup();
    webServerSetup();

    #if ENABLE_NOFUSS
        nofussSetup();
    #endif
    #if ENABLE_RF
        rfSetup();
    #endif
    #if ENABLE_DHT
        dhtSetup();
    #endif
    #if ENABLE_EMON
        powerMonitorSetup();
    #endif

}

void loop() {

    wifiLoop();
    hardwareLoop();
    buttonLoop();
    otaLoop();
    mqttLoop();
    webServerLoop();

    #if ENABLE_NOFUSS
        nofussLoop();
    #endif
    #if ENABLE_RF
        rfLoop();
    #endif
    #if ENABLE_DHT
        dhtLoop();
    #endif
    #if ENABLE_EMON
        powerMonitorLoop();
    #endif

    delay(1);

}
