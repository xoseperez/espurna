/*

TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if TERMINAL_SUPPORT

#if API_SUPPORT
#include "api.h"
#endif

#if WEB_SUPPORT
#include "ws.h"
#endif

#include "crash.h"
#include "mqtt.h"
#include "settings.h"
#include "system.h"
#include "sensor.h"
#include "telnet.h"
#include "terminal.h"
#include "utils.h"
#include "wifi.h"

#include "libs/PrintString.h"
#include "libs/Delimiter.h"

#include <algorithm>
#include <utility>

#include <Schedule.h>
#include <Stream.h>

// FS 'range', declared at compile time via .ld script PROVIDE declarations
// (althought, in recent Core versions, these may be set at runtime)
extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;

#if WEB_SUPPORT
#include "web_print.ipp"
#endif

namespace espurna {
namespace terminal {
namespace {

namespace build {

constexpr size_t serialBufferSize() {
    return TERMINAL_SERIAL_BUFFER_SIZE;
}

constexpr size_t serialPort() {
    return TERMINAL_SERIAL_PORT - 1;
}

} // namespace build

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

namespace commands {

PROGMEM_STRING(Commands, "COMMANDS");
PROGMEM_STRING(Help, "HELP");

void help(CommandContext&& ctx) {
    auto names = terminal::names();

    std::sort(names.begin(), names.end(),
        [](StringView lhs, StringView rhs) {
            // XXX: Core's ..._P funcs only allow 2nd pointer to be in PROGMEM,
            //      explicitly load the 1st one
            // TODO: can we just assume linker already sorted all strings?
            return strncasecmp_P(
                lhs.toString().begin(), rhs.begin(), lhs.length()) < 0;
        });

    ctx.output.print(F("Available commands:\n"));
    for (auto name : names) {
        ctx.output.printf("> %s\n", name.c_str());
    }

    terminalOK(ctx);
}

PROGMEM_STRING(LightSleep, "SLEEP.LIGHT");

void light_sleep(CommandContext&& ctx) {
    if (ctx.argv.size() == 2) {
        using namespace espurna::duration;

        const auto result = parse(ctx.argv[1], std::micro{});
        if (!result.ok) {
            terminalError(ctx, F("Invalid time"));
            return;
        }

        const auto duration = to_chrono<sleep::Microseconds>(result.value);
        if (!instantLightSleep(duration)) {
            terminalError(ctx, F("Could not sleep"));
            return;
        }

        return;
    }

    instantLightSleep();
}

PROGMEM_STRING(DeepSleep, "SLEEP.DEEP");

void deep_sleep(CommandContext&& ctx) {
    if (ctx.argv.size() != 2) {
        terminalError(ctx, F("SLEEP.DEEP <TIME (MICROSECONDS)>"));
        return;
    }

    using namespace espurna::duration;
    const auto result = parse(ctx.argv[1], std::micro{});

    if (!result.ok) {
        terminalError(ctx, F("Invalid time"));
        return;
    }

    const auto duration = to_chrono<sleep::Microseconds>(result.value);
    if (!instantDeepSleep(duration)) {
        terminalError(ctx, F("Could not sleep"));
        return;
    }
}

PROGMEM_STRING(Reset, "RESET");

void reset(CommandContext&& ctx) {
    prepareReset(CustomResetReason::Terminal);
    terminalOK(ctx);
}

PROGMEM_STRING(EraseConfig, "ERASE.CONFIG");

void erase_config(CommandContext&& ctx) {
    terminalOK(ctx);
    customResetReason(CustomResetReason::Terminal);
    forceEraseSDKConfig();
}

PROGMEM_STRING(Heap, "HEAP");

void heap(CommandContext&& ctx) {
    const auto stats = systemHeapStats();
    ctx.output.printf_P(PSTR("initial: %lu available: %lu contiguous: %lu\n"),
            systemInitialFreeHeap(), stats.available, stats.usable);

    terminalOK(ctx);
}

PROGMEM_STRING(Uptime, "UPTIME");

void uptime(CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("uptime %s\n"),
        prettyDuration(systemUptime()).c_str());
    terminalOK(ctx);
}

PROGMEM_STRING(Info, "INFO");

void info(CommandContext&& ctx) {
    const auto app = buildApp();
    ctx.output.printf_P(PSTR("%s %s built %s\n"),
            app.name.c_str(), app.version.c_str(), app.build_time.c_str());

    ctx.output.printf_P(PSTR("device: %s\n"),
            systemDevice().c_str());
    ctx.output.printf_P(PSTR("mcu: esp8266 chipid: %s freq: %hhumhz\n"),
            systemChipId().c_str(), system_get_cpu_freq());

    const auto sdk = buildSdk();
    ctx.output.printf_P(PSTR("sdk: %s core: %s\n"),
            sdk.base.c_str(), sdk.version.c_str());
    ctx.output.printf_P(PSTR("md5: %s\n"), ESP.getSketchMD5().c_str());

    const auto modules = buildModules();
    ctx.output.printf_P(PSTR("support: %.*s\n"),
        modules.length(), modules.c_str());

#if SENSOR_SUPPORT
    const auto sensors = sensorList();
    ctx.output.printf_P(PSTR("sensors: %.*s\n"),
        sensors.length(), sensors.c_str());
#endif

#if SYSTEM_CHECK_ENABLED
    ctx.output.printf_P(PSTR("system: %s boot counter: %u\n"),
        systemCheck()
            ? PSTR("OK")
            : PSTR("UNSTABLE"),
        systemStabilityCounter());
#endif

#if DEBUG_SUPPORT
    crashResetReason(ctx.output);
#endif

    terminalOK(ctx);
}

struct Layout {
    Layout() = delete;

    constexpr Layout(const Layout&) = default;
    constexpr Layout(Layout&&) = default;
    constexpr Layout(const char* const name, uint32_t start, uint32_t end) :
        _name(name),
        _start(start),
        _end(end)
    {}

    constexpr uint32_t size() const {
        return _end - _start;
    }

    constexpr uint32_t start() const {
        return _start;
    }

    constexpr uint32_t end() const {
        return _end;
    }

    constexpr const char* name() const {
        return _name;
    }

private:
    const char* const _name;
    uint32_t _start;
    uint32_t _end;
};

struct Layouts {
    using List = std::forward_list<Layout>;

    Layouts() = delete;
    explicit Layouts(uint32_t size) :
        _size(size),
        _current(size),
        _sectors(size / SPI_FLASH_SEC_SIZE)
    {}

    const Layout* head() const {
        if (_list.empty()) {
            return nullptr;
        }

        return &_list.front();
    }

    bool lock() {
        if (_lock) {
            return true;
        }

        _lock = true;
        return false;
    }

    uint32_t sectors() const {
        return _sectors;
    }

    uint32_t size() const {
        return _size - _current;
    }

    uint32_t current() const {
        return _current;
    }

    Layouts& add(const char* const name, uint32_t size) {
        if (!_lock && _current >= size) {
            Layout layout(name, _current - size, _current);
            _current -= layout.size();
            _list.push_front(layout);
        }

        return *this;
    }

    template <typename T>
    void foreach(T&& callback) {
        for (auto& layout : _list) {
            callback(layout);
        }
    }

private:
    bool _lock { false };
    List _list;
    uint32_t _size;
    uint32_t _current;
    uint32_t _sectors;
};

StringView flash_chip_mode() {
    static const String out = ([]() -> String {
        switch (ESP.getFlashChipMode()) {
        case FM_DIO:
            return PSTR("DIO");
        case FM_DOUT:
            return PSTR("DOUT");
        case FM_QIO:
            return PSTR("QIO");
        case FM_QOUT:
            return PSTR("QOUT");
        case FM_UNKNOWN:
            break;
        }

        return PSTR("UNKNOWN");
    })();

    return out;
}

PROGMEM_STRING(Storage, "STORAGE");

void storage(CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("flash chip ID: 0x%06X\n"), ESP.getFlashChipId());
    ctx.output.printf_P(PSTR("speed: %u\n"), ESP.getFlashChipSpeed());
    ctx.output.printf_P(PSTR("mode: %s\n"), flash_chip_mode().c_str());

    ctx.output.printf_P(PSTR("size: %u (SPI), %u (SDK)\n"),
        ESP.getFlashChipRealSize(), ESP.getFlashChipSize());

    Layouts layouts(ESP.getFlashChipRealSize());

    // SDK specifies a hard-coded layout, there's no data beyond
    // (...addressable by the Core, since it adheres the setting)
    if (ESP.getFlashChipRealSize() > ESP.getFlashChipSize()) {
        layouts.add("unused", ESP.getFlashChipRealSize() - ESP.getFlashChipSize());
    }

    // app is at a normal location, [0...size), but... since it is offset by the free space, make sure it is aligned
    // to the sector size (...and it is expected from the getFreeSketchSpace, as the app will align to use the fixed
    // sector address for OTA writes).
    layouts.add("sdk", 4 * SPI_FLASH_SEC_SIZE);
    layouts.add("eeprom", eepromSpace());

    auto app_size = (ESP.getSketchSize() + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));
    auto ota_size = layouts.current() - app_size;

    // OTA is allowed to use all but one eeprom sectors that, leaving the last one
    // for the settings snapshot during the update
    layouts.add("ota", ota_size);
    layouts.add("app", app_size);

    layouts.foreach(
        [&](const Layout& layout) {
            ctx.output.printf_P("%-6s [%08X...%08X) (%u bytes)\n",
                    layout.name(), layout.start(), layout.end(), layout.size());
        });

    terminalOK(ctx);
}

PROGMEM_STRING(Adc, "ADC");

void adc(CommandContext&& ctx) {
    const int pin = (ctx.argv.size() == 2)
        ? ctx.argv[1].toInt()
        : A0;

    ctx.output.printf_P(PSTR("value %d\n"), analogRead(pin));
    terminalOK(ctx);
}

#if SYSTEM_CHECK_ENABLED
PROGMEM_STRING(Stable, "STABLE");

void stable(CommandContext&&) {
    systemForceStable();
    prepareReset(CustomResetReason::Stability);
}

PROGMEM_STRING(Unstable, "UNSTABLE");

void unstable(CommandContext&&) {
    systemForceUnstable();
    prepareReset(CustomResetReason::Stability);
}

PROGMEM_STRING(Trap, "TRAP");

void trap(CommandContext&&) {
    __builtin_trap();
}
#endif

static constexpr ::terminal::Command List[] PROGMEM {
    {Commands, commands::help},
    {Help, commands::help},

    {Info, commands::info},
    {Storage, commands::storage},
    {Uptime, commands::uptime},
    {Heap, commands::heap},

    {Adc, commands::adc},

    {LightSleep, commands::light_sleep},
    {DeepSleep, commands::deep_sleep},

    {Reset, commands::reset},
    {EraseConfig, commands::erase_config},

#if SYSTEM_CHECK_ENABLED
    {Stable, commands::stable},
    {Unstable, commands::unstable},
    {Trap, commands::trap},
#endif
};

void setup() {
    espurna::terminal::add(List);
}

} // namespace commands

#if TERMINAL_SERIAL_SUPPORT
namespace serial {

using LoopFunc = void(*)();
void empty_loop() {
}

namespace internal {

Stream* stream { nullptr };
LoopFunc loop { empty_loop };

} // namespace internal

void processing_loop() {
    using LineBuffer = LineBuffer<build::serialBufferSize()>;
    static LineBuffer buffer;

    auto& port = *internal::stream;
    const auto available = port.available();
    if (available <= 0) {
        return;
    }

    buffer.append(port, available);

    if (buffer.overflow()) {
        terminal::error(port, F("Serial buffer overflow"));
        buffer.reset();
    }

    for (;;) {
        const auto result = buffer.next();
        if (result.overflow) {
            terminal::error(port, F("Command line buffer overflow"));
            continue;
        }

        if (!result.value.length()) {
            break;
        }

        api_find_and_call(result.value, port);
    }
}

void loop() {
    internal::loop();
}

void setup() {
    auto port = uartPort(build::serialPort());
    if (!port || (!port->rx || !port->tx)) {
        return;
    }

    internal::stream = port->stream;
    internal::loop = processing_loop;
}

} // namespace serial
#endif

#if MQTT_SUPPORT && TERMINAL_MQTT_SUPPORT
namespace mqtt {

void setup() {
    mqttRegister([](unsigned int type, StringView topic, StringView payload) {
        if (type == MQTT_CONNECT_EVENT) {
            mqttSubscribe(MQTT_TOPIC_CMD);
            return;
        }

        if (type == MQTT_MESSAGE_EVENT) {
            auto t = mqttMagnitude(topic);
            if (!t.startsWith(MQTT_TOPIC_CMD)) {
                return;
            }

            if (!payload.length()) {
                return;
            }

            // TODO: unlike http handler, we have only one output stream
            //       and **must** have a fixed-size output buffer
            //       (wishlist: MQTT client does some magic and we don't buffer twice)
            // TODO: or, at least, make it growable on-demand and cap at MSS?
            // TODO: PrintLine<...> instead of one giant blob?

            auto ptr = std::make_shared<String>(payload.toString());

            espurnaRegisterOnce([ptr]() {
                PrintString out(TCP_MSS);
                api_find_and_call(*ptr, out);

                if (out.length()) {
                    static const auto topic = mqttTopic(MQTT_TOPIC_CMD);
                    mqttSendRaw(topic.c_str(), out.c_str(), false);
                }
            });

            return;
        }
    });

}

} // namespace mqtt
#endif

#if WEB_SUPPORT
namespace web {

STRING_VIEW_INLINE(Prefix, "cmd");

constexpr auto BufferHint = espurna::web::ws::InplaceLog::BufferHint;
using Output = PrintLine<espurna::web::ws::InplaceLog>;

struct Command {
    String line;
    uint32_t id;
};

void onVisible(JsonObject& root) {
    wsPayloadModule(root, Prefix);
}

void onAction(uint32_t client_id, const char* action, JsonObject& data) {
    STRING_VIEW_INLINE(Cmd, "cmd");
    if (Cmd != action) {
        return;
    }

    STRING_VIEW_INLINE(Line, "line");
    auto cmd = Command{
        .line = data[Line].as<String>(),
        .id = client_id,
    };

    if (!cmd.line.length()) {
        return;
    }

    const auto shared =
        std::make_shared<Command>(std::move(cmd));

    espurnaRegisterOnce([shared]() {
        wsPostManual(shared->id,
            [shared](JsonObject& root) {
                Output out(root, shared->id);
                out.output().wait_time(espurnaLoopDelay());
                api_find_and_call(shared->line, out);
            },
            BufferHint);
    });
}

void setup() {
    wsRegister()
        .onVisible(onVisible)
        .onAction(onAction);
}

} // namespace web
#endif

// -----------------------------------------------------------------------------
// Pubic API
// -----------------------------------------------------------------------------

#if TERMINAL_WEB_API_SUPPORT
namespace api {

STRING_VIEW_INLINE(Value, "line");

STRING_VIEW_INLINE(Path, TERMINAL_WEB_API_PATH);
STRING_VIEW_INLINE(Key, "termWebApiPath");

// XXX: new `apiRegister()` depends that `webServer()` is available, meaning we can't call this setup func
// before the `webSetup()` is called. ATM, just make sure it is in order.

using Commands = std::shared_ptr<std::vector<String>>;

#define API_UPDATE_COMMANDS(OUT, NAME, VALUE) \
    if (!OUT) {\
        OUT = std::make_shared<Commands::element_type>();\
    }\
\
    if (NAME == Value) {\
        (OUT)->push_back(VALUE);\
    }\

#define API_SCHEDULE_COMMANDS(REQUEST, COMMANDS) \
    espurna::web::print::scheduleFromRequest(\
        REQUEST,\
        [COMMANDS](Print& out) {\
            for (const auto& cmd : *COMMANDS) {\
                api_find_and_call(cmd, out);\
            }\
        });\

void setup() {
#if API_SUPPORT
    apiRegister(
        getSetting(Key, Path),
        [](ApiRequest& api) {
            api.handle([](AsyncWebServerRequest* request) {
                auto* response = request->beginResponseStream(F("text/plain"));
                for (const auto view : names()) {
                    const auto name = view.toString();
                    response->write(name.c_str(), name.length());
                    response->write("\r\n", 2);
                }

                request->send(response);
            });

            return true;
        },
        [](ApiRequest& api) {
            Commands cmds;

            api.param_foreach(
                [&](const String& name, const String& value) {
                    API_UPDATE_COMMANDS(cmds, name, value);
                });

            if (!cmds || !cmds->size()) {
                return false;
            }

            api.handle([cmds](AsyncWebServerRequest* request) {
                API_SCHEDULE_COMMANDS(request, cmds);
            });

            return true;
        }
    );
#else
    webRequestRegister([](AsyncWebServerRequest* request) {
        STRING_VIEW_INLINE(BasePath, API_BASE_PATH);

        String path;
        path += BasePath;
        path += getSetting(Key, Path);

        if (path != request->url()) {
            return false;
        }

        if (!apiAuthenticate(request)) {
            request->send(403);
            return true;
        }

        Commands cmds;

        for (size_t n = 0; n < request->params(); ++n) {
            const auto* param = request->getParam(n);
            const auto& name = param->name();
            const auto& value = param->value();
            if (!apiReservedParam(name)) {
                API_UPDATE_COMMANDS(cmds, name, value);
            }
        }

        if (!cmds || !cmds->size()) {
            request->send(500);
            return true;
        }

        API_SCHEDULE_COMMANDS(request, cmds);

        return true;
    });
#endif // API_SUPPORT
}

#undef API_SCHEDULE_COMMANDS
#undef API_UPDATE_COMMANDS

} // namespace api
#endif // TERMINAL_WEB_API_SUPPORT

void loop() {
#if TERMINAL_SERIAL_SUPPORT
    serial::loop();
#endif
}

void setup() {
#if TERMINAL_SERIAL_SUPPORT
    serial::setup();
#endif

#if WEB_SUPPORT
    // Show DEBUG panel with input
    web::setup();
#endif

#if MQTT_SUPPORT && TERMINAL_MQTT_SUPPORT
    // Similar to the above, accept cmdline(s) in payload
    mqtt::setup();
#endif

    // Initialize default commands
    commands::setup();

    // Register loop
    espurnaRegisterLoop(loop);
}

} // namespace
} // namespace terminal
} // namespace espurna

void terminalOK(const espurna::terminal::CommandContext& ctx) {
    espurna::terminal::ok(ctx);
}

void terminalError(const espurna::terminal::CommandContext& ctx) {
    espurna::terminal::error(ctx, String());
}

void terminalError(const espurna::terminal::CommandContext& ctx, const String& message) {
    espurna::terminal::error(ctx, message);
}

void terminalError(const espurna::terminal::CommandContext& ctx, const __FlashStringHelper* message) {
    espurna::terminal::error(ctx, String(message));
}

void terminalError(const espurna::terminal::CommandContext& ctx, espurna::StringView message) {
    espurna::terminal::error(ctx, message.toString());
}

void terminalRegisterCommand(espurna::terminal::Commands commands) {
    espurna::terminal::add(commands);
}

void terminalRegisterCommand(espurna::StringView name, espurna::terminal::CommandFunc func) {
    espurna::terminal::add(name, func);
}

#if TERMINAL_WEB_API_SUPPORT
void terminalWebApiSetup() {
    espurna::terminal::api::setup();
}
#endif

void terminalSetup() {
    espurna::terminal::setup();
}

#endif // TERMINAL_SUPPORT
