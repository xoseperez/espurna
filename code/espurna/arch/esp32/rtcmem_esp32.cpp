/*

RTMEM MODULE FOR ESP32

*/

#include "espurna.h"
#include "arch/esp32/rtcmem_esp32.h"

#if defined(ESP32)

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

bool status() {
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
