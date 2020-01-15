/*

DEBUG MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if DEBUG_SUPPORT

#include <limits>
#include <vector>

#include "debug.h"

#if DEBUG_UDP_SUPPORT
#include <WiFiUdp.h>
WiFiUDP _udp_debug;
#if DEBUG_UDP_PORT == 514
char _udp_syslog_header[40] = {0};
#endif
#endif

// -----------------------------------------------------------------------------
// printf-like debug methods
// -----------------------------------------------------------------------------

constexpr const int DEBUG_SEND_STRING_BUFFER_SIZE = 128;

void _debugSendInternal(const char * message, bool add_timestamp = DEBUG_ADD_TIMESTAMP);

// TODO: switch to newlib vsnprintf for latest Cores to support PROGMEM args
void _debugSend(const char * format, va_list args) {

    char temp[DEBUG_SEND_STRING_BUFFER_SIZE];
    int len = ets_vsnprintf(temp, sizeof(temp), format, args);

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
    ets_vsnprintf(buffer, len, format, args);

    _debugSendInternal(buffer);
    free(buffer);

}

void debugSend(const char* format, ...) {

    va_list args;
    va_start(args, format);

    _debugSend(format, args);

    va_end(args);

}

void debugSend_P(PGM_P format_P, ...) {

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

#if DEBUG_TELNET_SUPPORT
    void _debugSendTelnet(const char* prefix, const char* data) {
        if (prefix && (prefix[0] != '\0')) {
            _telnetWrite(prefix);
        }
        _telnetWrite(data);

    }
#endif // DEBUG_TELNET_SUPPORT

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
            #if DEBUG_UDP_PORT == 514
                _udp_debug.write(_udp_syslog_header);
            #endif
            _udp_debug.write(message);
            _udp_debug.endPacket();
            pause = true;
        #if SYSTEM_CHECK_ENABLED
        }
        #endif
    #endif

    #if DEBUG_TELNET_SUPPORT
        _debugSendTelnet(timestamp, message);
        pause = true;
    #endif

    #if DEBUG_WEB_SUPPORT
        wsDebugSend(timestamp, message);
        pause = true;
    #endif

    #if DEBUG_LOG_BUFFER_SUPPORT
        _debugLogBuffer(timestamp, message);
    #endif

    if (pause) optimistic_yield(100);

}

// -----------------------------------------------------------------------------

#if DEBUG_WEB_SUPPORT

void _debugWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {

        #if TERMINAL_SUPPORT
            if (strcmp(action, "dbgcmd") == 0) {
                if (!data.containsKey("command") || !data["command"].is<const char*>()) return;
                const char* command = data["command"];
                if (command && strlen(command)) {
                    auto command = data.get<const char*>("command");
                    terminalInject((void*) command, strlen(command));
                    terminalInject('\n');
                }
            }
        #endif
}

void debugWebSetup() {

    wsRegister()
        .onVisible([](JsonObject& root) { root["dbgVisible"] = 1; })
        .onAction(_debugWebSocketOnAction);

    // TODO: if hostname string changes, need to update header too
    #if DEBUG_UDP_SUPPORT
    #if DEBUG_UDP_PORT == 514
        snprintf_P(_udp_syslog_header, sizeof(_udp_syslog_header), PSTR("<%u>%s ESPurna[0]: "), DEBUG_UDP_FAC_PRI, getSetting("hostname").c_str());
    #endif
    #endif


}

#endif // DEBUG_WEB_SUPPORT

// -----------------------------------------------------------------------------

void debugSetup() {

    #if DEBUG_SERIAL_SUPPORT
        DEBUG_PORT.begin(SERIAL_BAUDRATE);
        #if DEBUG_ESP_WIFI
            DEBUG_PORT.setDebugOutput(true);
        #endif
    #endif

    #if DEBUG_LOG_BUFFER_SUPPORT
    {
        auto enabled = getSetting("dbgBufEnabled", DEBUG_LOG_BUFFER_ENABLED).toInt() == 1;
        auto size = getSetting("dbgBufSize", DEBUG_LOG_BUFFER_SIZE).toInt();
        if (enabled) {
            _debug_log_buffer_enabled = true;
            _debug_log_buffer.reserve(size);
        }
    }

    #if TERMINAL_SUPPORT
        terminalRegisterCommand(F("DEBUG.BUFFER"), [](Embedis* e) {
            _debug_log_buffer_enabled = false;
            if (!_debug_log_buffer.size()) {
                DEBUG_MSG_P(PSTR("[DEBUG] Buffer is empty\n"));
                return;
            }
            DEBUG_MSG_P(PSTR("[DEBUG] Buffer size: %u / %u bytes\n"),
                _debug_log_buffer.size(),
                _debug_log_buffer.capacity()
            );

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
                _debugSendInternal(_debug_log_buffer.data() + index, false);
                _debug_log_buffer[index + len] = value;

                index += len;
            } while (true);

            _debug_log_buffer.clear();
            _debug_log_buffer.shrink_to_fit();
        });
    #endif // TERMINAL_SUPPORT

    #endif // DEBUG_LOG_BUFFER

}

#endif // DEBUG_SUPPORT
