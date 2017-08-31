/*

DEBUG MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if DEBUG_SERIAL_SUPPORT || DEBUG_UDP_SUPPORT

#include <stdio.h>
#include <stdarg.h>

#if DEBUG_UDP_SUPPORT
#include <WiFiUdp.h>
WiFiUDP udpDebug;
#endif

void debugSend(const char * format, ...) {

    char buffer[DEBUG_MESSAGE_MAX_LENGTH+1];

    va_list args;
    va_start(args, format);
    int len = ets_vsnprintf(buffer, DEBUG_MESSAGE_MAX_LENGTH, format, args);
    va_end(args);

    #if DEBUG_SERIAL_SUPPORT
        DEBUG_PORT.printf(buffer);
        if (len > DEBUG_MESSAGE_MAX_LENGTH) {
            DEBUG_PORT.printf(" (...)\n");
        }
    #endif

    #if DEBUG_UDP_SUPPORT
        if (systemCheck()) {
            udpDebug.beginPacket(DEBUG_UDP_IP, DEBUG_UDP_PORT);
            udpDebug.write(buffer);
            if (len > DEBUG_MESSAGE_MAX_LENGTH) {
                udpDebug.write(" (...)\n");
            }
            udpDebug.endPacket();
            delay(1);
        }
    #endif

}

void debugSend_P(PGM_P format, ...) {

    char f[DEBUG_MESSAGE_MAX_LENGTH+1];
    memcpy_P(f, format, DEBUG_MESSAGE_MAX_LENGTH);

    char buffer[DEBUG_MESSAGE_MAX_LENGTH+1];

    va_list args;
    va_start(args, format);
    int len = ets_vsnprintf(buffer, DEBUG_MESSAGE_MAX_LENGTH, f, args);
    va_end(args);

    #if DEBUG_SERIAL_SUPPORT
        DEBUG_PORT.printf(buffer);
        if (len > DEBUG_MESSAGE_MAX_LENGTH) {
            DEBUG_PORT.printf(" (...)\n");
        }
    #endif

    #if DEBUG_UDP_SUPPORT
        if (systemCheck()) {
            udpDebug.beginPacket(DEBUG_UDP_IP, DEBUG_UDP_PORT);
            udpDebug.write(buffer);
            if (len > DEBUG_MESSAGE_MAX_LENGTH) {
                udpDebug.write(" (...)\n");
            }
            udpDebug.endPacket();
            delay(1);
        }
    #endif

}

#endif // DEBUG_SERIAL_SUPPORT || DEBUG_UDP_SUPPORT
