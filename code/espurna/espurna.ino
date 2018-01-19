/*

ESPurna

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

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

unsigned long _loopDelay = 0;

void hardwareSetup() {

    EEPROM.begin(EEPROM_SIZE);

    #if DEBUG_SERIAL_SUPPORT
        DEBUG_PORT.begin(SERIAL_BAUDRATE);
        #if DEBUG_ESP_WIFI
            DEBUG_PORT.setDebugOutput(true);
        #endif
    #elif defined(SERIAL_BAUDRATE)
        Serial.begin(SERIAL_BAUDRATE);
    #endif

    #if SPIFFS_SUPPORT
        SPIFFS.begin();
    #endif

    #if defined(ESPLIVE)
        //The ESPLive has an ADC MUX which needs to be configured.
        pinMode(16, OUTPUT);
        digitalWrite(16, HIGH); //Defualt CT input (pin B, solder jumper B)
    #endif

}

void hardwareLoop() {

    // Heartbeat
    #if HEARTBEAT_ENABLED
        static unsigned long last = 0;
        if ((last == 0) || (millis() - last > HEARTBEAT_INTERVAL)) {
            last = millis();
            heartbeat();
        }
    #endif // HEARTBEAT_ENABLED

}

// -----------------------------------------------------------------------------
// BOOTING
// -----------------------------------------------------------------------------

void setup() {

    // Init EEPROM, Serial and SPIFFS
    hardwareSetup();

    // Question system stability
    #if SYSTEM_CHECK_ENABLED
        systemCheck(false);
    #endif

    // Init persistance and terminal features
    settingsSetup();
    if (getSetting("hostname").length() == 0) {
        setSetting("hostname", getIdentifier());
    }
    setBoardName();

    // Cache loop delay value to speed things (recommended max 250ms)
    _loopDelay = atol(getSetting("loopDelay", LOOP_DELAY_TIME).c_str());

    // Show welcome message and system configuration
    info();

    // Basic modules, will always run
    wifiSetup();
    otaSetup();
    #if TELNET_SUPPORT
        telnetSetup();
    #endif

    // Do not run the next services if system is flagged stable
    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) return;
    #endif

    // Init webserver required before any module that uses API
    #if WEB_SUPPORT
        webSetup();
        wsSetup();
        apiSetup();
    #endif

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightSetup();
    #endif
    relaySetup();
    buttonSetup();
    ledSetup();
    #if MQTT_SUPPORT
        mqttSetup();
    #endif

    #if MDNS_SERVER_SUPPORT
        mdnsServerSetup();
    #endif
    #if LLMNR_SUPPORT
        llmnrSetup();
    #endif
    #if NETBIOS_SUPPORT
        netbiosSetup();
    #endif
    #if SSDP_SUPPORT
        ssdpSetup();
    #endif
    #if NTP_SUPPORT
        ntpSetup();
    #endif
    #if I2C_SUPPORT
        i2cSetup();
        #if I2C_CLEAR_BUS
            i2cClearBus();
        #endif
        i2cScan();
    #endif

    #ifdef ITEAD_SONOFF_RFBRIDGE
        rfbSetup();
    #endif
    #if ALEXA_SUPPORT
        alexaSetup();
    #endif
    #if NOFUSS_SUPPORT
        nofussSetup();
    #endif
    #if INFLUXDB_SUPPORT
        idbSetup();
    #endif
    #if THINGSPEAK_SUPPORT
        tspkSetup();
    #endif
    #if RF_SUPPORT
        rfSetup();
    #endif
    #if IR_SUPPORT
        irSetup();
    #endif
    #if DOMOTICZ_SUPPORT
        domoticzSetup();
    #endif
    #if HOMEASSISTANT_SUPPORT
        haSetup();
    #endif
    #if SENSOR_SUPPORT
        sensorSetup();
    #endif
    #if SCHEDULER_SUPPORT
        schSetup();
    #endif

    // 3rd party code hook
    #if USE_EXTRA
        extraSetup();
    #endif

    // Prepare configuration for version 2.0
    migrate();

    saveSettings();

}

void loop() {

    hardwareLoop();
    settingsLoop();
    wifiLoop();
    otaLoop();

    #if SYSTEM_CHECK_ENABLED
        systemCheckLoop();
        // Do not run the next services if system is flagged stable
        if (!systemCheck()) return;
    #endif

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightLoop();
    #endif
    relayLoop();
    buttonLoop();
    ledLoop();
    #if MQTT_SUPPORT
        mqttLoop();
    #endif

    #ifdef ITEAD_SONOFF_RFBRIDGE
        rfbLoop();
    #endif
    #if SSDP_SUPPORT
        ssdpLoop();
    #endif
    #if NTP_SUPPORT
        ntpLoop();
    #endif
    #if ALEXA_SUPPORT
        alexaLoop();
    #endif
    #if NOFUSS_SUPPORT
        nofussLoop();
    #endif
    #if RF_SUPPORT
        rfLoop();
    #endif
    #if IR_SUPPORT
        irLoop();
    #endif
    #if SENSOR_SUPPORT
        sensorLoop();
    #endif
    #if THINGSPEAK_SUPPORT
        tspkLoop();
    #endif
    #if SCHEDULER_SUPPORT
        schLoop();
    #endif
    #if MDNS_CLIENT_SUPPORT
        mdnsClientLoop();
    #endif

    // 3rd party code hook
    #if USE_EXTRA
        extraLoop();
    #endif

    // Power saving delay
    delay(_loopDelay);

}
