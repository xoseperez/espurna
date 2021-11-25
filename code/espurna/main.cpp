/*

ESPurna

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

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

#include "espurna.h"
#include "main.h"

// -----------------------------------------------------------------------------
// GENERAL CALLBACKS
// -----------------------------------------------------------------------------

namespace espurna {
namespace {
namespace main {
namespace build {

// XXX: some SYS tasks require more time than yield(), as they won't
//      be scheduled if user task is always doing something
//      no particular need to delay too much though, besides attempting to reduce
//      the power consuption of the board
constexpr espurna::duration::Milliseconds LoopDelayMin { 10 };
constexpr espurna::duration::Milliseconds LoopDelayMax { 300 };

constexpr espurna::duration::Milliseconds loopDelay() {
    return espurna::duration::Milliseconds { LOOP_DELAY_TIME };
}

} // namespace build

namespace settings {

espurna::duration::Milliseconds loopDelay() {
    return std::clamp(getSetting("loopDelay", build::loopDelay()), build::LoopDelayMin, build::LoopDelayMax);
}

} // namespace settings

namespace internal {

std::vector<LoopCallback> loop_callbacks;
espurna::duration::Milliseconds loop_delay { build::LoopDelayMin };

std::vector<LoopCallback> reload_callbacks;
bool reload_flag { false };

} // namespace internal

bool reload() {
    if (internal::reload_flag) {
        internal::reload_flag = false;
        return true;
    }

    return false;
}

void loop() {
    // Reload config before running any callbacks
    if (reload()) {
        for (const auto& callback : internal::reload_callbacks) {
            callback();
        }
    }

    for (const auto& callback : internal::loop_callbacks) {
        callback();
    }

    espurna::time::delay(internal::loop_delay);
}

void setup() {
    // -------------------------------------------------------------------------
    // Basic modules, will always run
    // -------------------------------------------------------------------------

    // Cache initial free heap value
    systemInitialFreeHeap();

    // Init logging module
    #if DEBUG_SUPPORT
        debugSetup();
    #endif

    // Init GPIO functions
    gpioSetup();

    // Init RTCMEM
    rtcmemSetup();

    // Init EEPROM
    eepromSetup();

    // Init persistance
    settingsSetup();

    // Configure logger and crash recorder
    #if DEBUG_SUPPORT
        debugConfigureBoot();
        crashSetup();
    #endif

    // Init Serial, SPIFFS and system check
    systemSetup();

    // Init terminal features
    #if TERMINAL_SUPPORT
        terminalSetup();
    #endif

    // Hostname & board name initialization
    setDefaultHostname();
    setBoardName();

    boardSetup();
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
        #if DEBUG_WEB_SUPPORT
            debugWebSetup();
        #endif
        #if OTA_WEB_SUPPORT
            otaWebSetup();
        #endif
    #endif

    // Multiple modules depend on the generic 'API' services
    #if API_SUPPORT || TERMINAL_WEB_API_SUPPORT || PROMETHEUS_SUPPORT
        apiCommonSetup();
    #endif

    #if API_SUPPORT
        apiSetup();
    #endif

    // Run terminal command and send back the result
    #if TERMINAL_WEB_API_SUPPORT
        terminalWebApiSetup();
    #endif

    // Special HTTP metrics endpoint
    #if PROMETHEUS_SUPPORT
        prometheusSetup();
    #endif

    // Hardware GPIO expander, needs to be available for modules down below
    #if MCP23S08_SUPPORT
        MCP23S08Setup();
    #endif

    // lightSetup must be called before relaySetup
    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightSetup();
    #endif
    // rpnSetup must be called before relaySetup
    #if RPN_RULES_SUPPORT
        rpnSetup();
    #endif
    #if RELAY_SUPPORT
        relaySetup();
    #endif
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
    #if RFB_SUPPORT
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
    #if TUYA_SUPPORT
        tuya::setup();
    #endif
    #if KINGART_CURTAIN_SUPPORT
        kingartCurtainSetup();
    #endif
    #if FAN_SUPPORT
        fanSetup();
    #endif
    #if GARLAND_SUPPORT
        garlandSetup();
    #endif

    #if USE_EXTRA
        extraSetup();
    #endif
    
    // Update `cfg` version
    migrate();

    // Set up delay() after loop callbacks are finished
    // Notice that this requires settings storage to be available and must be **after** settingsSetup()!
    internal::loop_delay = settings::loopDelay();
}

} // namespace main
} // namespace
} // namespace espurna

void espurnaRegisterLoop(LoopCallback callback) {
    espurna::main::internal::loop_callbacks.push_back(callback);
}

void espurnaRegisterReload(LoopCallback callback) {
    espurna::main::internal::reload_callbacks.push_back(callback);
}

void espurnaReload() {
    espurna::main::internal::reload_flag = true;
}

espurna::duration::Milliseconds espurnaLoopDelay() {
    return espurna::main::internal::loop_delay;
}

void espurnaLoopDelay(espurna::duration::Milliseconds value) {
    espurna::main::internal::loop_delay = value;
}

void setup() {
    espurna::main::setup();
}

void loop() {
    espurna::main::loop();
}
