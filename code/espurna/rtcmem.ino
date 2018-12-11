uint32_t _rtcmem_crctime = 0;
bool _rtcmem_status = false;

// TODO different crc method?
// TODO generic crc32 as util?
// version adapted from arduino core, see: bootloaders/eboot/eboot_command.c
uint32_t _rtcmem_crc(uint32_t crc, uint8_t* data, size_t length) {
    while (length--) {
        uint8_t c = *data++;
        for (uint32_t i = 0x80; i > 0; i >>= 1) {
            bool bit = crc & 0x80000000;
            if (c & i) {
                bit = !bit;
            }
            crc <<= 1;
            if (bit) {
                crc ^= 0x04c11db7;
            }
        }
    }
    return crc;
}

// calculate crc over block range
uint32_t _rtcmem_crc_blocks(uint32_t crc, uint8_t first, uint8_t last) {
    uint8_t _block = first;

    union {
        uint32_t block;
        uint8_t bytes[4];
    } value;

    uint32_t crc_start = micros();

    do {
        value.block = reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR)[_block];
        crc = _rtcmem_crc(crc, value.bytes, 4);
    } while(_block++ < last);

    _rtcmem_crctime = micros() - crc_start;

    return crc;
}

// calculate crc over used blocks, skipping crc in block 0
uint32_t _rtcmem_crc_blocks() {
    return _rtcmem_crc_blocks(0xffffffff, 1, RtcmemSize);
}

void _rtcmemInit() {
    memset((uint32_t*)RTCMEM_ADDR, 0, sizeof(uint32_t) * RTCMEM_BLOCKS);
    Rtcmem->magic = RTCMEM_MAGIC;
}

bool _rtcmemStatus() {
    return (RTCMEM_MAGIC == Rtcmem->magic) and (Rtcmem->crc32 == _rtcmem_crc_blocks());
}

#if TERMINAL_SUPPORT

void _rtcmemInitCommands() {
    settingsRegisterCommand(F("RTCMEM"), [](Embedis* e) {
        rtcmemDebug();
    });

    settingsRegisterCommand(F("RTCMEM.REINIT"), [](Embedis* e) {
        _rtcmemInit();
    });

    settingsRegisterCommand(F("RTCMEM.DUMP"), [](Embedis* e) {
        DEBUG_MSG_P(PSTR("[RTCMEM] crc32:%u blocks:%u addr:0x%p\n"),
            _rtcmemStatus(), RtcmemSize, Rtcmem);

        for (uint8_t block=0; block<RtcmemSize; ++block) {
            DEBUG_MSG_P(PSTR("[RTCMEM] %02u: %u\n"),
                block, reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR)[block]);
        }
    });
}

#endif

// if any data was modified without this call, everything in rtcmem will be overwritten on the next Setup()
void rtcmemCommit() {
    Rtcmem->crc32 = _rtcmem_crc_blocks();
}

void rtcmemDebug() {
    DEBUG_MSG_P(PSTR("[RTCMEM] Blocks used: %u\n"), RtcmemSize);
    DEBUG_MSG_P(PSTR("[RTCMEM] CRC algorithm: CRC32\n"));
    DEBUG_MSG_P(PSTR("[RTCMEM] Current CRC32: 0x%X\n"), Rtcmem->crc32);
    DEBUG_MSG_P(PSTR("[RTCMEM] Time spent doing last CRC32: %u (µs)\n"), _rtcmem_crctime);
}

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
