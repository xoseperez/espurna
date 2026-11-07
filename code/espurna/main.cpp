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

#include <algorithm>
#include <utility>

#if ESPURNA_ASYNC_TASKS
#include <mutex>
#endif

#include "main.h"
#include "espurna_ota.h"
#include "rtcmem.h"

#if NSPANEL_SUPPORT
#include "nspanel.h"
#endif

#if MATTER_SUPPORT
#include "espurna_matter.h"
#endif

#if SAFEBOOT_SUPPORT
#include "safeboot_esp32.h"
#endif

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
namespace keys {

PROGMEM_STRING(LoopDelay, "loopDelay");

} // namespace keys

espurna::duration::Milliseconds loopDelay() {
    return std::clamp(getSetting(keys::LoopDelay, build::loopDelay()), build::LoopDelayMin, build::LoopDelayMax);
}

} // namespace settings

namespace internal {

std::vector<LoopCallback> reload_callbacks;
bool reload_flag { false };

std::vector<LoopCallback> loop_callbacks;
espurna::duration::Milliseconds loop_delay { build::LoopDelayMin };

// read by the stall guard (ESP32, another task), ref. espurnaLoopCurrent()
volatile uintptr_t current { 0 };

std::forward_list<Callback> once_callbacks;

#if ESPURNA_ASYNC_TASKS
// once callbacks are also scheduled from other tasks (e.g. SNTP notification in tcpip task)
std::mutex once_mutex;
#endif

} // namespace internal

#if ESPURNA_ASYNC_TASKS
using OnceLock = std::lock_guard<std::mutex>;
#define ONCE_LOCK() OnceLock once_lock(internal::once_mutex)
#else
#define ONCE_LOCK()
#endif

void flag_reload() {
    internal::reload_flag = true;
}

bool check_reload() {
    if (internal::reload_flag) {
        internal::reload_flag = false;
        return true;
    }

    return false;
}

void push_reload(ReloadCallback callback) {
    internal::reload_callbacks.push_back(callback);
}

void push_loop(LoopCallback callback) {
    internal::loop_callbacks.push_back(callback);
}

duration::Milliseconds loop_delay() {
    return internal::loop_delay;
}

void loop_delay(duration::Milliseconds value) {
    internal::loop_delay = value;
}

void push_once(Callback callback) {
    ONCE_LOCK();
    internal::once_callbacks.push_front(std::move(callback));
}

void push_once_unique(Callback::Type callback) {
    ONCE_LOCK();
    auto& callbacks = internal::once_callbacks;

    auto it = std::find_if(
        callbacks.begin(),
        callbacks.end(),
        [&](const Callback& other) {
            return other == callback;
        });

    if ((it != callbacks.begin()) && (it != callbacks.end())) {
        std::swap(*callbacks.begin(), *it);
        return;
    }

    callbacks.push_front(Callback(callback));
}

void loop() {
    {
        // network callbacks (ESP32) are handled while we are in delay()
        system::LoopGuard guard;

        // Reload config before running any callbacks
        if (check_reload()) {
            DEBUG_MSG_P(PSTR("[MAIN] Reloading configuration...\n"));
            for (const auto& callback : internal::reload_callbacks) {
                internal::current = reinterpret_cast<uintptr_t>(callback);
                callback();
            }
        }

        // Loop callbacks, registered some time in setup()
        // Notice that everything is in order of registration
        for (const auto& callback : internal::loop_callbacks) {
            internal::current = reinterpret_cast<uintptr_t>(callback);
            callback();
        }

        internal::current = 1;

        // One-time callbacks, registered some time during runtime
        // Notice that callback container is LIFO, most recently added
        // callback is called first. Copy to allow container modifications.
        decltype(internal::once_callbacks) once_callbacks;
        {
            ONCE_LOCK();
            once_callbacks.swap(internal::once_callbacks);
        }

        for (const auto& callback : once_callbacks) {
            callback();
        }

        internal::current = 0;
    }

    espurna::time::delay(internal::loop_delay);
}

