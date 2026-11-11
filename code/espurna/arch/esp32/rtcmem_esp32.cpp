/*

RTMEM MODULE FOR ESP32

*/

#include "espurna.h"
#include "arch/esp32/rtcmem_esp32.h"

#if defined(ESP32)

#include <esp_system.h>

// Safe static buffer for ESP32, placed in RTC memory to persist across reboots
RTC_NOINIT_ATTR static RtcmemData _rtcmem_storage;
volatile RtcmemData* Rtcmem = &_rtcmem_storage;

namespace espurna {
namespace peripherals {
namespace {
namespace rtc {
namespace internal {
    bool status = false;
}

void init() {
    memset((void*)Rtcmem, 0, sizeof(RtcmemData));
    Rtcmem->magic = RTCMEM_MAGIC;
}

// Treat RTC_NOINIT memory as dirty after cold boot, RST-pin, or brownout —
// matches the ESP8266 path which discards REASON_EXT_SYS_RST/REASON_DEFAULT_RST.
// RTC_NOINIT_ATTR retains values only across SW/WDT/panic resets and deep sleep.
//
// TODO: Warm-reset paths (SW/WDT/panic) still trust the 32-bit `magic` alone.
// A CRC across RtcmemData would close the 1/2^32 collision window on first
// boot when RTC RAM happens to hold our magic value by chance.
bool status() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:
        case ESP_RST_EXT:
        case ESP_RST_BROWNOUT:
        case ESP_RST_UNKNOWN:
            return false;
        default:
            break;
    }
    return Rtcmem && (Rtcmem->magic == RTCMEM_MAGIC);
}

void setup() {
    internal::status = status();
    if (!internal::status) {
        init();
    }
}

} // namespace rtc
} // namespace
} // namespace peripherals
} // namespace espurna

bool rtcmemStatus() {
    return espurna::peripherals::rtc::internal::status;
}

void rtcmemSetup() {
    espurna::peripherals::rtc::setup();
}

#endif
