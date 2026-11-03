/*

RTMEM MODULE FOR BEKEN (LibreTiny)

*/

#include "espurna.h"
#include "arch/beken/rtcmem_beken.h"

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

} // namespace

// Same as the SDK handlers (LibreTiny fixups/intc.c), except for the value left in the register.
// Called from the exception vectors (boot_handlers.S), linked with --wrap (ref. platformio_beken.ini)
extern "C" {

struct arm_registers;
void bk_show_register(struct arm_registers*);
void bk_cpu_shutdown(void);
void bk_printf(const char*, ...);

namespace {

[[noreturn]] void _rtcmem_crash(struct arm_registers* regs, RtcmemCrash kind, const char* name) {
    retention() = rtcmemRetention() | static_cast<uint32_t>(kind);
    bk_printf("%s\n", name);
    bk_show_register(regs);
    bk_cpu_shutdown();
    for (;;) {
    }
}

} // namespace

void __wrap_bk_trap_udef(struct arm_registers* regs) {
    _rtcmem_crash(regs, RtcmemCrash::Undefined, "undefined instruction");
}

void __wrap_bk_trap_pabt(struct arm_registers* regs) {
    _rtcmem_crash(regs, RtcmemCrash::PrefetchAbort, "prefetch abort");
}

void __wrap_bk_trap_dabt(struct arm_registers* regs) {
    _rtcmem_crash(regs, RtcmemCrash::DataAbort, "data abort");
}

void __wrap_bk_trap_resv(struct arm_registers* regs) {
    _rtcmem_crash(regs, RtcmemCrash::Reserved, "not used");
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

#endif
