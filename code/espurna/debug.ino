/*

DEBUG MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if DEBUG_SUPPORT

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

    #if DEBUG_TELNET_SUPPORT
        _telnetWrite(buffer, strlen(buffer));
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

    #if DEBUG_TELNET_SUPPORT
        _telnetWrite(buffer, strlen(buffer));
    #endif

}

// -----------------------------------------------------------------------------
// Save crash info
// Taken from krzychb EspSaveCrash
// https://github.com/krzychb/EspSaveCrash
// -----------------------------------------------------------------------------

#define SAVE_CRASH_EEPROM_OFFSET    0x0100  // initial address for crash data

/**
 * Structure of the single crash data set
 *
 *  1. Crash time
 *  2. Restart reason
 *  3. Exception cause
 *  4. epc1
 *  5. epc2
 *  6. epc3
 *  7. excvaddr
 *  8. depc
 *  9. adress of stack start
 * 10. adress of stack end
 * 11. stack trace bytes
 *     ...
 */
#define SAVE_CRASH_CRASH_TIME       0x00  // 4 bytes
#define SAVE_CRASH_RESTART_REASON   0x04  // 1 byte
#define SAVE_CRASH_EXCEPTION_CAUSE  0x05  // 1 byte
#define SAVE_CRASH_EPC1             0x06  // 4 bytes
#define SAVE_CRASH_EPC2             0x0A  // 4 bytes
#define SAVE_CRASH_EPC3             0x0E  // 4 bytes
#define SAVE_CRASH_EXCVADDR         0x12  // 4 bytes
#define SAVE_CRASH_DEPC             0x16  // 4 bytes
#define SAVE_CRASH_STACK_START      0x1A  // 4 bytes
#define SAVE_CRASH_STACK_END        0x1E  // 4 bytes
#define SAVE_CRASH_STACK_TRACE      0x22  // variable

/**
 * Save crash information in EEPROM
 * This function is called automatically if ESP8266 suffers an exception
 * It should be kept quick / consise to be able to execute before hardware wdt may kick in
 */
extern "C" void custom_crash_callback(struct rst_info * rst_info, uint32_t stack_start, uint32_t stack_end ) {

    // This method assumes EEPROM has already been initialized
    // which is the first thing ESPurna does

    // write crash time to EEPROM
    uint32_t crash_time = millis();
    EEPROM.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_CRASH_TIME, crash_time);

    // write reset info to EEPROM
    EEPROM.write(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_RESTART_REASON, rst_info->reason);
    EEPROM.write(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCEPTION_CAUSE, rst_info->exccause);

    // write epc1, epc2, epc3, excvaddr and depc to EEPROM
    EEPROM.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC1, rst_info->epc1);
    EEPROM.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC2, rst_info->epc2);
    EEPROM.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC3, rst_info->epc3);
    EEPROM.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCVADDR, rst_info->excvaddr);
    EEPROM.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_DEPC, rst_info->depc);

    // write stack start and end address to EEPROM
    EEPROM.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_START, stack_start);
    EEPROM.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_END, stack_end);

    // write stack trace to EEPROM
    int16_t current_address = SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_TRACE;
    for (uint32_t i = stack_start; i < stack_end; i++) {
        byte* byteValue = (byte*) i;
        EEPROM.write(current_address++, *byteValue);
    }

    EEPROM.commit();

}

/**
 * Clears crash info
 */
void debugClearCrashInfo() {
    uint32_t crash_time = 0xFFFFFFFF;
    EEPROM.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_CRASH_TIME, crash_time);
    EEPROM.commit();
}

/**
 * Print out crash information that has been previusly saved in EEPROM
 */
void debugDumpCrashInfo() {

    uint32_t crash_time;
    EEPROM.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_CRASH_TIME, crash_time);
    if ((crash_time == 0) || (crash_time == 0xFFFFFFFF)) {
        DEBUG_MSG_P(PSTR("[DEBUG] No crash info\n"));
        return;
    }

    DEBUG_MSG_P(PSTR("[DEBUG] Crash at %ld ms\n"), crash_time);
    DEBUG_MSG_P(PSTR("[DEBUG] Reason of restart: %d\n"), EEPROM.read(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_RESTART_REASON));
    DEBUG_MSG_P(PSTR("[DEBUG] Exception cause: %d\n"), EEPROM.read(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCEPTION_CAUSE));

    uint32_t epc1, epc2, epc3, excvaddr, depc;
    EEPROM.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC1, epc1);
    EEPROM.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC2, epc2);
    EEPROM.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC3, epc3);
    EEPROM.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCVADDR, excvaddr);
    EEPROM.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_DEPC, depc);
    DEBUG_MSG_P(PSTR("[DEBUG] epc1=0x%08x epc2=0x%08x epc3=0x%08x\n"), epc1, epc2, epc3);
    DEBUG_MSG_P(PSTR("[DEBUG] excvaddr=0x%08x depc=0x%08x\n"), excvaddr, depc);

    uint32_t stack_start, stack_end;
    EEPROM.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_START, stack_start);
    EEPROM.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_END, stack_end);
    DEBUG_MSG_P(PSTR("[DEBUG] >>>stack>>>\n[DEBUG] "));
    int16_t current_address = SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_TRACE;
    int16_t stack_len = stack_end - stack_start;
    uint32_t stack_trace;
    for (int16_t i = 0; i < stack_len; i += 0x10) {
        DEBUG_MSG_P(PSTR("%08x: "), stack_start + i);
        for (byte j = 0; j < 4; j++) {
            EEPROM.get(current_address, stack_trace);
            DEBUG_MSG_P(PSTR("%08x "), stack_trace);
            current_address += 4;
        }
        DEBUG_MSG_P(PSTR("\n[DEBUG] "));
    }
    DEBUG_MSG_P(PSTR("<<<stack<<<\n"));

}
#endif // DEBUG_SUPPORT
