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

#include "espurna.h"

#if DEBUG_SUPPORT

#include "crash.h"
#include "system.h"
#include "rtcmem.h"
#include "storage_eeprom.h"

#include <cstdio>
#include <cstdarg>

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
 * 11. stack trace size
 * 12. stack trace bytes
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
#define SAVE_CRASH_STACK_SIZE       0x22  // 2 bytes
#define SAVE_CRASH_STACK_TRACE      0x24  // variable, 4 bytes per value

constexpr int EepromCrashBegin = EepromReservedSize;
constexpr int EepromCrashEnd = 256;

constexpr size_t CrashReservedSize = EepromCrashEnd - EepromCrashBegin;
constexpr size_t CrashTraceReservedSize = CrashReservedSize - SAVE_CRASH_STACK_TRACE;

constexpr uint32_t EmptyTimestamp { 0xffffffff };

namespace debug {
namespace {
namespace crash {
namespace internal {

bool enabled = true;

} // namespace internal

namespace build {

constexpr bool enabled() {
    return 1 == SAVE_CRASH_ENABLED;
}

} // namespace build

namespace settings {

bool enabled() {
    return getSetting("sysCrashSave", build::enabled());
}

} // namespace settings

bool enabled() {
    return internal::enabled;
}

void enableFromSettings() {
    internal::enabled = settings::enabled();
}

size_t reserved() {
    if (enabled()) {
        return CrashReservedSize;
    }

    return 0;
}

// Simply reset the timestamp to stop dump() from printing the output more than once per crash.
void clear() {
    eepromPut(EepromCrashBegin + SAVE_CRASH_CRASH_TIME, EmptyTimestamp);
    eepromCommit();
}

// Print out crash information that has been previusly saved in EEPROM
// Optionally, check whether the timestamp is erased / EEPROM contains no data.
void dump(Print& print, bool check) {
    char buffer[256] = {0};

    uint32_t crash_time;
    eepromGet(EepromCrashBegin + SAVE_CRASH_CRASH_TIME, crash_time);

    bool crash_time_erased = ((crash_time == 0) || (crash_time == EmptyTimestamp));
    if (check && crash_time_erased) {
        return;
    }

    uint8_t reason = eepromRead(EepromCrashBegin + SAVE_CRASH_RESTART_REASON);
    if (!crash_time_erased) {
        snprintf_P(buffer, sizeof(buffer), PSTR("\nlatest crash was at %lu ms after boot\n"), crash_time);
        print.print(buffer);
    }

    snprintf_P(buffer, sizeof(buffer), PSTR("Reason of restart: %u\n"), reason);
    print.print(buffer);

    if (reason == REASON_EXCEPTION_RST) {
        snprintf_P(buffer, sizeof(buffer), PSTR("\nException (%u):\n"),
            eepromRead(EepromCrashBegin + SAVE_CRASH_EXCEPTION_CAUSE));
        print.print(buffer);

        uint32_t epc1, epc2, epc3, excvaddr, depc;
        eepromGet(EepromCrashBegin + SAVE_CRASH_EPC1, epc1);
        eepromGet(EepromCrashBegin + SAVE_CRASH_EPC2, epc2);
        eepromGet(EepromCrashBegin + SAVE_CRASH_EPC3, epc3);
        eepromGet(EepromCrashBegin + SAVE_CRASH_EXCVADDR, excvaddr);
        eepromGet(EepromCrashBegin + SAVE_CRASH_DEPC, depc);

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

    eepromGet(EepromCrashBegin + SAVE_CRASH_STACK_START, stack_start);
    eepromGet(EepromCrashBegin + SAVE_CRASH_STACK_END, stack_end);
    eepromGet(EepromCrashBegin + SAVE_CRASH_STACK_SIZE, stack_size);

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
        eepromGet(eeprom_addr, addr1);
        eepromGet((eeprom_addr += step), addr2);
        eepromGet((eeprom_addr += step), addr3);
        eepromGet((eeprom_addr += step), addr4);

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

void forceDump(Print& print) {
    dump(print, false);
}

void dump(Print& print) {
    dump(print, true);
}

} // namespace crash
} // namespace
} // namespace debug

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
    if (pendingDeferredReset()) {
        return;
    }

