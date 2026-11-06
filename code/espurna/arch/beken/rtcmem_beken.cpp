/*

RTMEM MODULE FOR BEKEN (LibreTiny)

*/

#include "espurna.h"
#include "arch/beken/rtcmem_beken.h"
#include "build.h"

#if MQTT_SUPPORT
#include "mqtt.h"
#endif

#include <cstddef>
#include <cstring>

#if defined(LIBRETINY)

// No RAM survives the reset, bootloader clears all of it (0x00400000 and up, checked on BK7231N).
// Contents only live until the next reboot, relay state is restored from settings instead.
static RtcmemData _rtcmem_storage;
volatile RtcmemData* Rtcmem = &_rtcmem_storage;

namespace {

// Except for the system word (custom reset reason and the stability counter), kept in the SDK retention
// register instead of the start type. Register only has 16 bits (checked on BK7231N), hardware keeps
// them across watchdog resets and clears them on power-on. SDK start types are 0x0..0xc and 0xad0x,
// ours are 0x5rc0 (reason, counter). SDK reads them as a watchdog reset, system_beken.cpp knows better.
// SDK rewrites the register on every watchdog feed, its own reboots and crashes, so ours is only
// written by restart() right before the reset, and by the crash handlers below (0x5rck, k = crash kind).
// Otherwise the stability counter would be lost on every crash and a crash loop never detected.
// A hang still loses it, the start type tells nothing more than 'watchdog' then
constexpr uintptr_t RegRetention { 0x00800150 };

constexpr uint32_t RetentionMarkerMask { 0xf000 };
constexpr uint32_t RetentionMarker { 0x5000 };

constexpr int RetentionCounterShift { 4 };
constexpr int RetentionReasonShift { 8 };
constexpr uint32_t RetentionFieldMask { 0xf };
constexpr uint32_t RetentionCrashMask { 0xf };

volatile uint32_t& retention() {
    return *reinterpret_cast<volatile uint32_t*>(RegRetention);
}

// Same layout as system.cpp Data, low byte is the counter and the next one is the reason
uint32_t pack(uint32_t sys) {
    const auto counter = std::min(sys & 0xff, RetentionFieldMask);
    const auto reason = std::min((sys >> 8) & 0xff, RetentionFieldMask);

    return RetentionMarker
        | (counter << RetentionCounterShift)
        | (reason << RetentionReasonShift);
}

uint32_t unpack(uint32_t value) {
    const auto counter = (value >> RetentionCounterShift) & RetentionFieldMask;
    const auto reason = (value >> RetentionReasonShift) & RetentionFieldMask;

    return counter | (reason << 8);
}

// Watchdog driver writes RESET_SOURCE_WATCHDOG over the whole register on every feed, the first one
// happens before setup(). Read it as early as possible, while running the static initializers
const uint32_t _rtcmem_retention_at_boot = retention();

// RAM does not survive the reset either, so the crash handlers below also write the registers and the top of the stack
// into the flash sector right above the settings slots (storage_eeprom_beken.cpp) and below the Tuya config, blank
// on every stock dump seen. The record stays until the next crash, 'pending' goes from all ones to zero (no erase
// needed) once the boot has reported it
constexpr uint32_t CrashRecordOffset { 0x1EB000 };
constexpr uint32_t CrashRecordMagic { 0x32535243 }; // CRS2, CRSH kept the exception mode sp lr
constexpr uint32_t CrashRecordPending { 0xffffffff };

// BK7231N RAM, a stack pointer outside of it is not read
constexpr uint32_t RamBegin { 0x00400000 };
constexpr uint32_t RamEnd { 0x00440000 };

// Code runs from the flash mapped at 0, an instruction outside of both is not read either
constexpr uint32_t FlashEnd { 0x00200000 };

// struct arm_registers of the SDK, r0..r12 sp lr pc spsr cpsr. Filled by the exception vector in the exception
// mode, so its sp lr and pc are that mode's own: sp points to this very struct and lr = pc = return address.
// Record keeps the interrupted mode's sp and lr instead (from the SDK backup area below) and the faulting pc
constexpr size_t CrashRegisters { 18 };
constexpr size_t CrashRegisterSp { 13 };
constexpr size_t CrashRegisterLr { 14 };
constexpr size_t CrashRegisterPc { 15 };
constexpr size_t CrashRegisterSpsr { 16 };
constexpr size_t CrashRegisterCpsr { 17 };

constexpr size_t CrashStackWords { 128 };

constexpr uint32_t PsrModeMask { 0x1f };
constexpr uint32_t PsrThumb { 1 << 5 };

// Vectors (boot_handlers.S PUSH_ALL_ARM_REG) store the banked r8..r14 of every mode at fixed addresses, the same
// ones bk_show_register prints. Address of the banked r13, r14 follows it
uintptr_t crash_banked_sp(uint32_t mode) {
    switch (mode) {
    case 0x10: // usr
    case 0x1f: // sys
        return 0x0040003c;
    case 0x12: // irq
        return 0x00400060;
    case 0x11: // fiq
        return 0x00400084;
    case 0x17: // abt
        return 0x004000a8;
    case 0x1b: // und
        return 0x004000cc;
    case 0x13: // svc
        return 0x004000f0;
    }

    return 0;
}

struct CrashRecord {
    uint32_t magic;
    uint32_t kind;
    uint32_t uptime;
    uint32_t regs[CrashRegisters];
    uint32_t insn;
    uint32_t stack[CrashStackWords];
    char task[16];
    char build[32];
    uint32_t pending;
};

static_assert(sizeof(CrashRecord) <= FLASH_SECTOR_SIZE, "");

// Not on the stack, the exception mode ones are small
CrashRecord _crash_record;

// Handlers can not ask for it, copied at boot
char _crash_build[sizeof(CrashRecord::build)] {};

const char* crash_name(uint32_t kind) {
    switch (static_cast<RtcmemCrash>(kind)) {
    case RtcmemCrash::Undefined:
        return "undefined instruction";
    case RtcmemCrash::PrefetchAbort:
        return "prefetch abort";
    case RtcmemCrash::DataAbort:
        return "data abort";
    case RtcmemCrash::Reserved:
        return "not used";
    case RtcmemCrash::None:
        break;
    }

    return "unknown";
}

// Faulting instruction, the one lr points after
uint32_t crash_pc(uint32_t lr, uint32_t spsr, RtcmemCrash kind) {
    switch (kind) {
    case RtcmemCrash::Undefined:
        return lr - (((spsr & PsrThumb) != 0) ? 2 : 4);
    case RtcmemCrash::PrefetchAbort:
        return lr - 4;
    case RtcmemCrash::DataAbort:
        return lr - 8;
    case RtcmemCrash::Reserved:
    case RtcmemCrash::None:
        break;
    }

    return lr;
}

bool crash_readable(uint32_t address, size_t size) {
    return ((address + size) <= FlashEnd)
        || ((address >= RamBegin) && ((address + size) <= RamEnd));
}

void crash_record_write(const uint32_t* regs, RtcmemCrash kind) {
    auto& record = _crash_record;
    std::memset(&record, 0, sizeof(record));

    record.magic = CrashRecordMagic;
    record.kind = static_cast<uint32_t>(kind);
    record.uptime = millis();
    std::memcpy(record.regs, regs, sizeof(record.regs));

    const auto spsr = regs[CrashRegisterSpsr];
    const auto banked = crash_banked_sp(spsr & PsrModeMask);
    if (banked) {
        record.regs[CrashRegisterSp] = *reinterpret_cast<const volatile uint32_t*>(banked);
        record.regs[CrashRegisterLr] = *reinterpret_cast<const volatile uint32_t*>(banked + sizeof(uint32_t));
    }

    const auto pc = crash_pc(regs[CrashRegisterPc], spsr, kind);
    record.regs[CrashRegisterPc] = pc;

    // tells a jump into data (heap, zeroes, erased flash) from a real undefined opcode
    if (crash_readable(pc & ~3u, sizeof(uint32_t))) {
        record.insn = *reinterpret_cast<const volatile uint32_t*>(pc & ~3u);
    }

    const auto sp = record.regs[CrashRegisterSp];
    if (((sp & 3) == 0) && (sp >= RamBegin) && (sp < RamEnd)) {
        const auto words = std::min<size_t>(CrashStackWords, (RamEnd - sp) / sizeof(uint32_t));
        std::memcpy(record.stack, reinterpret_cast<const void*>(sp), words * sizeof(uint32_t));
    }

    // no stack high water mark, the SDK builds FreeRTOS without it
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        std::strncpy(record.task, pcTaskGetName(nullptr), sizeof(record.task) - 1);
    }

