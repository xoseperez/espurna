bool _rtcmem_status = false;

void _rtcmemInit() {
    memset((uint32_t*)RTCMEM_ADDR, 0, sizeof(uint32_t) * RTCMEM_BLOCKS);
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
    terminalRegisterCommand(F("RTCMEM.REINIT"), [](Embedis* e) {
        _rtcmemInit();
    });

    terminalRegisterCommand(F("RTCMEM.TEST"), [](Embedis* e) {
    });

    terminalRegisterCommand(F("RTCMEM.DUMP"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("[RTCMEM] status:%u blocks:%u addr:0x%p\n"),
            _rtcmemStatus(), RtcmemSize, Rtcmem);

        for (uint8_t block=0; block<RtcmemSize; ++block) {
            DEBUG_MSG_P(PSTR("[RTCMEM] %02u: %u\n"),
                block, reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR)[block]);
        }
    });
}

#endif

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
