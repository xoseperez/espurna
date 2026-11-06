/*

BK7231N: ONE OTA FILE FOR BOTH BOOTLOADERS (ref. ota_beken.h, scripts/beken_image.py)

*/

#include "espurna.h"

#if defined(LIBRETINY)

#include "layout_beken.h"
#include "ota_beken.h"

#if MATTER_SUPPORT
#include "espurna_matter.h"
#endif

#include <cstdlib>
#include <cstring>

#if MATTER_TAIL_SUPPORT
extern "C" const uint8_t __build_id_start[];
#endif

namespace espurna {
namespace beken {
namespace ota {
namespace {

constexpr char Magic[] = "ESPBEKEN";
constexpr size_t BuildIdSize { 20 };
constexpr size_t PartsMax { 4 };
constexpr uint32_t SectorSize { 0x1000 };

enum class Kind : uint32_t {
    None = 0,
    RblTuya = 1,
    RblQuectel = 2,
    MatterTail = 3,
};

struct Header {
    char magic[8];
    uint32_t version;
    uint32_t parts;
    uint8_t build_id[BuildIdSize];
    uint8_t reserved[28];
};

struct Part {
    uint32_t kind;
    uint32_t offset;
    uint32_t length;
    uint32_t crc;
    uint32_t reserved[4];
};

static_assert(sizeof(Header) == 64, "");
static_assert(sizeof(Part) == 32, "");

struct State {
    bool active { false };
    uint8_t head[sizeof(Header) + PartsMax * sizeof(Part)];
    size_t head_size { 0 };
    size_t head_needed { sizeof(Header) };
    Header header {};
    Part part {};
    Kind kind { Kind::None };
    size_t position { 0 };   // in the uploaded file
    size_t written { 0 };    // of the selected part
    uint32_t crc { 0 };
    uint32_t erased { 0 };
    // first bytes of the RBL (its magic): written last, into the erased flash. An interrupted upload leaves no
    // package for the bootloader to install. No heap needed for that, it may be short when an update is due
    uint8_t first[16];
};

State state;

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

void release() {
}

bool same_build() {
#if MATTER_TAIL_SUPPORT
    return std::memcmp(state.header.build_id, __build_id_start + 16, BuildIdSize) == 0;
#else
    return false;
#endif
}

const char* select() {
    std::memcpy(&state.header, state.head, sizeof(Header));
    if (state.header.version != 1) {
        return PSTR("ERROR: Unknown image version");
    }

    const auto& layout = flashLayout();
    auto wanted = Kind::None;
    switch (layout.layout) {
    case Layout::Tuya:
        // the image is running already: what is left is its Matter code
        wanted = same_build() ? Kind::MatterTail : Kind::RblTuya;
        break;
    case Layout::Quectel:
        wanted = Kind::RblQuectel;
        break;
    case Layout::Unknown:
        return PSTR("ERROR: Unknown bootloader, no OTA on this board");
    }

    const auto* parts = reinterpret_cast<const Part*>(&state.head[sizeof(Header)]);
    for (size_t index = 0; index < state.header.parts; ++index) {
        if (static_cast<Kind>(parts[index].kind) == wanted) {
            state.part = parts[index];
            state.kind = wanted;
            break;
        }
    }

    if (state.kind == Kind::None) {
        return PSTR("ERROR: The image has nothing for this board");
    }

    if (state.part.offset < state.head_needed) {
        return PSTR("ERROR: Broken image");
    }

    if (state.kind == Kind::MatterTail) {
#if MATTER_TAIL_SUPPORT
        return matterTailBegin();
#else
        return PSTR("ERROR: No Matter in this firmware");
#endif
    }

    if (state.part.length > layout.download_length) {
        return PSTR("ERROR: The image does not fit the OTA partition");
    }

    // nothing to install until the upload is complete: the bootloader looks for the RBL magic at the start
    if (!lt_flash_erase_block(layout.download_offset)) {
        return PSTR("ERROR: Flash erase failed");
    }

    state.erased = layout.download_offset + SectorSize;
    DEBUG_MSG_P(PSTR("[UPGRADE] %s image, %u bytes to 0x%06X\n"),
        layoutName(layout.layout), state.part.length, layout.download_offset);
    return nullptr;
}

const char* write_rbl(const uint8_t* data, size_t length) {
    const auto& layout = flashLayout();
    while (length) {
        if (state.written < sizeof(state.first)) {
            const auto chunk = std::min(length, sizeof(state.first) - state.written);
            std::memcpy(&state.first[state.written], data, chunk);
            state.crc = crc32(state.crc, data, chunk);
            state.written += chunk;
            data += chunk;
            length -= chunk;
            continue;
        }

        const uint32_t offset = layout.download_offset + state.written;
        while (state.erased < (offset + length)) {
            if (!lt_flash_erase_block(state.erased)) {
                return PSTR("ERROR: Flash erase failed");
            }
            state.erased += SectorSize;
        }

        if (lt_flash_write(offset, data, length) != length) {
            return PSTR("ERROR: Flash write failed");
        }

        state.crc = crc32(state.crc, data, length);
        state.written += length;
        length = 0;
    }

    return nullptr;
}

// the part only, the rest of the file is skipped
const char* write_part(const uint8_t* data, size_t length) {
    const size_t start = state.part.offset;
    const size_t end = start + state.part.length;
    const size_t from = std::max(state.position, start);
    const size_t to = std::min(state.position + length, end);
    state.position += length;

    if (from >= to) {
        return nullptr;
    }

    const auto* chunk = data + (from - (state.position - length));
    const size_t size = to - from;
    if (state.kind == Kind::MatterTail) {
#if MATTER_TAIL_SUPPORT
        state.crc = crc32(state.crc, chunk, size);
        state.written += size;
        return matterTailWrite(chunk, size);
#endif
    }

    return write_rbl(chunk, size);
}

} // namespace
} // namespace ota
} // namespace beken
} // namespace espurna

