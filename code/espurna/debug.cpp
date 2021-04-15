/*

DEBUG MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if DEBUG_SUPPORT

#include <limits>
#include <type_traits>
#include <vector>

#include "settings.h"
#include "telnet.h"
#include "web.h"
#include "ntp.h"
#include "utils.h"
#include "ws.h"

#if DEBUG_WEB_SUPPORT
#include <ArduinoJson.h>
#endif

#if DEBUG_UDP_SUPPORT
#include <WiFiUdp.h>
WiFiUDP _udp_debug;

constexpr bool _udp_syslog_enabled = (514 == DEBUG_UDP_PORT);
char _udp_syslog_header[64];
#endif

bool _debug_enabled = false;

// -----------------------------------------------------------------------------
// printf-like debug methods
// -----------------------------------------------------------------------------

constexpr int DEBUG_SEND_STRING_BUFFER_SIZE = 128;

void _debugSendInternal(const char * message, bool add_timestamp = DEBUG_ADD_TIMESTAMP);

// TODO: switch to newlib vsnprintf for latest Cores to support PROGMEM args
void _debugSend(const char * format, va_list args) {

    char temp[DEBUG_SEND_STRING_BUFFER_SIZE];
    int len = vsnprintf(temp, sizeof(temp), format, args);

    // strlen(...) + '\0' already in temp buffer, avoid using malloc when possible
    if (len < DEBUG_SEND_STRING_BUFFER_SIZE) {
        _debugSendInternal(temp);
        return;
    }

    len += 1;
    auto* buffer = static_cast<char*>(malloc(len));
    if (!buffer) {
        return;
    }
    vsnprintf(buffer, len, format, args);

    _debugSendInternal(buffer);
    free(buffer);

}

void debugSendRaw(const char* line, bool timestamp) {
    if (_debug_enabled) {
        _debugSendInternal(line, timestamp);
    }
}

// Buffer data until we encounter line break, then flush via Raw debug method
// (which is supposed to 1-to-1 copy the data, without adding the timestamp)

namespace {
std::vector<char> _dbg_raw_output;
}

void debugSendBytes(const uint8_t* bytes, size_t size) {
    static bool lock { false };
    if (lock) {
        return;
    }

    if (!size || ((size > 0) && bytes[size - 1] == '\0')) {
        return;
    }

    lock = true;

    if (_dbg_raw_output.capacity() < (size + 2)) {
        _dbg_raw_output.reserve(_dbg_raw_output.size() + size + 2);
    }
    _dbg_raw_output.insert(_dbg_raw_output.end(),
        reinterpret_cast<const char*>(bytes),
        reinterpret_cast<const char*>(bytes) + size);

    if (_dbg_raw_output.end() != std::find(_dbg_raw_output.begin(), _dbg_raw_output.end(), '\n')) {
        _dbg_raw_output.push_back('\0');
        debugSendRaw(_dbg_raw_output.data());
        _dbg_raw_output.clear();
    }

    lock = false;
}

void debugSend(const char* format, ...) {
    if (!_debug_enabled) {
        return;
    }

    va_list args;
    va_start(args, format);

    _debugSend(format, args);

    va_end(args);
}

void debugSend_P(const char* format_P, ...) {
    if (!_debug_enabled) {
        return;
    }

    char format[strlen_P(format_P) + 1];
    memcpy_P(format, format_P, sizeof(format));

    va_list args;
    va_start(args, format_P);

    _debugSend(format, args);

    va_end(args);
}

// -----------------------------------------------------------------------------
// specific debug targets
// -----------------------------------------------------------------------------

#if DEBUG_SERIAL_SUPPORT
    void _debugSendSerial(const char* prefix, const char* data) {
        if (prefix && (prefix[0] != '\0')) {
            DEBUG_PORT.print(prefix);
        }
        DEBUG_PORT.print(data);
    }
#endif // DEBUG_SERIAL_SUPPORT

#if DEBUG_LOG_BUFFER_SUPPORT

std::vector<char> _debug_log_buffer;
bool _debug_log_buffer_enabled = false;

void _debugLogBuffer(const char* prefix, const char* data) {
    if (!_debug_log_buffer_enabled) return;

    const auto prefix_len = strlen(prefix);
    const auto data_len = strlen(data);
    const auto total_len = prefix_len + data_len;
    if (total_len >= std::numeric_limits<uint16_t>::max()) {
        return;
    }
    if ((_debug_log_buffer.capacity() - _debug_log_buffer.size()) <= (total_len + 3)) {
        _debug_log_buffer_enabled = false;
        return;
    }

    _debug_log_buffer.push_back(total_len >> 8);
    _debug_log_buffer.push_back(total_len & 0xff);
    if (prefix && (prefix[0] != '\0')) {
        _debug_log_buffer.insert(_debug_log_buffer.end(), prefix, prefix + prefix_len);
    }
    _debug_log_buffer.insert(_debug_log_buffer.end(), data, data + data_len);
}

void _debugLogBufferDump(Print& out) {
    size_t index = 0;
    do {
        if (index >= _debug_log_buffer.size()) {
            break;
        }

        size_t len = _debug_log_buffer[index] << 8;
        len = len | _debug_log_buffer[index + 1];
        index += 2;

        auto value = _debug_log_buffer[index + len];
        _debug_log_buffer[index + len] = '\0';
        out.print(_debug_log_buffer.data() + index);
        _debug_log_buffer[index + len] = value;

        index += len;
    } while (true);

    _debug_log_buffer.clear();
    _debug_log_buffer.shrink_to_fit();
}

bool debugLogBuffer() {
    return _debug_log_buffer_enabled;
}

#endif // DEBUG_LOG_BUFFER_SUPPORT

// -----------------------------------------------------------------------------

void _debugSendInternal(const char * message, bool add_timestamp) {

    const size_t msg_len = strlen(message);

    bool pause = false;
    char timestamp[10] = {0};

    #if DEBUG_ADD_TIMESTAMP
        static bool continue_timestamp = true;
        if (add_timestamp && continue_timestamp) {
            snprintf(timestamp, sizeof(timestamp), "[%06lu] ", millis() % 1000000);
        }
        continue_timestamp = add_timestamp || (message[msg_len - 1] == 10) || (message[msg_len - 1] == 13);
    #endif

    #if DEBUG_SERIAL_SUPPORT
        _debugSendSerial(timestamp, message);
    #endif

    #if DEBUG_UDP_SUPPORT
        #if SYSTEM_CHECK_ENABLED
        if (systemCheck()) {
        #endif
            _udp_debug.beginPacket(DEBUG_UDP_IP, DEBUG_UDP_PORT);
            if (_udp_syslog_enabled) {
                _udp_debug.write(_udp_syslog_header);
            }
            _udp_debug.write(message);
            pause = _udp_debug.endPacket() > 0;
        #if SYSTEM_CHECK_ENABLED
        }
        #endif
    #endif

    #if DEBUG_TELNET_SUPPORT
        pause = telnetDebugSend(timestamp, message) || pause;
    #endif

    #if DEBUG_WEB_SUPPORT
        pause = wsDebugSend(timestamp, message) || pause;
    #endif

    #if DEBUG_LOG_BUFFER_SUPPORT
        _debugLogBuffer(timestamp, message);
    #endif

    if (pause) {
        optimistic_yield(1000);
    }

}

// -----------------------------------------------------------------------------

#if DEBUG_WEB_SUPPORT

#if TERMINAL_SUPPORT
void _debugWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
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

void debugWebSetup() {

    wsRegister()
#if TERMINAL_SUPPORT
        .onAction(_debugWebSocketOnAction)
#endif
        .onVisible([](JsonObject& root) { root["dbgVisible"] = 1; });

}

#endif // DEBUG_WEB_SUPPOR

#if DEBUG_UDP_SUPPORT

// We use the syslog header as defined in RFC5424 (The Syslog Protocol), ref:
// - https://tools.ietf.org/html/rfc5424
// - https://github.com/xoseperez/espurna/issues/2312/

void debugUdpSyslogConfigure() {
    snprintf_P(
        _udp_syslog_header, sizeof(_udp_syslog_header),
        PSTR("<%u>1 - %s ESPurna - - - "), DEBUG_UDP_FAC_PRI,
        getSetting("hostname", getIdentifier()).c_str());
}

#endif // DEBUG_UDP_SUPPORT

// -----------------------------------------------------------------------------

void debugSetup() {

#if DEBUG_SERIAL_SUPPORT
    DEBUG_PORT.begin(SERIAL_BAUDRATE);
#endif

#if DEBUG_UDP_SUPPORT
    if (_udp_syslog_enabled) {
        debugUdpSyslogConfigure();
        espurnaRegisterReload(debugUdpSyslogConfigure);
    }
#endif

#if TERMINAL_SUPPORT && DEBUG_LOG_BUFFER_SUPPORT
    terminalRegisterCommand(F("DEBUG.BUFFER"), [](const terminal::CommandContext& ctx) {
        _debug_log_buffer_enabled = false;
        if (!_debug_log_buffer.size()) {
            terminalError(ctx, F("buffer is empty\n"));
            return;
        }

        ctx.output.printf_P(PSTR("Buffer size: %u / %u bytes\n"),
            _debug_log_buffer.size(),
            _debug_log_buffer.capacity());
        _debugLogBufferDump(ctx.output);
        terminalOK(ctx);
    });
#endif // TERMINAL_SUPPORT

}

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
        default:
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
        default:
            return DebugLogMode::Enabled;
    }
}

} // namespace internal
} // namespace settings

void debugConfigureBoot() {
    static_assert(
        std::is_same<int, std::underlying_type<DebugLogMode>::type>::value,
        "should be able to match DebugLogMode with int"
    );

    const auto mode = getSetting("dbgLogMode", DEBUG_LOG_MODE);
    switch (mode) {
        case DebugLogMode::SkipBoot:
            schedule_function([]() {
                _debug_enabled = true;
            });
            // fall through
        case DebugLogMode::Disabled:
            _debug_enabled = false;
            break;
        case DebugLogMode::Enabled:
            _debug_enabled = true;
            break;
    }

    debugConfigure();
}

bool _debugHeartbeat(heartbeat::Mask mask) {
    if (mask & heartbeat::Report::Uptime) {
        DEBUG_MSG_P(PSTR("[MAIN] Uptime: %s\n"), getUptime().c_str());
    }

    if (mask & heartbeat::Report::Freeheap) {
        auto stats = systemHeapStats();
        DEBUG_MSG_P(PSTR("[MAIN] %5u / %5u bytes available (%5u contiguous)\n"),
            stats.available, systemInitialFreeHeap(), stats.usable);
    }

    if ((mask & heartbeat::Report::Vcc) && (ADC_MODE_VALUE == ADC_VCC)) {
        DEBUG_MSG_P(PSTR("[MAIN] VCC: %lu mV\n"), ESP.getVcc());
    }

#if NTP_SUPPORT
    if ((mask & heartbeat::Report::Datetime) && (ntpSynced())) {
        DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), ntpDateTime().c_str());
    }
#endif

    return true;
}

void debugConfigure() {

    // HardwareSerial::begin() will automatically enable this when
    // `#if defined(DEBUG_ESP_PORT) && !defined(NDEBUG)`
    // Core debugging also depends on various DEBUG_ESP_... being defined
    {
#if defined(DEBUG_ESP_PORT)
#if not defined(NDEBUG)
        constexpr bool debug_sdk = true;
#endif // !defined(NDEBUG)
#else
        constexpr bool debug_sdk = false;
#endif // defined(DEBUG_ESP_PORT)
        DEBUG_PORT.setDebugOutput(getSetting("dbgSDK", debug_sdk));
    }

    // Make sure other modules are aware of used GPIOs
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
    {
        const auto enabled = getSetting("dbgLogBuf", 1 == DEBUG_LOG_BUFFER_ENABLED);
        const auto size = getSetting("dbgLogBufSize", DEBUG_LOG_BUFFER_SIZE);
        if (enabled) {
            _debug_log_buffer_enabled = true;
            _debug_log_buffer.reserve(size);
        }
    }
    #endif // DEBUG_LOG_BUFFER

    systemHeartbeat(_debugHeartbeat,
        getSetting("dbgHbMode", heartbeat::currentMode()),
        getSetting("dbgHbIntvl", heartbeat::currentInterval()));

}

#endif // DEBUG_SUPPORT
