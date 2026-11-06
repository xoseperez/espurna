/*

BK7231N FLASH LAYOUT, PICKED AT BOOT

One image for the boards of both bootloaders. The bootloader is told apart by the CRC32 of its flash region, the same
on every board of a vendor (checked against the stock dumps: CB3S and CBU, two XH-CB2S). An unknown bootloader gets
no layout at all: nothing is written to its partitions then (no OTA, no Matter code).

*/

#include "espurna.h"

#if defined(LIBRETINY)

#include "layout_beken.h"

namespace espurna {
namespace beken {
namespace {

constexpr uint32_t BootloaderSize { 0x11000 };

struct Known {
    uint32_t crc;
    FlashLayout layout;
};

constexpr Known KnownLayouts[] {
    {0xf684a1dc, {Layout::Tuya, 0x200000,
        0x12A000, 0xA6000,      // download
        0x011000, 0x119000,     // app
        0, 0}},
    {0x64b02979, {Layout::Quectel, 0x400000,
        0x286000, 0x15A000,
        0x011000, 0x1A9000,
        0x3E0000, 0x020000}},   // Quectel Matter factory data ("chip-factory")
};

uint32_t crc32(uint32_t crc, const uint8_t* data, size_t length) {
    crc = ~crc;
    for (size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320 & (0 - (crc & 1)));
        }
    }

    return ~crc;
}

FlashLayout detect() {
    FlashLayout out {};
    out.flash_size = lt_flash_get_size();

    uint8_t buffer[256];
    uint32_t crc { 0 };
    for (uint32_t offset = 0; offset < BootloaderSize; offset += sizeof(buffer)) {
        if (lt_flash_read(offset, buffer, sizeof(buffer)) != sizeof(buffer)) {
            return out;
        }
        crc = crc32(crc, buffer, sizeof(buffer));
    }

    for (const auto& known : KnownLayouts) {
        if ((known.crc == crc) && (known.layout.flash_size <= out.flash_size)) {
            return known.layout;
        }
    }

    DEBUG_MSG_P(PSTR("[MAIN] Unknown bootloader (CRC32 %08X), flash %u KiB: no OTA\n"),
        crc, out.flash_size / 1024);
    return out;
}

} // namespace

const FlashLayout& flashLayout() {
    static const FlashLayout layout = detect();
    return layout;
}

const char* layoutName(Layout value) {
    switch (value) {
    case Layout::Tuya:
        return "Tuya";
    case Layout::Quectel:
        return "Quectel";
    case Layout::Unknown:
        break;
    }

    return "unknown";
}

} // namespace beken
} // namespace espurna

#endif
