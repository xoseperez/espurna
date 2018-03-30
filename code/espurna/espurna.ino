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
#include <vector>

std::vector<void (*)()> _loop_callbacks;

// -----------------------------------------------------------------------------
// REGISTER
// -----------------------------------------------------------------------------

void espurnaRegisterLoop(void (*callback)()) {
    _loop_callbacks.push_back(callback);
}

// -----------------------------------------------------------------------------
// BOOTING
// -----------------------------------------------------------------------------

void setup() {

    // -------------------------------------------------------------------------
    // Basic modules, will always run
    // -------------------------------------------------------------------------

    // Init EEPROM, Serial, SPIFFS and system check
    systemSetup();

    // Init persistance and terminal features
    settingsSetup();

    // Hostname & board name initialization
    if (getSetting("hostname").length() == 0) {
        setDefaultHostname();
    }
    setBoardName();

    // Show welcome message and system configuration
    info();

    wifiSetup();
    otaSetup();
    #if TELNET_SUPPORT
        telnetSetup();
    #endif

    // -------------------------------------------------------------------------
    // Check if system is stable
    // -------------------------------------------------------------------------

    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) return;
    #endif

    // -------------------------------------------------------------------------
    // Next modules will be only loaded if system is flagged as stable
    // -------------------------------------------------------------------------

    // Init webserver required before any module that uses API
    #if WEB_SUPPORT
        webSetup();
        wsSetup();
        apiSetup();
        #if DEBUG_WEB_SUPPORT
            debugSetup();
        #endif
    #endif

    // lightSetup must be called before relaySetup
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
    #if MDNS_CLIENT_SUPPORT
        mdnsClientSetup();
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
    #if UART_MQTT_SUPPORT
        uartmqttSetup();
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

    // Call registered loop callbacks
    for (unsigned char i = 0; i < _loop_callbacks.size(); i++) {
        (_loop_callbacks[i])();
    }

}
