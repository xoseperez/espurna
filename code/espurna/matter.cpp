/*

MATTER MODULE

The part shared by the architectures: settings, the relay link, the terminal commands and the MATTER panel of the
WebUI. The Matter stack itself is the backend of the architecture (espurna_matter.h)

*/

#include "espurna.h"

#if MATTER_SUPPORT

#include "espurna_matter.h"
#include "relay.h"
#include "terminal.h"
#include "ws.h"

#if MQTT_SUPPORT
#include "mqtt.h"
#endif

#if ALEXA_SUPPORT
#include "alexa.h"
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#else
#include <FreeRTOS.h>
#include <semphr.h>
#endif

#include <atomic>
#include <cstring>

namespace espurna {
namespace matter {
namespace {

bool started { false };
bool connected { false };

// This boot was asked for by an OTA upload that cannot run while Matter does (ref. matterRebootForUpload())
bool skip_start { false };

// Relay changes from Matter come from its own task, they are applied in the loop: bit n of `changed` marks
// relay n, bit n of `values` is its new state
std::atomic<uint32_t> changed { 0 };
std::atomic<uint32_t> values { 0 };

std::atomic<bool> alive { false };

// Commissioning in progress (from the Matter task), applied in the loop
std::atomic<int> busy { -1 };

// Log lines of the Matter task, printed from the loop (ESPurna debug output is not for other tasks).
// Fixed slots, a line that does not fit is counted as dropped
namespace log {

constexpr size_t Slots { 8 };
constexpr size_t Length { 120 };

char lines[Slots][Length];
size_t head { 0 };
size_t tail { 0 };
size_t dropped { 0 };
SemaphoreHandle_t mutex { nullptr };

void push(const char* line) {
    if (!mutex) {
        return;
    }

    // A burst of lines waits a little for the loop to print them (it runs every 10 ms)
    for (size_t attempt = 0; attempt < 5; ++attempt) {
        xSemaphoreTake(mutex, portMAX_DELAY);
        if ((head - tail) < Slots) {
            std::strncpy(lines[head % Slots], line, Length - 1);
            lines[head % Slots][Length - 1] = '\0';
            ++head;
            xSemaphoreGive(mutex);
            return;
        }
        xSemaphoreGive(mutex);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    ++dropped;
    xSemaphoreGive(mutex);
}

void print() {
    if (!mutex) {
        return;
    }

    char line[Length];
    for (;;) {
        size_t lost { 0 };

        xSemaphoreTake(mutex, portMAX_DELAY);
        const bool empty = (head == tail);
        if (!empty) {
            std::memcpy(line, lines[tail % Slots], Length);
            ++tail;
        }
        lost = dropped;
        dropped = 0;
        xSemaphoreGive(mutex);

        if (lost) {
            DEBUG_MSG_P(PSTR("[MATTER] (%u lines dropped)\n"), lost);
        }

        if (empty) {
            break;
        }

        DEBUG_MSG_P(PSTR("[MATTER] %s\n"), line);
    }
}

} // namespace log

void web_tick();

// Quiet mode while a controller commissions: the certificate checks (PASE / CASE, CSR, NOC) need tens of KB of heap
// at once, more than is left with every module busy. Web socket clients are closed, MQTT disconnects, the Alexa servers
// stop; settings stay, everything comes back at the end (or after QuietMaxMs without an end event)
constexpr uint32_t QuietMaxMs { 5 * 60 * 1000 };
uint32_t quiet_since { 0 };

#if MQTT_SUPPORT
bool quiet_mqtt { false };
#endif

void quiet(bool active) {
    if (active == (quiet_since != 0)) {
        return;
    }

    quiet_since = active ? (millis() | 1) : 0;

#if WEB_SUPPORT
    // web socket clients hold heap the certificate checks need (Beken: ~10 s of them would run out otherwise)
    wsPause(active);
#endif

#if MQTT_SUPPORT
    if (active) {
        quiet_mqtt = mqttEnabled();
        if (quiet_mqtt) {
            mqttEnabled(false);
            mqttDisconnect();
        }
    } else if (quiet_mqtt) {
        mqttEnabled(true);
        quiet_mqtt = false;
    }
#endif

#if ALEXA_SUPPORT
    alexaPause(active);
#endif

    DEBUG_MSG_P(PSTR("[MATTER] Quiet mode %s, heap %u\n"), active ? "on" : "off", backend::heap());
}

void loop() {
    const auto mask = changed.exchange(0);
    if (mask) {
        const auto state = values.load();
        for (size_t index = 0; index < backend::endpoints(); ++index) {
            if (mask & (1u << index)) {
                relayStatus(index, (state & (1u << index)) != 0);
            }
        }
    }

    const auto active = busy.exchange(-1);
    if (active >= 0) {
        DEBUG_MSG_P(PSTR("[MATTER] Commissioning %s\n"), active ? "started" : "ended");
        quiet(active != 0);
    } else if (quiet_since && ((millis() - quiet_since) > QuietMaxMs)) {
        DEBUG_MSG_P(PSTR("[MATTER] No end of commissioning in %u s, resuming\n"), QuietMaxMs / 1000);
        quiet(false);
    }

    if (alive.exchange(false)) {
        DEBUG_MSG_P(PSTR("[MATTER] Task is alive\n"));
    }

    log::print();
    web_tick();
}

void start() {
    if (started) {
        return;
    }

    started = backend::start();
    DEBUG_MSG_P(PSTR("[MATTER] Start %s, %u endpoint(s), heap %u\n"),
        started ? "OK" : "FAILED", backend::endpoints(), backend::heap());

    if (started) {
        for (size_t index = 0; index < backend::endpoints(); ++index) {
            backend::set_onoff(index, relayStatus(index) == RelayStatus::On);
        }
    }
}

void wifi_event(espurna::wifi::Event event) {
    switch (event) {
    case espurna::wifi::Event::StationConnected:
        connected = true;
        backend::network(true);
        // Bring-up: a crash in Matter must not put the board into a boot loop, so it only starts on
        // MATTER.START until `matterAuto 1` is set
        if (getSetting("matterAuto", false) && !skip_start) {
            start();
        }
        break;

    case espurna::wifi::Event::StationDisconnected:
        if (connected) {
            connected = false;
            backend::network(false);
        }
        break;

    default:
        break;
    }
}

#if TERMINAL_SUPPORT
PROGMEM_STRING(Status, "MATTER");

void command_status(::terminal::CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("started: %s, fabrics: %d, heap: %u, lowest since boot: %u\n"),
        started ? "yes" : "no",
        started ? backend::fabrics() : 0,
        backend::heap(), backend::heap_min());
    terminalOK(ctx);
}

PROGMEM_STRING(Window, "MATTER.WINDOW");

// Commissioning window again: it closes after 15 minutes and after a controller added the device
void command_window(::terminal::CommandContext&& ctx) {
    if (!started) {
        terminalError(ctx, F("Matter is not started"));
        return;
    }

    backend::open_window();
    terminalOK(ctx);
}

PROGMEM_STRING(Ping, "MATTER.PING");

// Work item through the Matter task, answered from the loop
void command_ping(::terminal::CommandContext&& ctx) {
    if (!started) {
        terminalError(ctx, F("Matter is not started"));
        return;
    }

    backend::ping();
    terminalOK(ctx);
}

PROGMEM_STRING(Start, "MATTER.START");

void command_start(::terminal::CommandContext&& ctx) {
    if (!connected) {
        terminalError(ctx, F("Not connected to Wi-Fi"));
        return;
    }

    start();
    if (!started) {
        terminalError(ctx, F("Start failed"));
        return;
    }

    terminalOK(ctx);
}

// The manual pairing code and the QR payload carry the setup passcode, they are for the device owner only
PROGMEM_STRING(Codes, "MATTER.CODES");

void command_codes(::terminal::CommandContext&& ctx) {
    if (!started) {
        terminalError(ctx, F("Matter is not started"));
        return;
    }

    char manual[24];
    char qr[32];
    if (!backend::codes(manual, sizeof(manual), qr, sizeof(qr))) {
        terminalError(ctx, F("No commissioning data"));
        return;
    }

    ctx.output.printf_P(PSTR("manual pairing code: %s\nQR code: %s\n"), manual, qr);
    terminalOK(ctx);
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Status, command_status},
    {Codes, command_codes},
    {Start, command_start},
    {Ping, command_ping},
    {Window, command_window},
};
#endif

// ---------------------------------------------------------------------------------------------------------------------
// MATTER panel of the WebUI: status, start, auto start and the pairing codes (they carry the setup passcode, the panel
// is behind the WebUI login like the rest of the settings)

#if WEB_SUPPORT
namespace web {

STRING_VIEW_INLINE(Prefix, "matter");

int fabrics { -1 };
espurna::time::CoreClock::time_point last;

bool onKeyCheck(espurna::StringView key, const JsonVariant&) {
    return key.startsWith(Prefix);
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, Prefix);
}

void onData(JsonObject& root) {
    root["matterStarted"] = started ? F("yes") : F("no");
    root["matterFabrics"] = started ? backend::fabrics() : 0;
    root["matterProduct"] = backend::product();

    char manual[24] {};
    char qr[32] {};
    if (started && backend::codes(manual, sizeof(manual), qr, sizeof(qr))) {
        root["matterManual"] = manual;
        root["matterQr"] = qr;
    } else {
        root["matterManual"] = "";
        root["matterQr"] = "";
    }
}

void onConnected(JsonObject& root) {
    root["matterAuto"] = getSetting("matterAuto", false);
    onData(root);
}

void onAction(uint32_t, const char* action, JsonObject&) {
    if (std::strcmp(action, "matterWindow") == 0) {
        if (started) {
            backend::open_window();
        }
        return;
    }

    if (std::strcmp(action, "matterStart") != 0) {
        return;
    }

    if (connected) {
        start();
    }

    wsPost(onData);
}

void setup() {
    wsRegister()
        .onVisible(onVisible)
        .onConnected(onConnected)
        .onAction(onAction)
        .onKeyCheck(onKeyCheck);
}

} // namespace web

// a controller was added or removed: refresh the panel
void web_tick() {
    if (!started) {
        return;
    }

    const auto now = espurna::time::CoreClock::now();
    if ((now - web::last) < espurna::duration::Seconds(2)) {
        return;
    }
    web::last = now;

    const auto count = backend::fabrics();
    if (count != web::fabrics) {
        web::fabrics = count;
        if (wsConnected()) {
            wsPost(web::onData);
        }
    }
}
#else
void web_tick() {
}
#endif

} // namespace

