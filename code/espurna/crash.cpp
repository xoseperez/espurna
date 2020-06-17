/*

Part of the DEBUG MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

// -----------------------------------------------------------------------------
// Save crash info
// Original code by @krzychb
// https://github.com/krzychb/EspSaveCrash
// -----------------------------------------------------------------------------

#include "crash.h"

#if DEBUG_SUPPORT

#include <stdio.h>
#include <stdarg.h>

#include "system.h"
#include "storage_eeprom.h"

bool _save_crash_enabled = true;

size_t crashReservedSize() {
    if (!_save_crash_enabled) return 0;
    return CrashReservedSize;
}

/**
 * Save crash information in EEPROM
 * This function is called automatically if ESP8266 suffers an exception
 * It should be kept quick / consise to be able to execute before hardware wdt may kick in
 * This method assumes EEPROM has already been initialized, which is the first thing ESPurna does
 */
extern "C" void custom_crash_callback(struct rst_info * rst_info, uint32_t stack_start, uint32_t stack_end ) {

    // Small safeguard to protect from calling crash handler very early on boot.
    if (!eepromReady()) {
        return;
    }

    // If we crash more than once in a row, don't store (similar) crash log every time
    if (systemStabilityCounter() > 1) {
        return;
    }

    // Do not record crash data when doing a normal reboot or when crash trace was disabled
    if (checkNeedsReset()) {
        return;
    }

    if (!_save_crash_enabled) {
        return;
    }

    // We will use this later as a marker that there was a crash
    uint32_t crash_time = millis();
    EEPROMr.put(EepromCrashBegin + SAVE_CRASH_CRASH_TIME, crash_time);

    // XXX rst_info::reason and ::exccause are uint32_t, but are holding small values
    //     make sure we are using ::write() instead of ::put(), former tries to deduce the required size based on variable type
    EEPROMr.write(EepromCrashBegin + SAVE_CRASH_RESTART_REASON,
        static_cast<uint8_t>(rst_info->reason));
    EEPROMr.write(EepromCrashBegin + SAVE_CRASH_EXCEPTION_CAUSE,
        static_cast<uint8_t>(rst_info->exccause));

    // write epc1, epc2, epc3, excvaddr and depc to EEPROM as uint32_t
    EEPROMr.put(EepromCrashBegin + SAVE_CRASH_EPC1, rst_info->epc1);
    EEPROMr.put(EepromCrashBegin + SAVE_CRASH_EPC2, rst_info->epc2);
    EEPROMr.put(EepromCrashBegin + SAVE_CRASH_EPC3, rst_info->epc3);
    EEPROMr.put(EepromCrashBegin + SAVE_CRASH_EXCVADDR, rst_info->excvaddr);
    EEPROMr.put(EepromCrashBegin + SAVE_CRASH_DEPC, rst_info->depc);

    // EEPROM size is limited, write as little as possible.
    // we definitely want to avoid big stack traces, e.g. like when stack_end == 0x3fffffb0 and we are in SYS context.
    // but still should get enough relevant info and it is possible to set needed size at build/runtime
    const uint16_t stack_size = constrain((stack_end - stack_start), 0, CrashReservedSize);
    EEPROMr.put(EepromCrashBegin + SAVE_CRASH_STACK_START, stack_start);
    EEPROMr.put(EepromCrashBegin + SAVE_CRASH_STACK_END, stack_end);
    EEPROMr.put(EepromCrashBegin + SAVE_CRASH_STACK_SIZE, stack_size);

    // write stack trace to EEPROM and avoid overwriting settings and reserved data
    // [EEPROM RESERVED SPACE] >>> ... CRASH DATA ... >>> [SETTINGS]
    int eeprom_addr = EepromCrashBegin + SAVE_CRASH_STACK_TRACE;

    auto *addr = reinterpret_cast<uint32_t*>(stack_start);
    while (EepromCrashEnd > eeprom_addr) {
        EEPROMr.put(eeprom_addr, *addr);
        eeprom_addr += sizeof(uint32_t);
        ++addr;
    }

    EEPROMr.commit();

}

/**
 * Clears crash info CRASH_TIME value, later checked in crashDump()
 */
void crashClear() {
    uint32_t crash_time = 0xFFFFFFFF;
    EEPROMr.put(EepromCrashBegin + SAVE_CRASH_CRASH_TIME, crash_time);
    EEPROMr.commit();
}