// -DSETUP_HEAP_PROBE_SUPPORT=1: free heap after every module setup, `HEAP.SETUP` prints it (finding the modules that
// take memory while they are not configured)
#if SETUP_HEAP_PROBE_SUPPORT
namespace heap_probe {
namespace {

struct Probe {
    const char* name;
    uint32_t heap;
};

Probe probes[96];
size_t count { 0 };

PROGMEM_STRING(HeapSetup, "HEAP.SETUP");

void print(::terminal::CommandContext&& ctx) {
    uint32_t last = systemInitialFreeHeap();
    for (size_t index = 0; index < count; ++index) {
        ctx.output.printf_P(PSTR("%-16s %6u %6d\n"), probes[index].name, probes[index].heap,
            static_cast<int>(last) - static_cast<int>(probes[index].heap));
        last = probes[index].heap;
    }
    terminalOK(ctx);
}

static constexpr espurna::terminal::Command Commands[] PROGMEM {
    {HeapSetup, print},
};

} // namespace

void add(const char* name) {
    if (count < (sizeof(probes) / sizeof(probes[0]))) {
        probes[count++] = Probe{name, ESP.getFreeHeap()};
    }
}

void setup() {
    espurna::terminal::add(Commands);
}

} // namespace heap_probe
#define SETUP_HEAP_PROBE(NAME) heap_probe::add(NAME)
#else
#define SETUP_HEAP_PROBE(NAME)
#endif