void onoff_changed(size_t index, bool on) {
    if (index >= 32) {
        return;
    }

    const auto bit = 1u << index;
    if (on) {
        values.fetch_or(bit);
    } else {
        values.fetch_and(~bit);
    }
    changed.fetch_or(bit);
}

void commissioning(bool active) {
    busy = active ? 1 : 0;
}

void pong() {
    alive = true;
}

void log_line(const char* line) {
    log::push(line);
}

void setup() {
    skip_start = getSetting("matterSkip", false);
    if (skip_start) {
        delSetting("matterSkip");
        DEBUG_MSG_P(PSTR("[MATTER] Not started on this boot, it is for an OTA upload\n"));
    }

    log::mutex = xSemaphoreCreateMutex();
    backend::setup();
#if TERMINAL_SUPPORT
    terminal::add(Commands);
#endif
#if WEB_SUPPORT
    web::setup();
#endif
    wifiRegister(wifi_event);
    relayOnStatusChange([](size_t id, bool status) {
        if (started && (id < backend::endpoints())) {
            backend::set_onoff(id, status);
        }
    });
    espurnaRegisterLoop(loop);
}

} // namespace matter
} // namespace espurna

void matterSetup() {
    espurna::matter::setup();
}

bool matterStarted() {
    return espurna::matter::started;
}

// Matter code lives where the OTA image is written (2 MiB Beken boards): restart once without starting Matter
void matterRebootForUpload() {
    setSetting("matterSkip", 1);
    prepareReset(CustomResetReason::Ota);
}

#endif
