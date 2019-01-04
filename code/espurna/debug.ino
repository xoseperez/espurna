/*

DEBUG MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if DEBUG_SUPPORT

#if DEBUG_UDP_SUPPORT
#include <WiFiUdp.h>
WiFiUDP _udp_debug;
#if DEBUG_UDP_PORT == 514
char _udp_syslog_header[40] = {0};
#endif
#endif

void _debugSend(char * message) {

    bool pause = false;

    #if DEBUG_ADD_TIMESTAMP
        static bool add_timestamp = true;
        char timestamp[10] = {0};
        if (add_timestamp) snprintf_P(timestamp, sizeof(timestamp), PSTR("[%06lu] "), millis() % 1000000);
        add_timestamp = (message[strlen(message)-1] == 10) || (message[strlen(message)-1] == 13);
    #endif

    #if DEBUG_SERIAL_SUPPORT
        #if DEBUG_ADD_TIMESTAMP
            DEBUG_PORT.printf(timestamp);
        #endif
        DEBUG_PORT.printf(message);
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
        #if DEBUG_ADD_TIMESTAMP
            _telnetWrite(timestamp, strlen(timestamp));
        #endif
        _telnetWrite(message, strlen(message));
        pause = true;
    #endif

    #if DEBUG_WEB_SUPPORT
        if (wsConnected() && (getFreeHeap() > 10000)) {
            DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(1) + strlen(message) + 17);
            JsonObject &root = jsonBuffer.createObject();
            #if DEBUG_ADD_TIMESTAMP
                char buffer[strlen(timestamp) + strlen(message) + 1];
                snprintf_P(buffer, sizeof(buffer), "%s%s", timestamp, message);
                root.set("weblog", buffer);
            #else
                root.set("weblog", message);
            #endif
            String out;
            root.printTo(out);
            jsonBuffer.clear();

            wsSend(out.c_str());
            pause = true;
        }
    #endif

    if (pause) optimistic_yield(100);

}

// -----------------------------------------------------------------------------

void debugSend(const char * format, ...) {

    va_list args;
    va_start(args, format);
    char test[1];
    int len = ets_vsnprintf(test, 1, format, args) + 1;
    char * buffer = new char[len];
    ets_vsnprintf(buffer, len, format, args);
    va_end(args);

    _debugSend(buffer);

    delete[] buffer;

}

void debugSend_P(PGM_P format_P, ...) {

    char format[strlen_P(format_P)+1];
    memcpy_P(format, format_P, sizeof(format));

    va_list args;
    va_start(args, format_P);
    char test[1];
    int len = ets_vsnprintf(test, 1, format, args) + 1;
    char * buffer = new char[len];
    ets_vsnprintf(buffer, len, format, args);
    va_end(args);

    _debugSend(buffer);

    delete[] buffer;

}

#if DEBUG_WEB_SUPPORT

void debugWebSetup() {

    wsOnSendRegister([](JsonObject& root) {
        root["dbgVisible"] = 1;
    });

    wsOnActionRegister([](uint32_t client_id, const char * action, JsonObject& data) {
        if (strcmp(action, "dbgcmd") == 0) {
            const char* command = data.get<const char*>("command");
            char buffer[strlen(command) + 2];
            snprintf(buffer, sizeof(buffer), "%s\n", command);
            settingsInject((void*) buffer, strlen(buffer));
        }
    });

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

}

#endif // DEBUG_SUPPORT
