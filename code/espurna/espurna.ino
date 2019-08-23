/*

ESPurna

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

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

#include "libs/HeapStats.h"

std::vector<void (*)()> _loop_callbacks;
std::vector<void (*)()> _reload_callbacks;

bool _reload_config = false;
unsigned long _loop_delay = 0;

// -----------------------------------------------------------------------------
// GENERAL CALLBACKS
// -----------------------------------------------------------------------------

void espurnaRegisterLoop(void (*callback)()) {
    _loop_callbacks.push_back(callback);
}

void espurnaRegisterReload(void (*callback)()) {
    _reload_callbacks.push_back(callback);
}

void espurnaReload() {
    _reload_config = true;
}

void _espurnaReload() {
    for (unsigned char i = 0; i < _reload_callbacks.size(); i++) {
        (_reload_callbacks[i])();
    }
}

unsigned long espurnaLoopDelay() {
    return _loop_delay;
}

// -----------------------------------------------------------------------------
// BOOTING
// -----------------------------------------------------------------------------

void setup() {

    // -------------------------------------------------------------------------
    // Basic modules, will always run
    // -------------------------------------------------------------------------

    // Cache initial free heap value
    setInitialFreeHeap();

    // Serial debug
    #if DEBUG_SUPPORT
        debugSetup();
    #endif

    // Init RTCMEM
    rtcmemSetup();

    // Init EEPROM
    eepromSetup();

    // Init persistance
    settingsSetup();

    // Init crash recorder
    #if DEBUG_SUPPORT
        crashSetup();
    #endif

    // Return bogus free heap value for broken devices
    // XXX: device is likely to trigger other bugs! tread carefuly
    wtfHeap(getSetting("wtfHeap", 0).toInt());

    // Init Serial, SPIFFS and system check
    systemSetup();

    // Init terminal features
    #if TERMINAL_SUPPORT
        terminalSetup();
    #endif

    // Hostname & board name initialization
    if (getSetting("hostname").length() == 0) {
        setDefaultHostname();
    }
    setBoardName();

    // Show welcome message and system configuration
    info();

    wifiSetup();
    #if OTA_ARDUINOOTA_SUPPORT
        arduinoOtaSetup();
    #endif
    #if TELNET_SUPPORT
        telnetSetup();
    #endif
    #if OTA_CLIENT != OTA_CLIENT_NONE
        otaClientSetup();
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
        #if DEBUG_WEB_SUPPORT
            debugWebSetup();
        #endif
    #endif
    #if API_SUPPORT
        apiSetup();
    #endif

    // lightSetup must be called before relaySetup
    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightSetup();
    #endif
    relaySetup();
    #if BUTTON_SUPPORT
        buttonSetup();
    #endif
    #if ENCODER_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
        encoderSetup();
    #endif
    #if LED_SUPPORT
        ledSetup();
    #endif

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
    #if RF_SUPPORT
        rfbSetup();
    #endif
    #if ALEXA_SUPPORT
        alexaSetup();
    #endif
    #if NOFUSS_SUPPORT
        nofussSetup();
    #endif
    #if SENSOR_SUPPORT
        sensorSetup();
    #endif
    #if INFLUXDB_SUPPORT
        idbSetup();
    #endif
    #if THINGSPEAK_SUPPORT
        tspkSetup();
    #endif
    #if RFM69_SUPPORT
        rfm69Setup();
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
    #if SCHEDULER_SUPPORT
        schSetup();
    #endif
    #if UART_MQTT_SUPPORT
        uartmqttSetup();
    #endif
    #ifdef FOXEL_LIGHTFOX_DUAL
        lightfoxSetup();
    #endif
    #if THERMOSTAT_SUPPORT
        thermostatSetup();
    #endif
    #if THERMOSTAT_DISPLAY_SUPPORT
        displaySetup();
    #endif


    // 3rd party code hook
    #if USE_EXTRA
        extraSetup();
    #endif

    // Prepare configuration for version 2.0
    migrate();

    // Set up delay() after loop callbacks are finished
    // Note: should be after settingsSetup()
    _loop_delay = atol(getSetting("loopDelay", LOOP_DELAY_TIME).c_str());
    _loop_delay = constrain(_loop_delay, 0, 300);

    saveSettings();

}

void loop() {

    // Reload config before running any callbacks
    if (_reload_config) {
        _espurnaReload();
        _reload_config = false;
    }

    // Call registered loop callbacks
    for (unsigned char i = 0; i < _loop_callbacks.size(); i++) {
        (_loop_callbacks[i])();
    }

    // Power saving delay
    if (_loop_delay) delay(_loop_delay);

}
