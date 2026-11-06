/*

BK7231N FLASH LAYOUT, PICKED AT BOOT

*/

#pragma once

#include <cstddef>
#include <cstdint>

namespace espurna {
namespace beken {

// Which bootloader the board has: it decides where an OTA image goes and how much of the image fits there.
// Same firmware on both, ref. scripts/beken_image.py
enum class Layout {
    Unknown,
    Tuya,    // Tuya BK7231N bootloader, 2 MiB: CB2S, CB3S, CBU ... (flash encryption key of Tuya)
    Quectel, // Quectel bootloader, 4 MiB, no flash encryption: XH-CB2S / BK7231M (Sparkleiot Mini)
};

struct FlashLayout {
    Layout layout;
    uint32_t flash_size;
    // where the bootloader picks an OTA image (RBL) up from
    uint32_t download_offset;
    uint32_t download_length;
    // the application, as the bootloader installs it (with the CRC of every 32 bytes)
    uint32_t app_offset;
    uint32_t app_length;
    // Matter factory data of the stock firmware, 0 when there is none
    uint32_t matter_offset;
    uint32_t matter_length;
};

const FlashLayout& flashLayout();
const char* layoutName(Layout);

} // namespace beken
} // namespace espurna
