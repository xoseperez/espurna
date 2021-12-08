/*

RTMEM MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"
#include "rtcmem.h"

volatile RtcmemData* Rtcmem = reinterpret_cast<volatile RtcmemData*>(RTCMEM_ADDR);

namespace {

bool _rtcmem_status = false;

void _rtcmemErase() {
    auto ptr = reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR);
    const auto end = ptr + RTCMEM_BLOCKS;
    DEBUG_MSG_P(PSTR("[RTCMEM] Erasing start=%p end=%p\n"), ptr, end);
    do {
        *ptr = 0;
    } while (++ptr != end);
}

void _rtcmemInit() {
    _rtcmemErase();
    Rtcmem->magic = RTCMEM_MAGIC;
}

// Treat memory as dirty on cold boot, hardware wdt reset and rst pin
bool _rtcmemStatus() {
    bool readable;

    switch (systemResetReason()) {
        case REASON_EXT_SYS_RST:
        case REASON_WDT_RST:
        case REASON_DEFAULT_RST:
            readable = false;
            break;
        default:
            readable = true;
    }

    readable = readable and (RTCMEM_MAGIC == Rtcmem->magic);

    return readable;
}

#if TERMINAL_SUPPORT

void _rtcmemInitCommands() {
    terminalRegisterCommand(F("RTCMEM.REINIT"), [](::terminal::CommandContext&&) {
        _rtcmemInit();
    });

    #if DEBUG_SUPPORT
        terminalRegisterCommand(F("RTCMEM.DUMP"), [](::terminal::CommandContext&&) {

            DEBUG_MSG_P(PSTR("[RTCMEM] boot_status=%u status=%u blocks_used=%u\n"),
                _rtcmem_status, _rtcmemStatus(), RtcmemSize);

            String line;
            line.reserve(96);
            char buffer[16] = {0};

            auto addr = reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR);

            uint8_t block = 1;
            uint8_t offset = 0;
            uint8_t start = 0;

            do {

                offset = block - 1;

                snprintf(buffer, sizeof(buffer), "%08x ", *(addr + offset));
                line += buffer;

                if ((block % 8) == 0) {
                    DEBUG_MSG_P(PSTR("%02u %p: %s\n"), start, addr+start, line.c_str());
                    start = block;
                    line = "";
                }

                ++block;

            } while (block<(RTCMEM_BLOCKS+1));

        });
    #endif
}

#endif

} // namespace

bool rtcmemStatus() {
    return _rtcmem_status;
}

void rtcmemSetup() {
    _rtcmem_status = _rtcmemStatus();
    if (!_rtcmem_status) {
        _rtcmemInit();
    }

    #if TERMINAL_SUPPORT
        _rtcmemInitCommands();
    #endif
}
