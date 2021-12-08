/*

DEBUG MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if DEBUG_SUPPORT

#include "settings.h"
#include "telnet.h"
#include "web.h"
#include "ntp.h"
#include "utils.h"
#include "ws.h"

#include <limits>
#include <type_traits>
#include <vector>

#if DEBUG_WEB_SUPPORT
#include <ArduinoJson.h>
#endif

#if DEBUG_UDP_SUPPORT
#include <WiFiUdp.h>
#endif

namespace settings {
namespace internal {

String serialize(DebugLogMode value) {
    String result;
    switch (value) {
    case DebugLogMode::Disabled:
        result = "0";
        break;
    case DebugLogMode::SkipBoot:
        result = "2";
        break;
    case DebugLogMode::Enabled:
        result = "1";
        break;
    }

    return result;
}

template<>
DebugLogMode convert(const String& value) {
    switch (value.toInt()) {
    case 0:
        return DebugLogMode::Disabled;
    case 2:
        return DebugLogMode::SkipBoot;
    case 1:
        break;
    }

    return DebugLogMode::Enabled;
}

} // namespace internal
} // namespace settings

namespace debug {
namespace {

struct Timestamp {
    Timestamp() = default;

    constexpr Timestamp(bool value) :
        _value(value)
    {}

    constexpr explicit operator bool() const {
        return _value;
    }

private:
    bool _value { false };
};

namespace build {

constexpr Timestamp AddTimestamp { 1 == DEBUG_ADD_TIMESTAMP };

constexpr bool coreDebug() {
#if defined(DEBUG_ESP_PORT) && !defined(NDEBUG)
    return true;
#else
    return false;
#endif
}

constexpr bool sdkDebug() {
    return false;
}

constexpr DebugLogMode mode() {
    return DEBUG_LOG_MODE;
}

constexpr bool buffer() {
    return 1 == DEBUG_LOG_BUFFER_ENABLED;
}

constexpr size_t bufferSize() {
    return DEBUG_LOG_BUFFER_SIZE;
}

} // namespace build

namespace settings {

bool sdkDebug() {
    return getSetting("dbgSDK", build::sdkDebug());
}

DebugLogMode mode() {
    return getSetting("dbgLogMode", build::mode());
}

bool buffer() {
    return getSetting("dbgLogBuf", build::buffer());
}

size_t bufferSize() {
    return getSetting("dbgLogBufSize", build::bufferSize());
}

espurna::heartbeat::Mode heartbeatMode() {
    return getSetting("dbgHbMode", espurna::heartbeat::currentMode());
}

espurna::duration::Seconds heartbeatInterval() {
    return getSetting("dbgHbIntvl", espurna::heartbeat::currentInterval());
}

} // namespace settings

namespace internal {

bool enabled { false };

} // namespace internal

bool enabled() {
    return internal::enabled;
}

void disable() {
    internal::enabled = false;
}

void enable() {
    internal::enabled = true;
}

void delayedEnable() {
    disable();
    schedule_function(enable);
}

void send(const char* message, size_t len, Timestamp);
void send(const char* message, size_t len) {
    send(message, len, build::AddTimestamp);
}

void formatAndSend(const char* format, va_list args) {
    constexpr size_t SmallStringBufferSize { 128 };
    char temp[SmallStringBufferSize];

    int len = vsnprintf_P(temp, sizeof(temp), format, args);
    if (len <= 0) {
        return;
    }

    // strlen(...) + '\0' already in temp buffer, avoid (explicit) dynamic memory when possible
    // (TODO: printf might still do it anyway internally?)
    if (static_cast<size_t>(len) < sizeof(temp)) {
        send(temp, len);
        return;
    }

    const size_t BufferSize { static_cast<size_t>(len) + 1 };
    auto* buffer = new (std::nothrow) char[BufferSize];
    if (!buffer) {
        return;
    }

    vsnprintf_P(buffer, BufferSize, format, args);
    send(buffer, len);
    delete[] buffer;
}

namespace buffer {
namespace internal {

bool enabled { false };
std::vector<char> storage;

} // namespace internal

size_t size() {
    return internal::storage.size();
}

size_t capacity() {
    return internal::storage.capacity();
}

void reserve(size_t size) {
    internal::storage.reserve(size);
}

bool enabled() {
    return internal::enabled;
}

void disable() {
    internal::enabled = false;
}

void enable() {
    internal::enabled = true;
}

void enable(size_t reserved) {
    enable();
    reserve(reserved);
}

template <typename T>
struct Handle {
    Handle() = delete;
    explicit Handle(T& lock) :
        _lock(lock)
    {
        _lock.lock();
    }

    ~Handle() {
        _lock.unlock();
    }

    explicit operator bool() const {
        return _lock;
    }

private:
    T& _lock;
};

struct Lock {
    Lock() = default;

    explicit operator bool() const {
        return _value;
    }

    Handle<Lock> handle() {
        return Handle<Lock>(*this);
    }

    bool lock() {
        if (!_value) {
            _value = true;
            return true;
        }

        return false;
    }

    bool unlock() {
        if (_value) {
            _value = false;
            return true;
        }

        return false;
    }

private:
    bool _value { false };
};

struct DebugLock {
    DebugLock() {
        if (debug::enabled()) {
            _changed = true;
            debug::disable();
        }
    }

    ~DebugLock() {
        if (_changed) {
            debug::enable();
        }
    }

private:
    bool _changed { false };
};

// Buffer data until we encounter line break, then flush via debug method
// (which is supposed to 1-to-1 copy the data, without adding the timestamp)
// TODO: abstract as `PrintLine`, so this becomes generic line buffering output for terminal as well?

namespace internal {

std::vector<char> line;

} // namespace internal

void sendBytes(const uint8_t* bytes, size_t size) {
    static Lock lock;
    if (lock) {
        return;
    }

    if (!size || ((size > 0) && bytes[size - 1] == '\0')) {
        return;
    }

    auto handle = lock.handle();
    if (internal::line.capacity() < (size + 2)) {
        internal::line.reserve(internal::line.size() + size + 2);
    }

    internal::line.insert(internal::line.end(),
        reinterpret_cast<const char*>(bytes),
        reinterpret_cast<const char*>(bytes) + size);

    if (internal::line.end() != std::find(internal::line.begin(), internal::line.end(), '\n')) {
        // TODO: ws and telnet still assume this is a c-string and will try to strlen this pointer
        auto len = internal::line.size();
        internal::line.push_back('\0');

        DebugLock debugLock;
        debug::send(internal::line.data(), len, Timestamp(false));

        internal::line.clear();
    }
}

// Longer recording of all log data. Stops when storage is filled, requires manual flushing.

void add(const char (&prefix)[10], const char* data, size_t len) {
    if (len > std::numeric_limits<uint16_t>::max()) {
        return;
    }

    size_t total { len };
    bool withPrefix { prefix[0] != '\0' };
    if (withPrefix) {
        total += sizeof(prefix) - 1;
    }

    if ((internal::storage.capacity() - internal::storage.size()) <= (total + 3)) {
        internal::enabled = false;
        return;
    }

    internal::storage.push_back(total >> 8);
    internal::storage.push_back(total & 0xff);

    if (withPrefix) {
        internal::storage.insert(internal::storage.end(), prefix, prefix + sizeof(prefix));
    }
    internal::storage.insert(internal::storage.end(), data, data + len);
}

void dump(Print& out) {
    size_t index = 0;
    do {
        if (index >= internal::storage.size()) {
            break;
        }

        size_t len = internal::storage[index] << 8;
        len = len | internal::storage[index + 1];
        index += 2;

        auto value = internal::storage[index + len];
        internal::storage[index + len] = '\0';
        out.print(internal::storage.data() + index);
        internal::storage[index + len] = value;

        index += len;
    } while (true);

    internal::storage.clear();
    internal::storage.shrink_to_fit();
}

} // namespace buffer

#if DEBUG_SERIAL_SUPPORT
namespace serial {

void output(Print& out, const char (&prefix)[10], const char* message, size_t len) {
    if (prefix[0] != '\0') {
        out.write(&prefix[0], sizeof(prefix) - 1);
    }
    out.write(message, len);
}

} // namespace serial
#endif

#if DEBUG_UDP_SUPPORT
namespace syslog {
namespace build {

IPAddress ip() {
    return DEBUG_UDP_IP;
}

constexpr uint16_t port() {
    return DEBUG_UDP_PORT;
};

constexpr bool enabled() {
    return port() == 514;
}

} // namespace build

namespace internal {

size_t len { 0 };
char header[128] = {0};
WiFiUDP udp;

} // namespace

// We use the syslog header as defined in RFC5424 (The Syslog Protocol), ref:
// - https://tools.ietf.org/html/rfc5424
// - https://github.com/xoseperez/espurna/issues/2312/

void configure() {
    snprintf_P(
        internal::header, sizeof(internal::header),
        PSTR("<%u>1 - %.31s ESPurna - - - "), DEBUG_UDP_FAC_PRI,
        getHostname().c_str());
}

bool output(const char* message, size_t len) {
    if (build::enabled() && wifiConnected()) {
        internal::udp.beginPacket(build::ip(), build::port());
        internal::udp.write(internal::header, internal::len);
        internal::udp.write(message, len);

        return internal::udp.endPacket() > 0;
    }

    return false;
}

} // namespace syslog
#endif

void send(const char* message, size_t len, Timestamp timestamp) {
    if (!message || !len) {
        return;
    }

    char prefix[10] = {0};
    static bool continue_timestamp = true;
    if (timestamp && continue_timestamp) {
        snprintf(prefix, sizeof(prefix), "[%06lu] ", millis() % 1000000);
    }

    continue_timestamp = static_cast<bool>(timestamp)
        || (message[len - 1] == '\r')
        || (message[len - 1] == '\n');

    bool pause { false };

#if DEBUG_SERIAL_SUPPORT
    serial::output(DEBUG_PORT, prefix, message, len);
#endif

#if DEBUG_UDP_SUPPORT
    pause = syslog::output(message, len);
#endif

#if DEBUG_TELNET_SUPPORT
    pause = telnetDebugSend(prefix, message) || pause;
#endif

#if DEBUG_WEB_SUPPORT
    pause = wsDebugSend(prefix, message) || pause;
#endif

#if DEBUG_LOG_BUFFER_SUPPORT
    buffer::add(prefix, message, len);
#endif

    if (pause) {
        optimistic_yield(1000);
    }
}

// -----------------------------------------------------------------------------

#if DEBUG_WEB_SUPPORT
namespace web {

#if TERMINAL_SUPPORT
void onAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "dbgcmd") != 0) {
        return;
    }

    if (!data.containsKey("command") || !data["command"].is<const char*>()) {
        return;
    }

    const char* command = data["command"];
    auto len = command ? strlen(command) : 0ul;

    terminalInject(command, len);
    terminalInject('\n');
}
#endif

void onVisible(JsonObject& root) {
    wsPayloadModule(root, "dbg");
}

} // namespace web
#endif

// -----------------------------------------------------------------------------

bool status(espurna::heartbeat::Mask mask) {
    if (mask & espurna::heartbeat::Report::Uptime) {
        debugSend(PSTR("[MAIN] Uptime: %s\n"), getUptime().c_str());
    }

    if (mask & espurna::heartbeat::Report::Freeheap) {
        const auto stats = systemHeapStats();
        debugSend(PSTR("[MAIN] Heap: initial %5lu available %5lu contiguous %5hu\n"),
            systemInitialFreeHeap(), stats.available, stats.usable);
    }

    if ((mask & espurna::heartbeat::Report::Vcc) && (ADC_MODE_VALUE == ADC_VCC)) {
        debugSend(PSTR("[MAIN] VCC: %lu mV\n"), ESP.getVcc());
    }

#if NTP_SUPPORT
    if ((mask & espurna::heartbeat::Report::Datetime) && ntpSynced()) {
        debugSend(PSTR("[MAIN] Datetime: %s\n"), ntpDateTime().c_str());
    }
#endif

    return true;
}

void configure() {
    // HardwareSerial::begin() will automatically enable this when
    // `#if defined(DEBUG_ESP_PORT) && !defined(NDEBUG)`
    // Do not interfere when that is the case
    if (!build::coreDebug()) {
        DEBUG_PORT.setDebugOutput(settings::sdkDebug());
    }

    // Make sure other modules are aware of used GPIOs
    // TODO: external port config, without using Arduino globals, will fix this diagnostics mess...
#if DEBUG_SERIAL_SUPPORT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored  "-Wpragmas"
#pragma GCC diagnostic ignored  "-Wtautological-compare"
    if (&(DEBUG_PORT) == &Serial) {
        gpioLock(1);
        gpioLock(3);
    } else if (&(DEBUG_PORT) == &Serial1) {
        gpioLock(2);
    }
#pragma GCC diagnostic pop
#endif

#if DEBUG_LOG_BUFFER_SUPPORT
    if (settings::buffer()) {
        debug::buffer::enable(settings::bufferSize());
    }
#endif

    ::systemHeartbeat(status,
        settings::heartbeatMode(),
        settings::heartbeatInterval());
}

void onBoot() {
    static_assert(
        std::is_same<int, std::underlying_type<DebugLogMode>::type>::value,
        "should be able to match DebugLogMode with int"
    );

    switch (settings::mode()) {
    case DebugLogMode::SkipBoot:
        debug::delayedEnable();
        break;
    case DebugLogMode::Disabled:
        debug::disable();
        break;
    case DebugLogMode::Enabled:
        debug::enable();
        break;
    }

    configure();
}

} // namespace
} // namespace debug

void debugSendBytes(const uint8_t* bytes, size_t size) {
    debug::buffer::sendBytes(bytes, size);
}

#if DEBUG_LOG_BUFFER_SUPPORT
bool debugLogBuffer() {
    return debug::buffer::enabled();
}
#endif

void debugSend(const char* format, ...) {
    if (debug::enabled()) {
        va_list args;
        va_start(args, format);
        debug::formatAndSend(format, args);
        va_end(args);
    }
}

void debugConfigureBoot() {
    debug::onBoot();
}

#if WEB_SUPPORT
void debugWebSetup() {
    wsRegister()
        .onVisible(debug::web::onVisible)
        .onAction(debug::web::onAction);
}
#endif

void debugSetup() {
#if DEBUG_SERIAL_SUPPORT
    DEBUG_PORT.begin(SERIAL_BAUDRATE);
#endif

#if DEBUG_UDP_SUPPORT
    if (debug::syslog::build::enabled()) {
        debug::syslog::configure();
        espurnaRegisterReload(debug::syslog::configure);
    }
#endif

#if DEBUG_LOG_BUFFER_SUPPORT
#if TERMINAL_SUPPORT
    terminalRegisterCommand(F("DEBUG.BUFFER"), [](::terminal::CommandContext&& ctx) {
        debug::buffer::disable();
        if (!debug::buffer::size()) {
            terminalError(ctx, F("buffer is empty\n"));
            return;
        }

        ctx.output.printf_P(PSTR("buffer size: %u / %u bytes\n"),
            debug::buffer::size(), debug::buffer::capacity());
        debug::buffer::dump(ctx.output);
        terminalOK(ctx);
    });
#endif
#endif
}

#endif // DEBUG_SUPPORT