    std::memcpy(record.build, _crash_build, sizeof(record.build));
    record.pending = CrashRecordPending;

    lt_flash_erase(CrashRecordOffset, FLASH_SECTOR_SIZE);
    lt_flash_write(CrashRecordOffset, reinterpret_cast<const uint8_t*>(&record), sizeof(record));
}

bool crash_record_read(CrashRecord& out) {
    return (lt_flash_read(CrashRecordOffset, reinterpret_cast<uint8_t*>(&out), sizeof(out)) == sizeof(out))
        && (out.magic == CrashRecordMagic);
}

String crash_summary(const CrashRecord& record) {
    char buffer[224];
    snprintf_P(buffer, sizeof(buffer),
        PSTR("%s pc=0x%08x insn=0x%08x lr=0x%08x sp=0x%08x spsr=0x%02x task=%.16s uptime=%u build=%.32s"),
        crash_name(record.kind),
        record.regs[CrashRegisterPc], record.insn, record.regs[CrashRegisterLr], record.regs[CrashRegisterSp],
        record.regs[CrashRegisterSpsr] & 0xff, record.task, record.uptime, record.build);

    return String(buffer);
}

#if MQTT_SUPPORT
String _crash_mqtt;
#endif

} // namespace

// Same as the SDK handlers (LibreTiny fixups/intc.c), except for the value left in the register and the record.
// Called from the exception vectors (boot_handlers.S), linked with --wrap (ref. platformio_beken.ini).
// Vectors run them in the exception mode, whose stack is only 16 bytes (_UND_STACK_SIZE_, _ABT_STACK_SIZE_) and
// already overflown by the 72-byte register frame. Anything deeper, like printf or the flash driver, writes over
// the .bss below it (wifi globals, then whatever comes next) and never finishes: the watchdog resets the chip with
// an empty record and no output. So the C part runs on its own stack, with both interrupts off
extern "C" {

struct arm_registers;
void bk_show_register(struct arm_registers*);
void bk_cpu_shutdown(void);
void bk_printf(const char*, ...);

alignas(8) uint32_t _rtcmem_crash_stack[512];
static_assert(sizeof(_rtcmem_crash_stack) == 2048, "");

[[noreturn]] void _rtcmem_crash(const uint32_t* frame, uint32_t kind) {
    // frame lives below the exception mode stack, nothing writes there from now on
    uint32_t regs[CrashRegisters];
    std::memcpy(regs, frame, sizeof(regs));

    retention() = rtcmemRetention() | kind;
    crash_record_write(regs, static_cast<RtcmemCrash>(kind));

    bk_printf("%s\n", crash_name(kind));
    bk_show_register(reinterpret_cast<struct arm_registers*>(regs));
    bk_cpu_shutdown();
    for (;;) {
    }
}

// r0 is the frame, r1 the kind
#define RTCMEM_CRASH_HANDLER(NAME, KIND)\
    __attribute__((naked, target("arm"))) void NAME(struct arm_registers*) {\
        asm volatile(\
            "mrs r2, cpsr\n"\
            "orr r2, r2, #0xc0\n"\
            "msr cpsr_c, r2\n"\
            "mov r1, #" #KIND "\n"\
            "ldr sp, =_rtcmem_crash_stack + 2048\n"\
            "ldr r2, =_rtcmem_crash\n"\
            "bx r2\n"\
            ".ltorg\n");\
    }

static_assert(static_cast<uint32_t>(RtcmemCrash::Undefined) == 1, "");
static_assert(static_cast<uint32_t>(RtcmemCrash::PrefetchAbort) == 2, "");
static_assert(static_cast<uint32_t>(RtcmemCrash::DataAbort) == 3, "");
static_assert(static_cast<uint32_t>(RtcmemCrash::Reserved) == 4, "");

RTCMEM_CRASH_HANDLER(__wrap_bk_trap_udef, 1)
RTCMEM_CRASH_HANDLER(__wrap_bk_trap_pabt, 2)
RTCMEM_CRASH_HANDLER(__wrap_bk_trap_dabt, 3)
RTCMEM_CRASH_HANDLER(__wrap_bk_trap_resv, 4)

#undef RTCMEM_CRASH_HANDLER

} // extern "C"

