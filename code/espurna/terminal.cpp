/*

TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if TERMINAL_SUPPORT

#include "api.h"
#include "crash.h"
#include "mqtt.h"
#include "settings.h"
#include "system.h"
#include "telnet.h"
#include "terminal.h"
#include "utils.h"
#include "wifi.h"
#include "ws.h"

#include "libs/URL.h"
#include "libs/StreamAdapter.h"
#include "libs/PrintString.h"

#include "web_asyncwebprint.ipp"

#include <algorithm>
#include <utility>

#include <Schedule.h>
#include <Stream.h>

// not yet CONNECTING or LISTENING
extern struct tcp_pcb *tcp_bound_pcbs;
// accepting or sending data
extern struct tcp_pcb *tcp_active_pcbs;
// // TIME-WAIT status
extern struct tcp_pcb *tcp_tw_pcbs;

namespace {

// Based on libs/StreamInjector.h by Xose Pérez <xose dot perez at gmail dot com> (see git-log for more info)
// Instead of custom write(uint8_t) callback, we provide writer implementation in-place

template <size_t Capacity>
struct TerminalIO final : public Stream {
    using Buffer = std::array<char, Capacity>;

    // ---------------------------------------------------------------------
    // Stream part of the interface injects data into the internal buffer,
    // so we can later use the ::read()
    // ---------------------------------------------------------------------

    static constexpr size_t capacity() {
        return Capacity;
    }

    size_t inject(char ch) {
        _buffer[_write] = ch;
        _write = (_write + 1) % Capacity;
        return 1;
    }

    size_t inject(const char* data, size_t len) {
        for (size_t index = 0; index < len; ++index) {
            inject(data[index]);
        }
        return len;
    }

    // ---------------------------------------------------------------------
    // XXX: We are only supporting part of the Print & Stream interfaces
    //      But, we need to be have all pure virtual methods implemented
    // ---------------------------------------------------------------------

    // Return data from the internal buffer
    // Note that this cannot be negative, but the API requires `int`
    int available() override {
        size_t bytes = 0;
        if (_read > _write) {
            bytes += (_write - _read + Capacity);
        } else if (_read < _write) {
            bytes += (_write - _read);
        }
        return bytes;
    }

    int peek() override {
        int ch = -1;
        if (_read != _write) {
            ch = _buffer[_read];
        }
        return ch;
    }

    int read() override {
        int ch = -1;
        if (_read != _write) {
            ch = _buffer[_read];
            _read = (_read + 1) % Capacity;
        }
        return ch;
    }

    // {Stream,Print}::flush(), see:
    // - https://github.com/esp8266/Arduino/blob/master/cores/esp8266/Print.h
    // - https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/Print.h
    // - https://github.com/arduino/ArduinoCore-API/issues/102
    // Old 2.3.0 expects flush() on Stream, latest puts in in Print
    // We may have to cheat the system and implement everything as Stream to have it available.
    void flush() override {
        // Here, reset reader position so that we return -1 until we have new data
        // writer flushing is implemented below, we don't need it here atm
        _read = _write;
    }

    // TODO: allow DEBUG_SUPPORT=0 :/
    size_t write(const uint8_t* bytes, size_t size) override {
#if DEBUG_SUPPORT
        debugSendBytes(bytes, size);
#endif
        return size;
    }

    size_t write(uint8_t ch) override {
        uint8_t buffer[1] {ch};
        return write(buffer, 1);
    }

private:
    Buffer _buffer {};
    size_t _write { 0ul };
    size_t _read { 0ul };
};

constexpr size_t _terminalBufferSize() {
    return TERMINAL_SHARED_BUFFER_SIZE;
}

using Io = TerminalIO<_terminalBufferSize()>;

Io _io;
terminal::Terminal _terminal(_io, Io::capacity());

// TODO: re-evaluate how and why this is used
#if SERIAL_RX_ENABLED

constexpr size_t SerialRxBufferSize { 128u };
char _serial_rx_buffer[SerialRxBufferSize];
unsigned char _serial_rx_pointer = 0;

#endif // SERIAL_RX_ENABLED

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

void _terminalHelpCommand(::terminal::CommandContext&& ctx) {
    auto names = _terminal.names();

    // XXX: Core's ..._P funcs only allow 2nd pointer to be in PROGMEM,
    //      explicitly load the 1st one
    std::sort(names.begin(), names.end(), [](const __FlashStringHelper* lhs, const __FlashStringHelper* rhs) {
        const String lhs_as_string(lhs);
        return strncasecmp_P(lhs_as_string.c_str(), reinterpret_cast<const char*>(rhs), lhs_as_string.length()) < 0;
    });

    ctx.output.print(F("Available commands:\n"));
    for (auto* name : names) {
        ctx.output.printf("> %s\n", reinterpret_cast<const char*>(name));
    }

    terminalOK(ctx.output);
}

namespace dns {

using Callback = std::function<void(const char* name, const ip_addr_t* addr, void* arg)>;

namespace internal {

struct Task {
    Task() = delete;
    explicit Task(String&& hostname, Callback&& callback) :
        _hostname(std::move(hostname)),
        _callback(std::move(callback))
    {}

    ip_addr_t* addr() {
        return &_addr;
    }

    const char* hostname() const {
        return _hostname.c_str();
    }

    void callback(const char* name, const ip_addr_t* addr, void* arg) {
        _callback(name, addr, arg);
    }

    void callback() {
        _callback(hostname(), addr(), nullptr);
    }

private:
    String _hostname;
    Callback _callback;
    ip_addr_t _addr { IPADDR_NONE };
};

using TaskPtr = std::unique_ptr<Task>;
TaskPtr task;

void callback(const char* name, const ip_addr_t* addr, void* arg) {
    if (task) {
        task->callback(name, addr, arg);
    }
    task.reset();
}

} // namespace internal

bool started() {
    return static_cast<bool>(internal::task);
}

void start(String&& hostname, Callback&& callback) {
    auto task = std::make_unique<internal::Task>(std::move(hostname), std::move(callback));

    switch (dns_gethostbyname(task->hostname(), task->addr(), internal::callback, nullptr)) {
    case ERR_OK:
        task->callback();
        break;
    case ERR_INPROGRESS:
        internal::task = std::move(task);
        break;
    default:
        break;
    }
}

} // namespace dns

extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;

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

void _terminalInitCommands() {

    terminalRegisterCommand(F("COMMANDS"), _terminalHelpCommand);
    terminalRegisterCommand(F("HELP"), _terminalHelpCommand);

    terminalRegisterCommand(F("ERASE.CONFIG"), [](::terminal::CommandContext&&) {
        terminalOK();
        customResetReason(CustomResetReason::Terminal);
        forceEraseSDKConfig();
    });

    terminalRegisterCommand(F("ADC"), [](::terminal::CommandContext&& ctx) {
        const int pin = (ctx.argv.size() == 2)
            ? ctx.argv[1].toInt()
            : A0;

        ctx.output.printf_P(PSTR("value %d\n"), analogRead(pin));
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("GPIO"), [](::terminal::CommandContext&& ctx) {
        const int pin = (ctx.argv.size() >= 2)
            ? ctx.argv[1].toInt()
            : -1;

        if ((pin >= 0) && !gpioValid(pin)) {
            terminalError(ctx, F("Invalid pin number"));
            return;
        }

        int start = 0;
        int end = gpioPins();

        switch (ctx.argv.size()) {
        case 3:
            pinMode(pin, OUTPUT);
            digitalWrite(pin, (1 == ctx.argv[2].toInt()));
            break;
        case 2:
            start = pin;
            end = pin + 1;
            // fallthrough into print
        case 1:
            for (auto current = start; current < end; ++current) {
                if (gpioValid(current)) {
                    ctx.output.printf_P(PSTR("%c %s @ GPIO%02d (%s)\n"),
                        gpioLocked(current) ? '*' : ' ',
                        GPEP(current) ? "OUTPUT" : "INPUT ",
                        current,
                        (HIGH == digitalRead(current)) ? "HIGH" : "LOW"
                    );
                }
            }
            break;
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("HEAP"), [](::terminal::CommandContext&& ctx) {
        const auto stats = systemHeapStats();
        ctx.output.printf_P(PSTR("initial: %lu available: %lu contiguous: %hu\n"),
            systemInitialFreeHeap(), stats.available, stats.usable);

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("STACK"), [](::terminal::CommandContext&& ctx) {
        ctx.output.printf_P(PSTR("continuation stack initial: %d, free: %u\n"),
            CONT_STACKSIZE, systemFreeStack());
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("INFO"), [](::terminal::CommandContext&& ctx) {
        ctx.output.printf_P(PSTR("%s %s built %s\n"), getAppName(), getVersion(), buildTime().c_str());
        ctx.output.printf_P(PSTR("mcu: esp8266 chipid: %s freq: %hhumhz\n"), getFullChipId().c_str(), system_get_cpu_freq());
        ctx.output.printf_P(PSTR("sdk: %s core: %s\n"),
                ESP.getSdkVersion(), getCoreVersion().c_str());
        ctx.output.printf_P(PSTR("md5: %s\n"), ESP.getSketchMD5().c_str());
        ctx.output.printf_P(PSTR("support: %s\n"), getEspurnaModules());
#if SENSOR_SUPPORT
        ctx.output.printf_P(PSTR("sensors: %s\n"), getEspurnaSensors());
#endif
#if SYSTEM_CHECK_ENABLED
        ctx.output.printf_P(PSTR("system: %s boot counter: %u\n"),
            systemCheck() ? PSTR("OK") : PSTR("UNSTABLE"), systemStabilityCounter());
#endif
#if DEBUG_SUPPORT
        crashResetReason(ctx.output);
#endif
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("STORAGE"), [](::terminal::CommandContext&& ctx) {
        ctx.output.printf_P(PSTR("flash chip ID: 0x%06X\n"), ESP.getFlashChipId());
        ctx.output.printf_P(PSTR("speed: %u\n"), ESP.getFlashChipSpeed());
        ctx.output.printf_P(PSTR("mode: %s\n"), getFlashChipMode());

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

        layouts.foreach([&](const Layout& layout) {
            ctx.output.printf_P("%-6s [%08X...%08X) (%u bytes)\n",
                    layout.name(), layout.start(), layout.end(), layout.size());
        });

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("RESET"), [](::terminal::CommandContext&& ctx) {
        auto count = 1;
        if (ctx.argv.size() == 2) {
            count = ctx.argv[1].toInt();
            if (count < SYSTEM_CHECK_MAX) {
                systemStabilityCounter(count);
            }
        }

        terminalOK(ctx);
        prepareReset(CustomResetReason::Terminal);
    });

    terminalRegisterCommand(F("UPTIME"), [](::terminal::CommandContext&& ctx) {
        ctx.output.printf_P(PSTR("uptime %s\n"), getUptime().c_str());
        terminalOK(ctx);
    });

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
    terminalRegisterCommand(F("MFLN.PROBE"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() != 3) {
            terminalError(ctx, F("<url> <value>"));
            return;
        }

        URL _url(std::move(ctx.argv[1]));
        uint16_t requested_mfln = atol(ctx.argv[2].c_str());

        auto client = std::make_unique<BearSSL::WiFiClientSecure>();
        client->setInsecure();

        if (client->probeMaxFragmentLength(_url.host.c_str(), _url.port, requested_mfln)) {
            terminalOK(ctx);
            return;
        }

        terminalError(ctx, F("Buffer size not supported"));
    });
#endif

    terminalRegisterCommand(F("HOST"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() != 2) {
            terminalError(ctx, F("<hostname>"));
            return;
        }

        dns::start(std::move(ctx.argv[1]), [&](const char* name, const ip_addr_t* addr, void*) {
            if (!addr) {
                ctx.output.printf_P(PSTR("%s not found\n"), name);
                return;
            }

            ctx.output.printf_P(PSTR("%s has address %s\n"),
                name, IPAddress(addr).toString().c_str());
        });

        while (dns::started()) {
            delay(100);
        }
    });

    terminalRegisterCommand(F("NETSTAT"), [](::terminal::CommandContext&& ctx) {
        auto print = [](Print& out, tcp_pcb* list) {
            for (tcp_pcb* pcb = list; pcb != nullptr; pcb = pcb->next) {
                out.printf_P(PSTR("state %s local %s:%hu remote %s:%hu\n"),
                        tcp_debug_state_str(pcb->state),
                        IPAddress(pcb->local_ip).toString().c_str(),
                        pcb->local_port,
                        IPAddress(pcb->remote_ip).toString().c_str(),
                        pcb->remote_port);
            }
        };

        print(ctx.output, tcp_active_pcbs);
        print(ctx.output, tcp_tw_pcbs);
        print(ctx.output, tcp_bound_pcbs);
    });
}

void _terminalLoop() {

    // TODO: custom Stream input, don't depend on debug logging output as input
#if DEBUG_SERIAL_SUPPORT
    while (DEBUG_PORT.available()) {
        _io.inject(DEBUG_PORT.read());
    }
#endif

    _terminal.process([](terminal::Terminal::Result result) {
        bool out = false;
        switch (result) {
            case terminal::Terminal::Result::CommandNotFound:
                terminalError(terminalDefaultStream(), F("Command not found"));
                out = true;
                break;
            case terminal::Terminal::Result::BufferOverflow:
                terminalError(terminalDefaultStream(), F("Command line buffer overflow"));
                out = true;
                break;
            case terminal::Terminal::Result::Command:
                out = true;
                break;
            case terminal::Terminal::Result::Pending:
                out = false;
                break;
            case terminal::Terminal::Result::Error:
                terminalError(terminalDefaultStream(), F("Unexpected error when parsing command line"));
                out = false;
                break;
            case terminal::Terminal::Result::NoInput:
                out = false;
                break;
        }
        return out;
    });

    #if SERIAL_RX_ENABLED

        while (SERIAL_RX_PORT.available() > 0) {
            char rc = SERIAL_RX_PORT.read();
            _serial_rx_buffer[_serial_rx_pointer++] = rc;
            if ((_serial_rx_pointer == SerialRxBufferSize) || (rc == 10)) {
                terminalInject(_serial_rx_buffer, (size_t) _serial_rx_pointer);
                _serial_rx_pointer = 0;
            }
        }

    #endif // SERIAL_RX_ENABLED

}

#if MQTT_SUPPORT && TERMINAL_MQTT_SUPPORT

void _terminalMqttSetup() {

    mqttRegister([](unsigned int type, const char* topic, char* payload) {
        if (type == MQTT_CONNECT_EVENT) {
            mqttSubscribe(MQTT_TOPIC_CMD);
            return;
        }

        if (type == MQTT_MESSAGE_EVENT) {
            String t = mqttMagnitude(topic);
            if (!t.startsWith(MQTT_TOPIC_CMD)) return;
            if (!strlen(payload)) return;

            String cmd(payload);
            if (!cmd.endsWith("\r\n") && !cmd.endsWith("\n")) {
                cmd += '\n';
            }

            // TODO: unlike http handler, we have only one output stream
            //       and **must** have a fixed-size output buffer
            //       (wishlist: MQTT client does some magic and we don't buffer twice)
            schedule_function([cmd]() {
                PrintString buffer(TCP_MSS);
                StreamAdapter<const char*> stream(buffer, cmd.c_str(), cmd.c_str() + cmd.length() + 1);

                String out;
                terminal::Terminal handler(stream);
                switch (handler.processLine()) {
                case terminal::Terminal::Result::CommandNotFound:
                    out += F("Command not found");
                    break;
                case terminal::Terminal::Result::Command:
                    out = std::move(buffer);
                default:
                    break;
                }

                if (out.length()) {
                    mqttSendRaw(mqttTopic(MQTT_TOPIC_CMD, false).c_str(), out.c_str(), false);
                }
            });

            return;
        }
    });

}

#endif // MQTT_SUPPORT && TERMINAL_MQTT_SUPPORT

#if WEB_SUPPORT

void _terminalWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "cmd");
}

#endif

} // namespace

// -----------------------------------------------------------------------------
// Pubic API
// -----------------------------------------------------------------------------

#if TERMINAL_WEB_API_SUPPORT

// XXX: new `apiRegister()` depends that `webServer()` is available, meaning we can't call this setup func
// before the `webSetup()` is called. ATM, just make sure it is in order.

void terminalWebApiSetup() {
#if API_SUPPORT
    apiRegister(getSetting("termWebApiPath", TERMINAL_WEB_API_PATH),
        [](ApiRequest& api) {
            api.handle([](AsyncWebServerRequest* request) {
                AsyncResponseStream *response = request->beginResponseStream("text/plain");
                for (auto* name : _terminal.names()) {
                    response->print(name);
                    response->print("\r\n");
                }

                request->send(response);
            });
            return true;
        },
        [](ApiRequest& api) {
            // TODO: since HTTP spec allows query string to contain repeating keys, allow iteration
            // over every received 'line' to provide a way to call multiple commands at once
            auto cmd = api.param(F("line"));
            if (!cmd.length()) {
                return false;
            }

            if (!cmd.endsWith("\r\n") && !cmd.endsWith("\n")) {
                cmd += '\n';
            }

            api.handle([&](AsyncWebServerRequest* request) {
                AsyncWebPrint::scheduleFromRequest(request, [cmd](Print& print) {
                    StreamAdapter<const char*> stream(print, cmd.c_str(), cmd.c_str() + cmd.length() + 1);
                    terminal::Terminal handler(stream);
                    handler.processLine();
                });
            });

            return true;
        }
    );
#else
    webRequestRegister([](AsyncWebServerRequest* request) {
        String path(F(API_BASE_PATH));
        path += getSetting("termWebApiPath", TERMINAL_WEB_API_PATH);
        if (path != request->url()) {
            return false;
        }

        if (!apiAuthenticate(request)) {
            request->send(403);
            return true;
        }

        auto* cmd_param = request->getParam("line", (request->method() == HTTP_PUT));
        if (!cmd_param) {
            request->send(500);
            return true;
        }

        auto cmd = cmd_param->value();
        if (!cmd.length()) {
            request->send(500);
            return true;
        }

        if (!cmd.endsWith("\r\n") && !cmd.endsWith("\n")) {
            cmd += '\n';
        }

        // TODO: batch requests? processLine() -> process(...)
        AsyncWebPrint::scheduleFromRequest(request, [cmd](Print& print) {
            StreamAdapter<const char*> stream(print, cmd.c_str(), cmd.c_str() + cmd.length() + 1);
            terminal::Terminal handler(stream);
            handler.processLine();
        });

        return true;
    });
#endif // API_SUPPORT
}

#endif // TERMINAL_WEB_API_SUPPORT

Stream& terminalDefaultStream() {
    return (Stream &) _io;
}

size_t terminalCapacity() {
    return Io::capacity();
}

void terminalInject(const char* data, size_t len) {
    _io.inject(data, len);
}

void terminalInject(char ch) {
    _io.inject(ch);
}

void terminalRegisterCommand(const __FlashStringHelper* name, terminal::Terminal::CommandFunc func) {
    terminal::Terminal::addCommand(name, func);
};

void terminalOK(Print& print) {
    print.print(F("+OK\n"));
}

void terminalError(Print& print, const String& error) {
    print.printf_P(PSTR("-ERROR: %s\n"), error.c_str());
}

void terminalOK(const ::terminal::CommandContext& ctx) {
    terminalOK(ctx.output);
}

void terminalError(const ::terminal::CommandContext& ctx, const String& error) {
    terminalError(ctx.output, error);
}

void terminalOK() {
    terminalOK(_io);
}

void terminalError(const String& error) {
    terminalError(_io, error);
}

void terminalSetup() {

    // Show DEBUG panel with input
    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_terminalWebSocketOnVisible);
    #endif

    // Similar to the above, but we allow only very small and in-place outputs.
    #if MQTT_SUPPORT && TERMINAL_MQTT_SUPPORT
        _terminalMqttSetup();
    #endif

    // Initialize default commands
    _terminalInitCommands();

    #if SERIAL_RX_ENABLED
        SERIAL_RX_PORT.begin(SERIAL_RX_BAUDRATE);
    #endif // SERIAL_RX_ENABLED

    // Register loop
    espurnaRegisterLoop(_terminalLoop);

}

#endif // TERMINAL_SUPPORT