using namespace espurna::beken::ota;

bool bekenImageIsUpload(const uint8_t* data, size_t length) {
    return (length >= (sizeof(Magic) - 1)) && (std::memcmp(data, Magic, sizeof(Magic) - 1) == 0);
}

const char* bekenImageBegin() {
    release();
    state = State{};
    state.active = true;
    return nullptr;
}

const char* bekenImageWrite(const uint8_t* data, size_t length) {
    if (!state.active) {
        return PSTR("ERROR: No upload");
    }

    while (length && (state.head_size < state.head_needed)) {
        const auto chunk = std::min(length, state.head_needed - state.head_size);
        std::memcpy(&state.head[state.head_size], data, chunk);
        state.head_size += chunk;
        state.position += chunk;
        data += chunk;
        length -= chunk;

        if (state.head_size == sizeof(Header)) {
            const auto* header = reinterpret_cast<const Header*>(state.head);
            if (!header->parts || (header->parts > PartsMax)) {
                state.active = false;
                return PSTR("ERROR: Broken image");
            }
            state.head_needed = sizeof(Header) + header->parts * sizeof(Part);
        }

        if ((state.head_size == state.head_needed) && (state.head_needed > sizeof(Header))) {
            const auto* error = select();
            if (error) {
                state.active = false;
                release();
                return error;
            }
        }
    }

    if (!length) {
        return nullptr;
    }

    const auto* error = write_part(data, length);
    if (error) {
        state.active = false;
        release();
    }

    return error;
}

const char* bekenImageEnd() {
    if (!state.active) {
        return PSTR("ERROR: No upload");
    }

    state.active = false;
    if ((state.kind == Kind::None) || (state.written != state.part.length)) {
        release();
        return PSTR("ERROR: Incomplete image");
    }

    if (state.crc != state.part.crc) {
        release();
        return PSTR("ERROR: Image CRC mismatch");
    }

    if (state.kind == Kind::MatterTail) {
#if MATTER_TAIL_SUPPORT
        return matterTailEnd();
#else
        return PSTR("ERROR: No Matter in this firmware");
#endif
    }

    // now the package becomes visible to the bootloader
    const auto& layout = espurna::beken::flashLayout();
    const auto first = std::min(sizeof(state.first), state.written);
    const bool ok = lt_flash_write(layout.download_offset, state.first, first) == first;
    release();
    if (!ok) {
        return PSTR("ERROR: Flash write failed");
    }

    uint32_t magic { 0 };
    lt_flash_read(layout.download_offset, reinterpret_cast<uint8_t*>(&magic), sizeof(magic));
    if (magic != 0x004C4252) { // "RBL\0"
        lt_flash_erase_block(layout.download_offset);
        return PSTR("ERROR: Not an OTA package");
    }

    DEBUG_MSG_P(PSTR("[UPGRADE] Image written, %u bytes\n"), state.written);
    return nullptr;
}

void bekenImageAbort() {
    if (state.kind == Kind::MatterTail) {
#if MATTER_TAIL_SUPPORT
        matterTailAbort();
#endif
    }

    state.active = false;
    release();
}

bool bekenImageAgain() {
#if MATTER_TAIL_SUPPORT
    return state.kind == Kind::RblTuya;
#else
    return false;
#endif
}

// LibreTiny Update writes the 'download' partition of the layout this firmware was built for
const char* bekenUf2Allowed() {
    const auto& layout = espurna::beken::flashLayout();
    if ((layout.layout != espurna::beken::Layout::Unknown) && (layout.download_offset != FLASH_DOWNLOAD_OFFSET)) {
        return PSTR("ERROR: This firmware was built for another bootloader, upload espurna-beken-matter.bin");
    }

    return nullptr;
}

#endif
