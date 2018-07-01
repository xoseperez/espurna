/*

DEBUG MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

Module key prefix: dbg

*/

#if DEBUG_SUPPORT

#include <stdio.h>
#include <stdarg.h>
#include <EEPROM_Rotate.h>

extern "C" {
    #include "user_interface.h"
}

#if DEBUG_UDP_SUPPORT
#include <WiFiUdp.h>
WiFiUDP _udp_debug;
#if DEBUG_UDP_PORT == 514
char _udp_syslog_header[40] = {0};
#endif
#endif

#if DEBUG_SERIAL_SUPPORT
HardwareSerial & _dbg_port = DEBUG_PORT;
bool _dbg_serial_enabled = true;
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
        if (_dbg_serial_enabled) {
            #if DEBUG_ADD_TIMESTAMP
                _dbg_port.printf(timestamp);
            #endif
            _dbg_port.printf(message);
        }
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
        snprintf_P(_udp_syslog_header, sizeof(_udp_syslog_header), PSTR("<%u>%s ESPurna[0]: "), DEBUG_UDP_FAC_PRI, getHostname().c_str());
    #endif
    #endif


}

#endif // DEBUG_WEB_SUPPORT

bool _debugKeyCheck(const char * key) {
    return (strncmp(key, "dbg", 3) == 0);
}

int debugSerialAvailable() {
    #if DEBUG_SERIAL_SUPPORT
        if (_dbg_serial_enabled) {
            return _dbg_port.available();
        }
    #endif
    return 0;
}

int debugSerialRead() {
    #if DEBUG_SERIAL_SUPPORT
        if (_dbg_serial_enabled) {
            return _dbg_port.read();
        }
    #endif
    return 0;
}

void debugSerialWrite(uint8_t ch) {
    #if DEBUG_SERIAL_SUPPORT
        if (_dbg_serial_enabled) {
            _dbg_port.write(ch);
        }
    #endif
}

void debugSetup() {

    #if DEBUG_SERIAL_SUPPORT

        _dbg_serial_enabled = getSetting("dbgSerial", 1).toInt() == 1;

        if (_dbg_serial_enabled) {

            unsigned char port = getSetting("dbgPort", 0).toInt();
            if (0 == port) {
                _dbg_port = Serial;
            } else {
                _dbg_port = Serial1;
            }

            unsigned long speed = getSetting("dbgSpeed", DEBUG_SERIAL_SPEED).toInt();
            _dbg_port.begin(speed);

            #if DEBUG_ESP_WIFI
                _dbg_port.setDebugOutput(true);
            #endif

        }

    #endif

    settingsRegisterKeyCheck(_debugKeyCheck);

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
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_CRASH_TIME, crash_time);

    // write reset info to EEPROM
    EEPROMr.write(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_RESTART_REASON, rst_info->reason);
    EEPROMr.write(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCEPTION_CAUSE, rst_info->exccause);

    // write epc1, epc2, epc3, excvaddr and depc to EEPROM
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC1, rst_info->epc1);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC2, rst_info->epc2);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC3, rst_info->epc3);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCVADDR, rst_info->excvaddr);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_DEPC, rst_info->depc);

    // write stack start and end address to EEPROM
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_START, stack_start);
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_END, stack_end);

    // write stack trace to EEPROM
    int16_t current_address = SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_TRACE;
    for (uint32_t i = stack_start; i < stack_end; i++) {
        byte* byteValue = (byte*) i;
        EEPROMr.write(current_address++, *byteValue);
    }

    EEPROMr.commit();

}

/**
 * Clears crash info
 */
void debugClearCrashInfo() {
    uint32_t crash_time = 0xFFFFFFFF;
    EEPROMr.put(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_CRASH_TIME, crash_time);
    EEPROMr.commit();
}

/**
 * Print out crash information that has been previusly saved in EEPROM
 */
void debugDumpCrashInfo() {

    uint32_t crash_time;
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_CRASH_TIME, crash_time);
    if ((crash_time == 0) || (crash_time == 0xFFFFFFFF)) {
        DEBUG_MSG_P(PSTR("[DEBUG] No crash info\n"));
        return;
    }

    DEBUG_MSG_P(PSTR("[DEBUG] Latest crash was at %lu ms after boot\n"), crash_time);
    DEBUG_MSG_P(PSTR("[DEBUG] Reason of restart: %u\n"), EEPROMr.read(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_RESTART_REASON));
    DEBUG_MSG_P(PSTR("[DEBUG] Exception cause: %u\n"), EEPROMr.read(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCEPTION_CAUSE));

    uint32_t epc1, epc2, epc3, excvaddr, depc;
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC1, epc1);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC2, epc2);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EPC3, epc3);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_EXCVADDR, excvaddr);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_DEPC, depc);
    DEBUG_MSG_P(PSTR("[DEBUG] epc1=0x%08x epc2=0x%08x epc3=0x%08x\n"), epc1, epc2, epc3);
    DEBUG_MSG_P(PSTR("[DEBUG] excvaddr=0x%08x depc=0x%08x\n"), excvaddr, depc);

    uint32_t stack_start, stack_end;
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_START, stack_start);
    EEPROMr.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_END, stack_end);
    DEBUG_MSG_P(PSTR("[DEBUG] >>>stack>>>\n[DEBUG] "));
    int16_t current_address = SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_STACK_TRACE;
    int16_t stack_len = stack_end - stack_start;
    uint32_t stack_trace;
    for (int16_t i = 0; i < stack_len; i += 0x10) {
        DEBUG_MSG_P(PSTR("%08x: "), stack_start + i);
        for (byte j = 0; j < 4; j++) {
            EEPROMr.get(current_address, stack_trace);
            DEBUG_MSG_P(PSTR("%08x "), stack_trace);
            current_address += 4;
        }
        DEBUG_MSG_P(PSTR("\n[DEBUG] "));
    }
    DEBUG_MSG_P(PSTR("<<<stack<<<\n"));

}

#endif // DEBUG_SUPPORT