    if (!debug::crash::enabled()) {
        return;
    }

    // We will use this later as a marker that there was a crash
    uint32_t crash_time = millis();
    eepromPut(EepromCrashBegin + SAVE_CRASH_CRASH_TIME, crash_time);

    // XXX rst_info::reason and ::exccause are uint32_t, but are holding small values
    //     make sure we are using ::write() instead of ::put(), former tries to deduce the required size based on variable type
    eepromWrite(EepromCrashBegin + SAVE_CRASH_RESTART_REASON,
        static_cast<uint8_t>(rst_info->reason));
    eepromWrite(EepromCrashBegin + SAVE_CRASH_EXCEPTION_CAUSE,
        static_cast<uint8_t>(rst_info->exccause));

    // write epc1, epc2, epc3, excvaddr and depc to EEPROM as uint32_t
    eepromPut(EepromCrashBegin + SAVE_CRASH_EPC1, rst_info->epc1);
    eepromPut(EepromCrashBegin + SAVE_CRASH_EPC2, rst_info->epc2);
    eepromPut(EepromCrashBegin + SAVE_CRASH_EPC3, rst_info->epc3);
    eepromPut(EepromCrashBegin + SAVE_CRASH_EXCVADDR, rst_info->excvaddr);
    eepromPut(EepromCrashBegin + SAVE_CRASH_DEPC, rst_info->depc);

    // EEPROM size is limited, write as little as possible.
    // we definitely want to avoid big stack traces, e.g. like when stack_end == 0x3fffffb0 and we are in SYS context.
    // but still should get enough relevant info and it is possible to set needed size at build/runtime
    const uint16_t stack_size = constrain((stack_end - stack_start), 0, CrashReservedSize);
    eepromPut(EepromCrashBegin + SAVE_CRASH_STACK_START, stack_start);
    eepromPut(EepromCrashBegin + SAVE_CRASH_STACK_END, stack_end);
    eepromPut(EepromCrashBegin + SAVE_CRASH_STACK_SIZE, stack_size);

    // write stack trace to EEPROM and avoid overwriting settings and reserved data
    // [EEPROM RESERVED SPACE] >>> ... CRASH DATA ... >>> [SETTINGS]
    int eeprom_addr = EepromCrashBegin + SAVE_CRASH_STACK_TRACE;

    auto *addr = reinterpret_cast<uint32_t*>(stack_start);
    while (EepromCrashEnd > eeprom_addr) {
        eepromPut(eeprom_addr, *addr);
        eeprom_addr += sizeof(uint32_t);
        ++addr;
    }

    eepromForceCommit();
}

void crashForceDump(Print& print) {
    debug::crash::forceDump(print);
}

void crashDump(Print& print) {
    debug::crash::dump(print);
}

void crashResetReason(Print& print) {
    const auto reason = customResetReason();
    bool custom { CustomResetReason::None != reason };
    print.printf_P(PSTR("last reset reason: %s\n"), custom
        ? customResetReasonToPayload(reason).c_str()
        : ESP.getResetReason().c_str());

    if (!custom) {
        print.printf_P(PSTR("extra info: %s\n"), ESP.getResetInfo().c_str());
    }

    crashDump(print);
}

size_t crashReservedSize() {
    return debug::crash::reserved();
}

void crashClear() {
    debug::crash::clear();
}

void crashSetup() {
    if (!rtcmemStatus()) {
        debug::crash::clear();
    }

#if TERMINAL_SUPPORT
    terminalRegisterCommand(F("CRASH"), [](::terminal::CommandContext&& ctx) {
        debug::crash::forceDump(ctx.output);
        terminalOK(ctx);
    });
#endif

    debug::crash::enableFromSettings();
}

#endif // DEBUG_SUPPORT
