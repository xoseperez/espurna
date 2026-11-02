/*

RTMEM MODULE FOR BEKEN (LibreTiny)

*/

#include "espurna.h"
#include "arch/beken/rtcmem_beken.h"

#if defined(LIBRETINY)

// No memory known to survive the reset (bootloader runs first and the startup code clears .bss),
// contents only live until the next reboot. Relay state is restored from settings instead.
static RtcmemData _rtcmem_storage;
volatile RtcmemData* Rtcmem = &_rtcmem_storage;

bool rtcmemStatus() {
    return false;
}

void rtcmemSetup() {
    memset((void*)Rtcmem, 0, sizeof(RtcmemData));
    Rtcmem->magic = RTCMEM_MAGIC;
}

#endif