namespace {

/**
 * Print out crash information that has been previusly saved in EEPROM
 * We can optionally check for the recorded crash time before proceeding.
 */
void _crashDump(Print& print, bool check) {

    char buffer[256] = {0};

    uint32_t crash_time;
    EEPROMr.get(EepromCrashBegin + SAVE_CRASH_CRASH_TIME, crash_time);

    bool crash_time_erased = ((crash_time == 0) || (crash_time == 0xFFFFFFFF));
    if (check && crash_time_erased) {
        return;
    }

    uint8_t reason = EEPROMr.read(EepromCrashBegin + SAVE_CRASH_RESTART_REASON);
    if (!crash_time_erased) {
        snprintf_P(buffer, sizeof(buffer), PSTR("\nLatest crash was at %lu ms after boot\n"), crash_time);
        print.print(buffer);
    }

    snprintf_P(buffer, sizeof(buffer), PSTR("Reason of restart: %u\n"), reason);
    print.print(buffer);

    if (reason == REASON_EXCEPTION_RST) {
        snprintf_P(buffer, sizeof(buffer), PSTR("\nException (%u):\n"),
            EEPROMr.read(EepromCrashBegin + SAVE_CRASH_EXCEPTION_CAUSE));
        print.print(buffer);

        uint32_t epc1, epc2, epc3, excvaddr, depc;
        EEPROMr.get(EepromCrashBegin + SAVE_CRASH_EPC1, epc1);
        EEPROMr.get(EepromCrashBegin + SAVE_CRASH_EPC2, epc2);
        EEPROMr.get(EepromCrashBegin + SAVE_CRASH_EPC3, epc3);
        EEPROMr.get(EepromCrashBegin + SAVE_CRASH_EXCVADDR, excvaddr);
        EEPROMr.get(EepromCrashBegin + SAVE_CRASH_DEPC, depc);

        snprintf_P(buffer, sizeof(buffer), PSTR("epc1=0x%08x epc2=0x%08x epc3=0x%08x excvaddr=0x%08x depc=0x%08x\n"),
            epc1, epc2, epc3, excvaddr, depc);
        print.print(buffer);
    }

    // We need something like
    //
    //>>>stack>>>
    //
    //ctx: sys
    //sp: 3fffecf0 end: 3fffffb0 offset: 0000
    //...addresses...
    //...addresses...
    //...addresses...
    //...addresses...
    //<<<stack<<<
    //
    // Also note that we should always recommend using latest toolchain to decode the .elf,
    // older versions (the one used by the 2.3.0 specifically) binutils are broken.

    uint32_t stack_start, stack_end;
    uint16_t stack_size;

    EEPROMr.get(EepromCrashBegin + SAVE_CRASH_STACK_START, stack_start);
    EEPROMr.get(EepromCrashBegin + SAVE_CRASH_STACK_END, stack_end);
    EEPROMr.get(EepromCrashBegin + SAVE_CRASH_STACK_SIZE, stack_size);

    if ((0 == stack_size) || (0xffff == stack_size)) return;
    stack_size = constrain(stack_size, 0, CrashTraceReservedSize);

    // offset is technically an unknown, Core's crash handler only gives us `stack_start` as `sp_dump + offset`
    // (...maybe we can hack Core / walk the stack / etc... but, that's not really portable between versions)
    snprintf_P(buffer, sizeof(buffer),
        PSTR("\n>>>stack>>>\n\nctx: todo\nsp: %08x end: %08x offset: 0000\n"),
        stack_start, stack_end
    );
    print.print(buffer);

    constexpr auto step = sizeof(uint32_t);

    int eeprom_addr = EepromCrashBegin + SAVE_CRASH_STACK_TRACE;
    uint16_t offset = 0;
    uint32_t addr1, addr2, addr3, addr4;

    while ((eeprom_addr + (4 * step)) < EepromCrashEnd) {
        EEPROMr.get(eeprom_addr, addr1);
        EEPROMr.get((eeprom_addr += step), addr2);
        EEPROMr.get((eeprom_addr += step), addr3);
        EEPROMr.get((eeprom_addr += step), addr4);

        snprintf_P(buffer, sizeof(buffer),
            PSTR("%08x:  %08x %08x %08x %08x \n"),
            stack_start + offset,
            addr1, addr2, addr3, addr4
        );
        print.print(buffer);

        eeprom_addr += step;
        offset += step;
    }

    snprintf_P(buffer, sizeof(buffer), PSTR("<<<stack<<<\n"));
    print.print(buffer);

}

#if TERMINAL_SUPPORT

void _crashTerminalCommand(const terminal::CommandContext& ctx) {
    if ((ctx.argc == 2) && (ctx.argv[1].equals(F("force")))) {
        crashForceDump(ctx.output);
    } else {
        crashDump(ctx.output);
        crashClear();
    }
    terminalOK(ctx);
}

#endif

} // namespace

void crashForceDump(Print& print) {
    _crashDump(print, false);
}

void crashDump(Print& print) {
    _crashDump(print, true);
}

void crashSetup() {

    #if TERMINAL_SUPPORT
        terminalRegisterCommand(F("CRASH"), _crashTerminalCommand);
    #endif

    _save_crash_enabled = getSetting("sysCrashSave", 1 == SAVE_CRASH_ENABLED);

}

#endif // DEBUG_SUPPORT