void setup() {
    system::LoopGuard guard;

    // -------------------------------------------------------------------------
    // Basic modules, will always run
    // -------------------------------------------------------------------------

    // Cache initial free heap value
    systemInitialFreeHeap();

    // Init debug & logging routines
    debugSetup();
    SETUP_HEAP_PROBE("debug");

    // Init GPIO functions
    gpioSetup();
    SETUP_HEAP_PROBE("gpio");

    // Init RTCMEM
    rtcmemSetup();
    SETUP_HEAP_PROBE("rtcmem");

    // Init EEPROM
    eepromSetup();
    SETUP_HEAP_PROBE("eeprom");

    // Init persistance
    settingsSetup();
    SETUP_HEAP_PROBE("settings");

    // Init hardware / software UART ports
    #if UART_SUPPORT
        uartSetup();
        SETUP_HEAP_PROBE("uart");
    #endif

    // Extra configuration after settings can be used
    debugConfigureBoot();

    // Crash dump only when debugging is supported as well
    #if DEBUG_SUPPORT
        crashSetup();
        SETUP_HEAP_PROBE("crash");
    #endif

    // Init Serial, SPIFFS and system check
    systemSetup();
    SETUP_HEAP_PROBE("system");

    // Init terminal features
    #if TERMINAL_SUPPORT
        terminalSetup();
        SETUP_HEAP_PROBE("terminal");
    #endif

    networkSetup();

    SETUP_HEAP_PROBE("network");
    wifiSetup();
    SETUP_HEAP_PROBE("wifi");
    otaSetup();
    SETUP_HEAP_PROBE("ota");

    // Our app banner (usually, for uart)
    #if DEBUG_SUPPORT
        debugShowBanner();
    #endif

    #if TELNET_SUPPORT
        telnetSetup();
        SETUP_HEAP_PROBE("telnet");
    #endif

    // NSPanel display power and buzzer pins, also in the unstable mode
    #if NSPANEL_SUPPORT
        nspanelBootSetup();
        SETUP_HEAP_PROBE("nspanelBoot");
    #endif

    // Asymmetric (safeboot) flash layout: crash-loop guard and `safeboot`, also in the unstable mode
    #if SAFEBOOT_SUPPORT
        safebootSetup();
        SETUP_HEAP_PROBE("safeboot");
    #endif

    // -------------------------------------------------------------------------
    // Check if system is stable
    // -------------------------------------------------------------------------

    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) {
            systemSetupUnstable();
            return;
        }
    #endif

    // -------------------------------------------------------------------------
    // Next modules will be only loaded if system is flagged as stable
    // -------------------------------------------------------------------------

    // Init webserver required before any module that uses API
    #if WEB_SUPPORT
        webSetup();
        SETUP_HEAP_PROBE("web");
        wsSetup();
        SETUP_HEAP_PROBE("ws");
        #if DEBUG_WEB_SUPPORT
            debugWebSetup();
            SETUP_HEAP_PROBE("debugWeb");
        #endif
        #if OTA_WEB_SUPPORT
            otaWebSetup();
            SETUP_HEAP_PROBE("otaWeb");
        #endif
        #if SAFEBOOT_SUPPORT
            safebootWebSetup();
            SETUP_HEAP_PROBE("safebootWeb");
        #endif
    #endif

    // Multiple modules depend on the generic 'API' services
    #if API_SUPPORT || TERMINAL_WEB_API_SUPPORT || PROMETHEUS_SUPPORT
        apiCommonSetup();
        SETUP_HEAP_PROBE("apiCommon");
    #endif

    #if API_SUPPORT
        apiSetup();
        SETUP_HEAP_PROBE("api");
    #endif

    // Run terminal command and send back the result
    #if TERMINAL_WEB_API_SUPPORT
        terminalWebApiSetup();
        SETUP_HEAP_PROBE("terminalWebApi");
    #endif

    // Special HTTP metrics endpoint
    #if PROMETHEUS_SUPPORT
        prometheusSetup();
        SETUP_HEAP_PROBE("prometheus");
    #endif

    // Hardware GPIO expander, needs to be available for modules down below
    #if MCP23S08_SUPPORT
        MCP23S08Setup();
        SETUP_HEAP_PROBE("MCP23S08");
    #endif

    // PWM driver
    #if PWM_SUPPORT
        pwmSetup();
        SETUP_HEAP_PROBE("pwm");
    #endif

    // lightSetup must be called before relaySetup
    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightSetup();
        SETUP_HEAP_PROBE("light");
    #endif
    // rpnSetup must be called before relaySetup
    #if RPN_RULES_SUPPORT
        rpnSetup();
        SETUP_HEAP_PROBE("rpn");
    #endif
    #if RELAY_SUPPORT
        relaySetup();
        SETUP_HEAP_PROBE("relay");
    #endif
    #if BUTTON_SUPPORT
        buttonSetup();
        SETUP_HEAP_PROBE("button");
    #endif
    #if ENCODER_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
        encoderSetup();
        SETUP_HEAP_PROBE("encoder");
    #endif
    #if LED_SUPPORT
        ledSetup();
        SETUP_HEAP_PROBE("led");
    #endif

    #if MQTT_SUPPORT
        mqttSetup();
        SETUP_HEAP_PROBE("mqtt");
    #endif
    #if MATTER_SUPPORT
        matterSetup();
        SETUP_HEAP_PROBE("matter");
    #endif
    #if MDNS_SERVER_SUPPORT
        mdnsServerSetup();
        SETUP_HEAP_PROBE("mdnsServer");
    #endif
    #if LLMNR_SUPPORT
        llmnrSetup();
        SETUP_HEAP_PROBE("llmnr");
    #endif
    #if NETBIOS_SUPPORT
        netbiosSetup();
        SETUP_HEAP_PROBE("netbios");
    #endif
    #if SSDP_SUPPORT
        ssdpSetup();
        SETUP_HEAP_PROBE("ssdp");
    #endif
    #if NTP_SUPPORT
        ntpSetup();
        SETUP_HEAP_PROBE("ntp");
    #endif
    #if I2C_SUPPORT
        i2cSetup();
        SETUP_HEAP_PROBE("i2c");
    #endif
    #if ONE_WIRE_SUPPORT
        oneWireSetup();
        SETUP_HEAP_PROBE("oneWire");
    #endif
    #if RFB_SUPPORT
        rfbSetup();
        SETUP_HEAP_PROBE("rfb");
    #endif
    #if ALEXA_SUPPORT
        alexaSetup();
        SETUP_HEAP_PROBE("alexa");
    #endif
    #if NOFUSS_SUPPORT
        nofussSetup();
        SETUP_HEAP_PROBE("nofuss");
    #endif
    #if SENSOR_SUPPORT
        sensorSetup();
        SETUP_HEAP_PROBE("sensor");
    #endif
    #if INFLUXDB_SUPPORT
        idbSetup();
        SETUP_HEAP_PROBE("idb");
    #endif
    #if THINGSPEAK_SUPPORT
        tspkSetup();
        SETUP_HEAP_PROBE("tspk");
    #endif
    #if RFM69_SUPPORT
        rfm69Setup();
        SETUP_HEAP_PROBE("rfm69");
    #endif
    #if IR_SUPPORT
        irSetup();
        SETUP_HEAP_PROBE("ir");
    #endif
    #if DOMOTICZ_SUPPORT
        domoticzSetup();
        SETUP_HEAP_PROBE("domoticz");
    #endif
    #if HOMEASSISTANT_SUPPORT
        haSetup();
        SETUP_HEAP_PROBE("ha");
    #endif
    #if SCHEDULER_SUPPORT
        schSetup();
        SETUP_HEAP_PROBE("sch");
    #endif
    #if UART_MQTT_SUPPORT
        uartMqttSetup();
        SETUP_HEAP_PROBE("uartMqtt");
    #endif
    #if LIGHTFOX_SUPPORT
        lightfoxSetup();
        SETUP_HEAP_PROBE("lightfox");
    #endif
    #if THERMOSTAT_SUPPORT
        thermostatSetup();
        SETUP_HEAP_PROBE("thermostat");
    #endif
    #if THERMOSTAT_DISPLAY_SUPPORT
        displaySetup();
        SETUP_HEAP_PROBE("display");
    #endif
    #if TUYA_SUPPORT
        tuya::setup();
    #endif
    #if CURTAIN_SUPPORT
        curtainSetup();
        SETUP_HEAP_PROBE("curtain");
    #endif
    #if FAN_SUPPORT
        fanSetup();
        SETUP_HEAP_PROBE("fan");
    #endif
    #if GARLAND_SUPPORT
        garlandSetup();
        SETUP_HEAP_PROBE("garland");
    #endif
    #if NSPANEL_SUPPORT
        nspanelSetup();
        SETUP_HEAP_PROBE("nspanel");
    #endif

    #if USE_EXTRA
        extraSetup();
        SETUP_HEAP_PROBE("extra");
    #endif

    // Update `cfg` version
    migrate();

    // Set up delay() after loop callbacks are finished
    // Notice that this requires settings storage to be available and must be **after** settingsSetup()!
    internal::loop_delay = settings::loopDelay();
    SETUP_HEAP_PROBE("end");
#if SETUP_HEAP_PROBE_SUPPORT
    heap_probe::setup();
#endif
}

} // namespace main

} // namespace
} // namespace espurna

void espurnaRegisterOnce(espurna::Callback callback) {
    espurna::main::push_once(std::move(callback));
}

void espurnaRegisterOnceUnique(espurna::Callback::Type ptr) {
    espurna::main::push_once_unique(ptr);
}

void espurnaRegisterReload(LoopCallback callback) {
    espurna::main::push_reload(callback);
}

void espurnaRegisterLoop(LoopCallback callback) {
    espurna::main::push_loop(callback);
}

void espurnaReload() {
    espurna::main::flag_reload();
}

uintptr_t espurnaLoopCurrent() {
    return espurna::main::internal::current;
}

espurna::duration::Milliseconds espurnaLoopDelay() {
    return espurna::main::loop_delay();
}

void espurnaLoopDelay(espurna::duration::Milliseconds value) {
    espurna::main::loop_delay(value);
}

void setup() {
    espurna::main::setup();
}

void loop() {
    espurna::main::loop();
}