bool rtcmemStatus() {
    return false;
}

// Also used before setup(), by the reset reason
bool rtcmemSysStatus() {
    return (lt_get_reboot_reason() != REBOOT_REASON_POWER)
        && ((_rtcmem_retention_at_boot & RetentionMarkerMask) == RetentionMarker);
}

RtcmemCrash rtcmemCrash() {
    return rtcmemSysStatus()
        ? static_cast<RtcmemCrash>(_rtcmem_retention_at_boot & RetentionCrashMask)
        : RtcmemCrash::None;
}

uint32_t rtcmemRetention() {
    return pack(Rtcmem->sys);
}

void rtcmemSetup() {
    memset((void*)Rtcmem, 0, sizeof(RtcmemData));
    Rtcmem->magic = RTCMEM_MAGIC;

    if (rtcmemSysStatus()) {
        Rtcmem->sys = unpack(_rtcmem_retention_at_boot);
    }
}

void rtcmemCrashSetup() {
    const auto build = buildTime();
    std::memcpy(_crash_build, build.data(), std::min(build.length(), sizeof(_crash_build) - 1));

    CrashRecord record;
    if (!crash_record_read(record) || (record.pending != CrashRecordPending)) {
        return;
    }

    const auto summary = crash_summary(record);
    DEBUG_MSG_P(PSTR("[MAIN] Last crash: %s (CRASH shows the registers)\n"), summary.c_str());

    const uint32_t reported { 0 };
    lt_flash_write(CrashRecordOffset + offsetof(CrashRecord, pending),
        reinterpret_cast<const uint8_t*>(&reported), sizeof(reported));

#if MQTT_SUPPORT
    // once, on the first connection
    _crash_mqtt = summary;
    mqttRegister([](unsigned int type, espurna::StringView, espurna::StringView) {
        if ((type == MQTT_CONNECT_EVENT) && _crash_mqtt.length()) {
            mqttSendRaw(mqttTopic(F("crash")).c_str(), _crash_mqtt.c_str(), false);
            _crash_mqtt = String();
        }
    });
#endif
}

void rtcmemCrashDump(Print& print) {
    CrashRecord record;
    if (!crash_record_read(record)) {
        print.print(F("No crash recorded\n"));
        return;
    }

    print.printf_P(PSTR("Last crash: %s\n"), crash_summary(record).c_str());

    const auto* regs = record.regs;
    print.printf_P(PSTR("cpsr=0x%08x spsr=0x%08x, sp lr of the interrupted mode\n"),
        regs[CrashRegisterCpsr], regs[CrashRegisterSpsr]);
    for (size_t index = 0; index < CrashRegisterSp; index += 4) {
        print.printf_P(PSTR("r%-2u 0x%08x"), index, regs[index]);
        for (size_t next = index + 1; (next < index + 4) && (next < CrashRegisterSp); ++next) {
            print.printf_P(PSTR("  r%-2u 0x%08x"), next, regs[next]);
        }
        print.print('\n');
    }

    print.printf_P(PSTR("stack at 0x%08x:\n"), regs[CrashRegisterSp]);
    for (size_t index = 0; index < CrashStackWords; index += 8) {
        for (size_t next = index; next < index + 8; ++next) {
            print.printf_P(PSTR("%08x "), record.stack[next]);
        }
        print.print('\n');
    }
}

#endif
