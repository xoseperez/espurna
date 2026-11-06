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
    case RtcmemCrash::MallocFailed:
        return "malloc failed";
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
    case RtcmemCrash::MallocFailed:
    case RtcmemCrash::None:
        break;
    }

    return lr;
}

bool crash_readable(uint32_t address, size_t size) {
    return ((address + size) <= FlashEnd)
        || ((address >= RamBegin) && ((address + size) <= RamEnd));
}

void crash_flash_write(size_t offset, const void* data, size_t size) {
    lt_flash_write(CrashRecordOffset + offset, static_cast<const uint8_t*>(data), size);
}

// Written field by field straight from where the data is, no copy of the whole record in RAM. What is not
// written stays erased: stack words past the end of RAM read 0xffffffff and 'pending' is already all ones
void crash_record_write(const uint32_t* frame, RtcmemCrash kind) {
    uint32_t regs[CrashRegisters];
    std::memcpy(regs, frame, sizeof(regs));

    const auto spsr = regs[CrashRegisterSpsr];
    const auto banked = crash_banked_sp(spsr & PsrModeMask);
    if (banked) {
        regs[CrashRegisterSp] = *reinterpret_cast<const volatile uint32_t*>(banked);
        regs[CrashRegisterLr] = *reinterpret_cast<const volatile uint32_t*>(banked + sizeof(uint32_t));
    }

    const auto pc = crash_pc(regs[CrashRegisterPc], spsr, kind);
    regs[CrashRegisterPc] = pc;

    // tells a jump into data (heap, zeroes, erased flash) from a real undefined opcode
    uint32_t insn { 0 };
    if (crash_readable(pc & ~3u, sizeof(uint32_t))) {
        insn = *reinterpret_cast<const volatile uint32_t*>(pc & ~3u);
    }

    // no stack high water mark, the SDK builds FreeRTOS without it
    char task[sizeof(CrashRecord::task)] {};
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        std::strncpy(task, pcTaskGetName(nullptr), sizeof(task) - 1);
    }

    const uint32_t head[] {
        CrashRecordMagic,
        static_cast<uint32_t>(kind),
        static_cast<uint32_t>(millis()),
    };

    static_assert(offsetof(CrashRecord, magic) == 0, "");
    static_assert(offsetof(CrashRecord, uptime) == sizeof(head) - sizeof(uint32_t), "");

    lt_flash_erase(CrashRecordOffset, FLASH_SECTOR_SIZE);
    crash_flash_write(offsetof(CrashRecord, magic), head, sizeof(head));
    crash_flash_write(offsetof(CrashRecord, regs), regs, sizeof(regs));
    crash_flash_write(offsetof(CrashRecord, insn), &insn, sizeof(insn));

    const auto sp = regs[CrashRegisterSp];
    if (((sp & 3) == 0) && (sp >= RamBegin) && (sp < RamEnd)) {
        const auto words = std::min<size_t>(CrashStackWords, (RamEnd - sp) / sizeof(uint32_t));
        crash_flash_write(offsetof(CrashRecord, stack), reinterpret_cast<const void*>(sp), words * sizeof(uint32_t));
    }

    crash_flash_write(offsetof(CrashRecord, task), task, sizeof(task));
    crash_flash_write(offsetof(CrashRecord, build), _crash_build, sizeof(_crash_build));
}

bool crash_record_read(CrashRecord& out) {
    return (lt_flash_read(CrashRecordOffset, reinterpret_cast<uint8_t*>(&out), sizeof(out)) == sizeof(out))
        && (out.magic == CrashRecordMagic);
}

String crash_summary(const CrashRecord& record) {
    char buffer[224];
    if (record.kind == static_cast<uint32_t>(RtcmemCrash::MallocFailed)) {
        snprintf_P(buffer, sizeof(buffer),
            PSTR("%s free=%u lowest=%u in=0x%08x sp=0x%08x task=%.16s uptime=%u build=%.32s"),
            crash_name(record.kind), record.regs[0], record.regs[1], record.regs[CrashRegisterPc],
            record.regs[CrashRegisterSp], record.task, record.uptime, record.build);
        return String(buffer);
    }

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
// an empty record and no output. So the C part runs on a borrowed stack, with both interrupts off: the FIQ one
// (_FIQ_STACK_SIZE_ 0x7f0), unused once FIQ is masked, or the IRQ one (0xff0) when the crash happened in FIQ mode
// and the FIQ stack is the one being dumped. Nothing returns from here, so their old contents do not matter.
// No buffer of our own: interrupt stacks and heap are the RAM that Matter boards are short of
extern "C" {

struct arm_registers;
void bk_show_register(struct arm_registers*);
void bk_cpu_shutdown(void);
void bk_printf(const char*, ...);

[[noreturn]] void _rtcmem_crash(const uint32_t* frame, uint32_t kind) {
    // frame lives below the exception mode stack, the borrowed one is elsewhere (boot_handlers.S .bss order:
    // und, abt, fiq, irq), so it stays as it is
    retention() = rtcmemRetention() | kind;
    crash_record_write(frame, static_cast<RtcmemCrash>(kind));

    bk_printf("%s\n", crash_name(kind));
    bk_show_register(reinterpret_cast<struct arm_registers*>(const_cast<uint32_t*>(frame)));
    bk_cpu_shutdown();
    for (;;) {
    }
}

// r0 is the frame, r1 the kind. Frame spsr (word 16) is the interrupted cpsr, mode 0x11 is FIQ
#define RTCMEM_CRASH_HANDLER(NAME, KIND)\
    __attribute__((naked, target("arm"))) void NAME(struct arm_registers*) {\
        asm volatile(\
            "mrs r2, cpsr\n"\
            "orr r2, r2, #0xc0\n"\
            "msr cpsr_c, r2\n"\
            "mov r1, #" #KIND "\n"\
            "ldr r2, [r0, #64]\n"\
            "and r2, r2, #0x1f\n"\
            "cmp r2, #0x11\n"\
            "ldreq sp, =irq_stack_start + 0xff0\n"\
            "ldrne sp, =fiq_stack_start + 0x7f0\n"\
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

// FreeRTOS calls it from pvPortMalloc() (heap_4, scheduler suspended) when a block can not be found, the SDK one
// is empty. Nothing faults afterwards: the caller gets NULL, writes near 0 are ignored on BK7231N and the chip
// usually hangs until the watchdog, without any trace. So the first failure of a boot leaves a record, reported at
// the next boot like a crash. The hook has no arguments: r0 = free heap, r1 = lowest free heap ever, pc = lr =
// the return address inside pvPortMalloc() and the stack words hold its caller and the requested size.
// Linked with --wrap (ref. platformio_beken.ini). Runs on the stack of the task that asked, keep it small
namespace {

bool _rtcmem_malloc_failed { false };

} // namespace

size_t xPortGetFreeHeapSize(void);
size_t xPortGetMinimumEverFreeHeapSize(void);

void __wrap_vApplicationMallocFailedHook(void) {
    if (_rtcmem_malloc_failed) {
        return;
    }

    _rtcmem_malloc_failed = true;

    uint32_t regs[CrashRegisters] {};
    regs[0] = xPortGetFreeHeapSize();
    regs[1] = xPortGetMinimumEverFreeHeapSize();

    uint32_t sp;
    asm volatile("mov %0, sp" : "=r"(sp));
    regs[CrashRegisterSp] = sp;

    const auto ret = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
    regs[CrashRegisterLr] = ret;
    regs[CrashRegisterPc] = ret;

    // spsr 0 is no mode, the record keeps the sp and lr above
    crash_record_write(regs, RtcmemCrash::MallocFailed);
}

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
