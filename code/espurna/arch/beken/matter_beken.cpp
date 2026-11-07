/*

MATTER BACKEND FOR BEKEN (LibreTiny)

Matter On/Off plug (connectedhomeip, prebuilt libMatterApp.a of matter-bk7231n) on top of ESPurna. ESPurna keeps
Wi-Fi, the relay and the web UI (matter.cpp); Matter gets the bk_* platform API it was written against (matter_pal.h
of the BK7231N port):
- "chip-factory" namespace: read only, straight from the factory data the stock firmware left in flash: the factory
  table of the Quectel SDK (Sparkleiot XH-CB2S), or the encrypted key-value store of Tuya's Matter firmware (MOES
  MS-104B Matter, CBU, ref. tuya_kv_beken.h): DAC and its key, PAI, certification declaration, passcode verifier,
  discriminator, VID / PID. Nothing is ever written there
- every other namespace (chip-config, chip-counters, the KVS of fabrics, ACL ...): FlashDB on the 'kvs' partition
- Wi-Fi status and the connected / got IP events from the ESPurna wifi module
- entropy from the TRNG of the chip

*/

#include "espurna.h"

#if defined(LIBRETINY) && MATTER_SUPPORT

#include "espurna_matter.h"
#include "layout_beken.h"
#include "relay.h"
#include "terminal.h"
#include "tuya_kv_beken.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <flashdb.h>

#include <lwip/igmp.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#if defined(MATTER_UDP_LOG)
#include <lwip/udp.h>
#endif

#include <algorithm>
#include <atomic>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------------------------------------------------
// The C API between ESPurna and the Matter library (matter_pal.h and matter_app.cpp of the BK7231N port)

extern "C" {

typedef int bk_err_t;

typedef enum {
    EVENT_MOD_WIFI,
    EVENT_MOD_NETIF,
} event_module_t;

typedef int (*event_cb_t)(void* arg, event_module_t event_module, int event_id, void* event_data);

typedef struct {
    int disconnect_reason;
    bool local_generated;
} matter_sta_disconnected_t;

typedef struct {
    char ssid[33];
    uint8_t bssid[6];
    uint8_t channel;
    int security;
    char password[65];
} wifi_sta_config_t;

typedef struct {
    char ssid[33];
    uint8_t bssid[6];
    uint8_t channel;
    int8_t rssi;
    int security;
} wifi_link_status_t;

typedef struct {
    char ssid[33];
} wifi_scan_config_t;

typedef struct {
    char ssid[33];
    uint8_t bssid[6];
    int rssi;
    uint8_t channel;
    int security;
} wifi_scan_ap_info_t;

typedef struct {
    int ap_num;
    wifi_scan_ap_info_t* aps;
} wifi_scan_result_t;

int matter_app_start(void);
// plug endpoints of the library (1..4); libraries before multi-plug have only endpoint 1 and no such function
uint8_t matter_app_plugs(void) __attribute__((weak));
void matter_set_onoff(uint8_t endpoint, bool on);
bool matter_onboarding_codes(char* manual, size_t manual_size, char* qr, size_t qr_size);
int matter_fabric_count(void);
// controller list / removal; libraries before them do not have these
bool matter_fabric_info(int n, uint8_t* index, uint16_t* vendor, uint64_t* node, char* label, size_t label_size)
    __attribute__((weak));
bool matter_remove_fabric(uint8_t index) __attribute__((weak));
void matter_open_window(void);
void matter_ping(void);

} // extern "C"

namespace espurna {
namespace matter {
namespace {

constexpr int BkOk { 0 };
constexpr int BkFail { -1 };

constexpr int EventWifiStaConnected { 1 };
constexpr int EventWifiStaDisconnected { 2 };
constexpr int EventNetifGotIp4 { 0 };

constexpr char FactoryNamespace[] = "chip-factory";

// ---------------------------------------------------------------------------------------------------------------------

namespace factory {

// Namespace "chip-factory" of the stock key-value store. Table of 64-byte entries, each one
// {0x55 0xff 0xff 0xff, offset, length, name[32]}, values right after the table
// Factory data partition of the board, when its bootloader / layout has one (ref. layout_beken.cpp)
constexpr uint32_t TableStart { 0x7000 };
constexpr uint32_t TableSize { 0x1000 };
constexpr uint32_t DataStart { 0x8000 };
constexpr uint32_t DataSize { 0x2000 };
constexpr uint32_t EntryMagic { 0xffffff55 };

struct __attribute__((packed)) Entry {
    uint32_t magic;
    uint32_t offset;
    uint32_t length;
    char name[32];
    uint8_t reserved[20];
};

static_assert(sizeof(Entry) == 0x40, "");

// Copies at most size bytes, length is the full one
bool copy(const void* data, size_t data_length, uint8_t* out, size_t size, size_t& length) {
    length = data_length;
    const auto count = std::min(size, data_length);
    if (out && count) {
        std::memcpy(out, data, count);
    }

    return true;
}

// little endian, as the Quectel table keeps the numbers
bool copy_number(uint32_t value, size_t bytes, uint8_t* out, size_t size, size_t& length) {
    const uint8_t data[4] {
        static_cast<uint8_t>(value),
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value >> 16),
        static_cast<uint8_t>(value >> 24)};
    return copy(data, bytes, out, size, length);
}

namespace tuya {

// Tuya's Matter firmware has no factory partition: the identity is in the encrypted key-value store of the Tuya SDK at
// the end of the flash, PEM and JSON (ref. tuya_kv_beken.h). Decrypted and converted once, in the loop (backend::start(),
// MATTER.FACTORY, product()) before the Matter task asks for it, and kept: the library reads it at every
// commissioning step. Only on the Tuya layout, the store is out of reach of ESPurna's own partitions
using ::espurna::beken::tuya_kv::MatterFactory;

enum class State : uint8_t {
    Unknown,
    None,
    Found,
};

State state { State::Unknown };
MatterFactory* data { nullptr };

bool flash_read(uint32_t offset, uint8_t* out, size_t size) {
    return lt_flash_read(offset, out, size) == size;
}

const MatterFactory* get() {
    if (state != State::Unknown) {
        return data;
    }

    state = State::None;

    const auto& layout = ::espurna::beken::flashLayout();
    if ((layout.layout != ::espurna::beken::Layout::Tuya) || layout.matter_offset) {
        return nullptr;
    }

    // `matterTuya 0` keeps the board on its own identity even with Tuya's factory data on the chip
    if (!getSetting("matterTuya", true)) {
        return nullptr;
    }

    auto* out = new MatterFactory;
    if (!::espurna::beken::tuya_kv::load_matter_factory(flash_read, layout.flash_size, *out)) {
        delete out;
        return nullptr;
    }

    data = out;
    state = State::Found;
    DEBUG_MSG_P(PSTR("[MATTER] Factory identity in the Tuya key-value store at 0x%06X, VID 0x%04X PID 0x%04X\n"),
        out->offset, out->vendor_id, out->product_id);

    return data;
}

// Device type of the certification declaration (tag 3 of its TLV), 0 when there is none. The MOES MS-104B Matter
// board is certified as an On/Off Light (0x0100); a controller that knows the product by its VID/PID (the speaker hub)
// does not finish adding it while the endpoints say On/Off Plug-in Unit
uint16_t declared_device_type() {
    const auto* factory = get();
    if (!factory) {
        return 0;
    }

    const auto& cd = factory->declaration;

    // CMS SignedData: eContent of id-data (1.2.840.113549.1.7.1) is [0] { OCTET STRING { TLV } }
    constexpr uint8_t IdData[] { 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x01 };
    const auto it = std::search(cd.begin(), cd.end(), std::begin(IdData), std::end(IdData));
    if (it == cd.end()) {
        return 0;
    }

    size_t pos = (it - cd.begin()) + sizeof(IdData);
    const auto der_length = [&](size_t& at, size_t& out) -> bool {
        if (at >= cd.size()) {
            return false;
        }
        uint8_t first = cd[at++];
        if (first < 0x80) {
            out = first;
            return true;
        }
        const size_t bytes = first & 0x7f;
        if (!bytes || (bytes > 3) || ((at + bytes) > cd.size())) {
            return false;
        }
        out = 0;
        for (size_t index = 0; index < bytes; ++index) {
            out = (out << 8) | cd[at++];
        }
        return true;
    };

    size_t length;
    if ((pos >= cd.size()) || (cd[pos++] != 0xa0) || !der_length(pos, length)) {
        return 0;
    }
    if ((pos >= cd.size()) || (cd[pos++] != 0x04) || !der_length(pos, length) || ((pos + length) > cd.size())) {
        return 0;
    }

    // Matter TLV: anonymous structure, context tags, unsigned integers 0x04..0x07, strings 0x0c..0x13
    const size_t end = pos + length;
    if ((pos >= end) || (cd[pos++] != 0x15)) {
        return 0;
    }

    int depth = 0;
    while (pos < end) {
        const uint8_t control = cd[pos++];
        if (control == 0x18) {
            if (!depth) {
                break;
            }
            --depth;
            continue;
        }

        const uint8_t type = control & 0x1f;
        const uint8_t tag_form = control >> 5;
        int tag = -1;
        if (tag_form == 1) {
            if (pos >= end) {
                return 0;
            }
            tag = cd[pos++];
        } else if (tag_form != 0) {
            return 0;
        }

        if ((type >= 0x15) && (type <= 0x17)) {
            ++depth;
            continue;
        }

        if (type <= 0x07) {
            const size_t bytes = size_t(1) << (type & 0x03);
            if ((pos + bytes) > end) {
                return 0;
            }
            uint32_t value = 0;
            for (size_t index = 0; (index < bytes) && (index < 4); ++index) {
                value |= uint32_t(cd[pos + index]) << (8 * index);
            }
            pos += bytes;
            if (!depth && (tag == 3)) {
                return (value <= 0xffff) ? static_cast<uint16_t>(value) : 0;
            }
            continue;
        }

        if ((type == 0x08) || (type == 0x09) || (type == 0x14)) {
            continue;
        }

        if ((type >= 0x0c) && (type <= 0x13)) {
            const size_t bytes = size_t(1) << (type & 0x03);
            if ((pos + bytes) > end) {
                return 0;
            }
            size_t size = 0;
            for (size_t index = 0; (index < bytes) && (index < 4); ++index) {
                size |= size_t(cd[pos + index]) << (8 * index);
            }
            pos += bytes + size;
            continue;
        }

        return 0;
    }

    return 0;
}

bool read(const char* name, uint8_t* out, size_t size, size_t& length) {
    const auto* factory = get();
    if (!factory) {
        return false;
    }

    struct Bytes {
        const char* name;
        const void* data;
        size_t length;
    };

    const Bytes values[] {
        {"dac-cert", factory->dac.data(), factory->dac.size()},
        {"pai-cert", factory->pai.data(), factory->pai.size()},
        {"cert-dclrn", factory->declaration.data(), factory->declaration.size()},
        {"dac-pub-key", factory->dac_public.data(), factory->dac_public.size()},
        {"dac-key", factory->dac_private.data(), factory->dac_private.size()},
        {"salt", factory->salt.data(), factory->salt.size()},
        {"verifier", factory->verifier.data(), factory->verifier.size()},
        {"vendor-name", factory->vendor_name.data(), factory->vendor_name.size()},
        {"product-name", factory->product_name.data(), factory->product_name.size()},
        {"serial-num", factory->serial.data(), factory->serial.size()},
    };

    for (const auto& value : values) {
        if ((std::strcmp(name, value.name) == 0) && value.length) {
            return copy(value.data, value.length, out, size, length);
        }
    }

    struct Number {
        const char* name;
        uint32_t value;
        size_t bytes;
    };

    const Number numbers[] {
        {"discriminator", factory->discriminator, 2},
        {"vendor-id", factory->vendor_id, 2},
        {"product-id", factory->product_id, 2},
        {"hardware-ver", factory->hardware_version, 2},
        {"iteration-count", factory->iterations, 4},
        {"pin-code", factory->passcode, 4},
    };

    for (const auto& number : numbers) {
        if (std::strcmp(name, number.name) == 0) {
            return copy_number(number.value, number.bytes, out, size, length);
        }
    }

    return false;
}

} // namespace tuya

bool quectel_read(const char* name, uint8_t* out, size_t size, size_t& length);

// Value of a key, or false when there is none. Copies at most size bytes, length is the full one
bool read(const char* name, uint8_t* out, size_t size, size_t& length) {
    const auto& layout = ::espurna::beken::flashLayout();
    if (!layout.matter_offset) {
        // no factory data partition in this flash layout (Tuya 2 MiB boards): Tuya's Matter firmware may have left
        // the identity in its own store
        return tuya::read(name, out, size, length);
    }

    return quectel_read(name, out, size, length);
}

// Where the identity of the board comes from, for MATTER.FACTORY and the MATTER page. No secrets. Empty: no factory
// DAC, the library makes and uses its own development identity
String source() {
    size_t length { 0 };
    if (!read("dac-cert", nullptr, 0, length)) {
        return String();
    }

    const auto& layout = ::espurna::beken::flashLayout();
    String out;
    if (layout.matter_offset) {
        out = F("Quectel factory table at 0x");
        out += String(layout.matter_offset, 16);
    } else {
        out = F("Tuya key-value store at 0x");
        out += String(tuya::data->offset, 16);
    }

    return out;
}

bool quectel_read(const char* name, uint8_t* out, size_t size, size_t& length) {
    const auto& layout = ::espurna::beken::flashLayout();
    const uint32_t TableOffset { layout.matter_offset + TableStart };
    const uint32_t DataOffset { layout.matter_offset + DataStart };

    for (uint32_t offset = 0; offset < TableSize; offset += sizeof(Entry)) {
        Entry entry;
        if (lt_flash_read(TableOffset + offset, reinterpret_cast<uint8_t*>(&entry), sizeof(entry)) != sizeof(entry)) {
            return false;
        }

        if (entry.magic != EntryMagic) {
            continue;
        }

        if (std::strncmp(entry.name, name, sizeof(entry.name)) != 0) {
            continue;
        }

        if ((entry.offset + entry.length) > DataSize) {
            return false;
        }

        length = entry.length;

        const auto copy = std::min(size, static_cast<size_t>(entry.length));
        return !copy || (lt_flash_read(DataOffset + entry.offset, out, copy) == copy);
    }

    return false;
}

bool read(const char* name, std::vector<uint8_t>& out) {
    size_t length { 0 };
    if (!read(name, nullptr, 0, length)) {
        return false;
    }

    out.resize(length);
    return read(name, out.data(), out.size(), length);
}

uint32_t read_number(const char* name) {
    uint8_t value[4] {};
    size_t length { 0 };
    if (!read(name, value, sizeof(value), length) || (length > sizeof(value))) {
        return 0;
    }

    uint32_t out { 0 };
    for (size_t index = length; index > 0; --index) {
        out = (out << 8) | value[index - 1];
    }

    return out;
}

String read_text(const char* name) {
    std::vector<uint8_t> value;
    if (!read(name, value)) {
        return String();
    }

    String out;
    out.concat(reinterpret_cast<const char*>(value.data()), value.size());

    return out;
}

} // namespace factory

// ---------------------------------------------------------------------------------------------------------------------

namespace kv {

// 'kvs' partition of the board (0x1D3000, 32 KiB), empty on the boards with Matter factory data
struct fdb_kvdb db;
SemaphoreHandle_t mutex { nullptr };
bool ready { false };

void lock(fdb_db_t) {
    xSemaphoreTake(mutex, portMAX_DELAY);
}

void unlock(fdb_db_t) {
    xSemaphoreGive(mutex);
}

bool init() {
    if (ready) {
        return true;
    }

    mutex = xSemaphoreCreateMutex();
    fdb_kvdb_control(&db, FDB_KVDB_CTRL_SET_LOCK, reinterpret_cast<void*>(lock));
    fdb_kvdb_control(&db, FDB_KVDB_CTRL_SET_UNLOCK, reinterpret_cast<void*>(unlock));

    ready = fdb_kvdb_init(&db, "matter", "kvs", nullptr, nullptr) == FDB_NO_ERR;
    return ready;
}

String key(const char* ns, const char* name) {
    String out;
    out.reserve(std::strlen(ns) + std::strlen(name) + 1);
    out += ns;
    out += '/';
    out += name;

    return out;
}

int read(const char* ns, const char* name, char* buf, uint32_t len, uint32_t* out_len) {
    if (!init()) {
        return BkFail;
    }

    struct fdb_kv obj;
    const auto full = key(ns, name);
    if (!fdb_kv_get_obj(&db, full.c_str(), &obj)) {
        return BkFail;
    }

    struct fdb_blob blob;
    const auto copy = std::min(len, static_cast<uint32_t>(obj.value_len));
    fdb_blob_read(reinterpret_cast<fdb_db_t>(&db), fdb_kv_to_blob(&obj, fdb_blob_make(&blob, buf, copy)));

    if (out_len) {
        *out_len = copy;
    }

    return BkOk;
}

int write(const char* ns, const char* name, const char* buf, uint32_t len) {
    if (!init()) {
        return BkFail;
    }

    struct fdb_blob blob;
    const auto full = key(ns, name);
    return (fdb_kv_set_blob(&db, full.c_str(), fdb_blob_make(&blob, buf, len)) == FDB_NO_ERR) ? BkOk : BkFail;
}

int remove(const char* ns, const char* name) {
    if (!init()) {
        return BkFail;
    }

    const auto full = key(ns, name);
    return (fdb_kv_del(&db, full.c_str()) == FDB_NO_ERR) ? BkOk : BkFail;
}

bool exists(const char* ns, const char* name) {
    if (!init()) {
        return false;
    }

    struct fdb_kv obj;
    const auto full = key(ns, name);
    return fdb_kv_get_obj(&db, full.c_str(), &obj) != nullptr;
}

int clear(const char* ns) {
    if (!init()) {
        return BkFail;
    }

    String prefix(ns);
    prefix += '/';

    std::vector<String> names;

    struct fdb_kv_iterator iterator;
    fdb_kv_iterator_init(&iterator);
    while (fdb_kv_iterate(&db, &iterator)) {
        if (std::strncmp(iterator.curr_kv.name, prefix.c_str(), prefix.length()) == 0) {
            names.emplace_back(iterator.curr_kv.name);
        }
    }

    for (const auto& name : names) {
        fdb_kv_del(&db, name.c_str());
    }

    return BkOk;
}

// Matter factory reset: fabrics, ACL, counters... everything but the own identity of the board (namespace
// "espurna-id" of matter_app.cpp), so the pairing code stays the same
constexpr char IdentityPrefix[] = "espurna-id/";

int clear_all() {
    if (!init()) {
        return BkFail;
    }

    std::vector<String> names;

    struct fdb_kv_iterator iterator;
    fdb_kv_iterator_init(&iterator);
    while (fdb_kv_iterate(&db, &iterator)) {
        if (std::strncmp(iterator.curr_kv.name, IdentityPrefix, sizeof(IdentityPrefix) - 1) != 0) {
            names.emplace_back(iterator.curr_kv.name);
        }
    }

    for (const auto& name : names) {
        fdb_kv_del(&db, name.c_str());
    }

    return BkOk;
}

} // namespace kv

// ---------------------------------------------------------------------------------------------------------------------

namespace events {

struct Callback {
    event_module_t module;
    event_cb_t callback;
    void* arg;
};

std::vector<Callback> callbacks;
bool connected { false };

void notify(event_module_t module, int id, void* data) {
    for (const auto& callback : callbacks) {
        if (callback.module == module) {
            callback.callback(callback.arg, module, id, data);
        }
    }
}

void add(event_module_t module, event_cb_t callback, void* arg) {
    for (auto& entry : callbacks) {
        if ((entry.module == module) && (entry.callback == callback)) {
            entry.arg = arg;
            return;
        }
    }

    callbacks.push_back(Callback{module, callback, arg});

    // Matter starts after ESPurna is already on the network
    if (connected) {
        callback(arg, module,
            (module == EVENT_MOD_NETIF) ? EventNetifGotIp4 : EventWifiStaConnected,
            nullptr);
    }
}

void remove(event_module_t module, event_cb_t callback) {
    for (auto it = callbacks.begin(); it != callbacks.end(); ++it) {
        if (((*it).module == module) && ((*it).callback == callback)) {
            callbacks.erase(it);
            return;
        }
    }
}

} // namespace events

// ---------------------------------------------------------------------------------------------------------------------

namespace trng {

constexpr uintptr_t Ctrl { 0x00802480 };
constexpr uintptr_t Data { 0x00802484 };
constexpr uint32_t Enable { 1 };

void fill(unsigned char* out, size_t length) {
    auto* ctrl = reinterpret_cast<volatile uint32_t*>(Ctrl);
    const auto* data = reinterpret_cast<const volatile uint32_t*>(Data);

    *ctrl = *ctrl | Enable;

    while (length) {
        uint32_t word { 0 };
        for (size_t index = 0; index < 4; ++index) {
            delayMicroseconds(20);
            word = (word << 7) ^ (word >> 25) ^ *data;
        }

        const auto copy = std::min(length, sizeof(word));
        std::memcpy(out, &word, copy);
        out += copy;
        length -= copy;
    }

    *ctrl = *ctrl & ~Enable;
}

} // namespace trng

// ---------------------------------------------------------------------------------------------------------------------
// Matter code in its own flash region, the 'tail' (scripts/beken_matter_tail.py). On 2 MiB boards it lives in the
// 'download' partition: the bootloader installs the rest of the image (the head) from there as usual, the tail is
// written afterwards from the web OTA form. Nothing in the tail may run before it is checked: no Matter call, not
// even its static constructors, until the build id and the CRC of the tail match this image.

#if MATTER_TAIL_SUPPORT
} // namespace
} // namespace matter
} // namespace espurna

extern "C" {
typedef void (*matter_tail_init_t)(void);
extern const uint8_t __matter_tail_start[];
extern const uint8_t __matter_tail_end[];
extern matter_tail_init_t __matter_tail_init_start[];
extern matter_tail_init_t __matter_tail_init_end[];
extern const uint8_t __build_id_start[];
} // extern "C"

namespace espurna {
namespace matter {
namespace {
namespace tail {

constexpr uint32_t Magic { 0x314C544D }; // "MTL1"
constexpr size_t BuildIdSize { 20 };

// Filled in by scripts/beken_link_tail.py, all zeroes here
struct Header {
    uint32_t magic;
    uint32_t size;
    uint32_t crc;
    uint32_t reserved;
    uint8_t build_id[BuildIdSize];
};

__attribute__((section(".matter_tail_header"), used))
const Header header {};

enum class State {
    Unchecked,
    Missing,
    OtherBuild,
    Corrupt,
    Ready,
};

State state { State::Unchecked };
bool constructed { false };

// GNU note of the link: namesz 4, descsz 20, type 3, "GNU\0", then the id
const uint8_t* build_id() {
    return __build_id_start + 16;
}

uint32_t crc32(uint32_t crc, const volatile uint8_t* data, size_t length) {
    static constexpr uint32_t Table[16] {
        0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
        0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C,
    };

    crc = ~crc;
    for (size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        crc = (crc >> 4) ^ Table[crc & 0xf];
        crc = (crc >> 4) ^ Table[crc & 0xf];
    }

    return ~crc;
}

uint32_t flash_offset(const uint8_t* address) {
    return reinterpret_cast<uintptr_t>(address) * 34 / 32;
}

uint32_t flash_size() {
    return flash_offset(__matter_tail_end) - flash_offset(__matter_tail_start);
}

// The region is a part of the 'download' partition of this board's bootloader (Tuya 2 MiB layout): an OTA image there
// overwrites the tail, and the tail is written separately. On the Quectel 4 MiB layout it is inside the application,
// the bootloader installs it with the rest
bool shares_download() {
    const auto& layout = ::espurna::beken::flashLayout();
    const auto start = flash_offset(__matter_tail_start);
    return layout.download_length
        && (start >= layout.download_offset)
        && (start < (layout.download_offset + layout.download_length));
}

// Matter code written by ESPurna, not by the bootloader
bool writable() {
    return ::espurna::beken::flashLayout().layout == ::espurna::beken::Layout::Tuya;
}

// CRC16 of every 32 bytes in the flash (CRC-16/CMS, big endian after the data), as the bootloader and the
// packager write it
uint16_t crc16(const uint8_t* data, size_t length) {
    uint16_t crc { 0xffff };
    for (size_t index = 0; index < length; ++index) {
        crc ^= static_cast<uint16_t>(data[index]) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000)
                ? static_cast<uint16_t>((crc << 1) ^ 0x8005)
                : static_cast<uint16_t>(crc << 1);
        }
    }

    return crc;
}

constexpr size_t BlockSize { 34 };

// Raw reads, block by block. Reading a block with a wrong CRC through the flash mapping stalls the chip until the
// watchdog resets it (seen on the CB3S with an OTA image or erased flash in the region), so the mapping is only
// used once every block of the region checks out
bool blocks_valid(uint32_t offset, uint32_t size) {
    uint8_t buffer[BlockSize * 16];
    for (uint32_t done = 0; done < size; done += sizeof(buffer)) {
        const auto chunk = std::min(static_cast<uint32_t>(sizeof(buffer)), size - done);
        if (lt_flash_read(offset + done, buffer, chunk) != chunk) {
            return false;
        }

        for (size_t block = 0; (block + BlockSize) <= chunk; block += BlockSize) {
            const uint16_t stored = (static_cast<uint16_t>(buffer[block + 32]) << 8) | buffer[block + 33];
            if (crc16(&buffer[block], 32) != stored) {
                return false;
            }
        }
    }

    return true;
}

// Through the flash mapping, like the code will be read. Volatile, the compiler knows the header as all zeroes
State check() {
    const auto start = flash_offset(__matter_tail_start);
    if (!blocks_valid(start, BlockSize)) {
        return State::Missing;
    }

    if (!blocks_valid(start, flash_size())) {
        return State::Corrupt;
    }

    const auto* ptr = reinterpret_cast<const volatile Header*>(__matter_tail_start);
    if (ptr->magic != Magic) {
        return State::Missing;
    }

    for (size_t index = 0; index < BuildIdSize; ++index) {
        if (ptr->build_id[index] != build_id()[index]) {
            return State::OtherBuild;
        }
    }

    const size_t size = __matter_tail_end - __matter_tail_start;
    if (ptr->size != size) {
        return State::Corrupt;
    }

    const auto* body = reinterpret_cast<const volatile uint8_t*>(__matter_tail_start) + sizeof(Header);
    if (crc32(0, body, size - sizeof(Header)) != ptr->crc) {
        return State::Corrupt;
    }

    return State::Ready;
}

const char* name(State value) {
    switch (value) {
    case State::Unchecked:
        return "not checked";
    case State::Missing:
        return "missing";
    case State::OtherBuild:
        return "from another build";
    case State::Corrupt:
        return "corrupt";
    case State::Ready:
        return "ready";
    }

    return "?";
}

// Once, before the first Matter call
bool ready() {
    if (state != State::Ready) {
        state = check();
        if (state != State::Ready) {
            DEBUG_MSG_P(PSTR("[MATTER] Matter code (flash 0x%06X) is %s, upload matter-tail.bin of this build\n"),
                flash_offset(__matter_tail_start), name(state));
            return false;
        }
    }

    if (!constructed) {
        constructed = true;
        for (auto* init = __matter_tail_init_start; init != __matter_tail_init_end; ++init) {
            (*init)();
        }
    }

    return true;
}

String status() {
    if (state == State::Unchecked) {
        state = check();
    }

    char id[9];
    snprintf_P(id, sizeof(id), PSTR("%02x%02x%02x%02x"),
        build_id()[0], build_id()[1], build_id()[2], build_id()[3]);

    String out;
    out.reserve(128);
    out += F("flash 0x");
    out += String(flash_offset(__matter_tail_start), 16);
    out += F(", ");
    out += String(flash_size(), 10);
    out += F(" bytes, ");
    out += name(state);
    out += F(", build ");
    out += id;
    if (shares_download()) {
        out += F(", in the OTA partition");
    }

    return out;
}

// matter-tail.bin: this header, then the tail as it is in the flash (encrypted, CRC'd)
struct Container {
    char magic[8];
    uint32_t version;
    uint32_t offset;
    uint32_t length;
    uint32_t crc;
    uint8_t build_id[BuildIdSize];
};

constexpr char ContainerMagic[] = "ESPMTAIL";
constexpr size_t ContainerSize { 64 };
constexpr size_t SectorSize { 0x1000 };

struct Upload {
    uint8_t header[ContainerSize];
    size_t header_size { 0 };
    Container container {};
    size_t written { 0 };
    uint32_t erased { 0 };
    uint32_t crc { 0 };
    bool active { false };
};

Upload upload;

bool is_container(const uint8_t* data, size_t length) {
    return (length >= (sizeof(ContainerMagic) - 1))
        && (std::memcmp(data, ContainerMagic, sizeof(ContainerMagic) - 1) == 0);
}

const char* upload_begin(bool matter_started) {
    if (matter_started) {
        return PSTR("ERROR: Matter is running, upload after a reboot");
    }

    // the tail is encrypted for the Tuya key and the Tuya layout; other boards get it with the firmware
    if (!writable()) {
        return PSTR("ERROR: matter-tail.bin is not needed on this board, the firmware has the Matter code");
    }

    upload = Upload{};
    upload.active = true;
    return nullptr;
}

const char* upload_header() {
    std::memcpy(&upload.container, upload.header, sizeof(Container));
    const auto& container = upload.container;
    if (container.version != 1) {
        return PSTR("ERROR: Unknown matter-tail.bin version");
    }

    if ((container.offset != flash_offset(__matter_tail_start)) || (container.length > flash_size())) {
        return PSTR("ERROR: matter-tail.bin is for another flash layout");
    }

    if (std::memcmp(container.build_id, build_id(), BuildIdSize) != 0) {
        return PSTR("ERROR: matter-tail.bin is from another build, upload the firmware of the same build first");
    }

    // An OTA image left in the 'download' partition is overwritten from here on. With its header erased the
    // bootloader sees no update to install, like after LibreTiny reverts an OTA
    if (shares_download() && !lt_flash_erase_block(::espurna::beken::flashLayout().download_offset)) {
        return PSTR("ERROR: Flash erase failed");
    }

    upload.erased = container.offset;
    return nullptr;
}

const char* upload_write(const uint8_t* data, size_t length) {
    if (!upload.active) {
        return PSTR("ERROR: No upload");
    }

    while (length && (upload.header_size < ContainerSize)) {
        const auto chunk = std::min(length, ContainerSize - upload.header_size);
        std::memcpy(&upload.header[upload.header_size], data, chunk);
        upload.header_size += chunk;
        data += chunk;
        length -= chunk;
        if (upload.header_size == ContainerSize) {
            const auto* error = upload_header();
            if (error) {
                upload.active = false;
                return error;
            }
        }
    }

    if (!length) {
        return nullptr;
    }

    if ((upload.written + length) > upload.container.length) {
        upload.active = false;
        return PSTR("ERROR: matter-tail.bin is longer than its header says");
    }

    const uint32_t offset = upload.container.offset + upload.written;
    while (upload.erased < (offset + length)) {
        if (!lt_flash_erase_block(upload.erased)) {
            upload.active = false;
            return PSTR("ERROR: Flash erase failed");
        }
        upload.erased += SectorSize;
    }

    if (lt_flash_write(offset, data, length) != length) {
        upload.active = false;
        return PSTR("ERROR: Flash write failed");
    }

    upload.crc = crc32(upload.crc, data, length);
    upload.written += length;
    return nullptr;
}

const char* upload_end() {
    if (!upload.active) {
        return PSTR("ERROR: No upload");
    }

    upload.active = false;
    if ((upload.header_size != ContainerSize) || (upload.written != upload.container.length)) {
        return PSTR("ERROR: matter-tail.bin is incomplete");
    }

    if (upload.crc != upload.container.crc) {
        return PSTR("ERROR: matter-tail.bin CRC mismatch");
    }

    state = check();
    if (state != State::Ready) {
        return PSTR("ERROR: Matter code does not check out after writing");
    }

    DEBUG_MSG_P(PSTR("[MATTER] Matter code written, %u bytes\n"), upload.written);
    return nullptr;
}

void upload_abort() {
    upload.active = false;
}

// nullptr when the Matter code is in place, what is wrong with it otherwise
const char* problem() {
    if (state == State::Unchecked) {
        state = check();
    }

    return (state == State::Ready) ? nullptr : name(state);
}

} // namespace tail
#endif

// ---------------------------------------------------------------------------------------------------------------------

#if defined(MATTER_UDP_LOG)
// Bring-up aid for boards without a serial wire, off unless built with -DMATTER_UDP_LOG="a.b.c.d": every Matter log
// line and start step as one UDP packet
// to MATTER_UDP_LOG (a.b.c.d), port 5514, sent from the tcpip thread, so it works while the loop is stuck
namespace udplog {

struct Message {
    size_t length;
    char text[160];
};

udp_pcb* pcb { nullptr };

void send_tcpip(void* arg) {
    auto* message = static_cast<Message*>(arg);
    if (!pcb) {
        pcb = udp_new();
    }

    if (pcb) {
        auto* buffer = pbuf_alloc(PBUF_TRANSPORT, message->length, PBUF_RAM);
        if (buffer) {
            std::memcpy(buffer->payload, message->text, message->length);
            ip_addr_t address;
            ipaddr_aton(MATTER_UDP_LOG, &address);
            udp_sendto(pcb, buffer, &address, 5514);
            pbuf_free(buffer);
        }
    }

    free(message);
}

void send(const char* text) {
    auto* message = static_cast<Message*>(malloc(sizeof(Message)));
    if (!message) {
        return;
    }

    message->length = snprintf(message->text, sizeof(message->text), "%u %u %s",
        static_cast<unsigned>(millis()), static_cast<unsigned>(lt_heap_get_free()), text);
    message->length = std::min(message->length, sizeof(message->text) - 1);
    if (tcpip_callback(send_tcpip, message) != ERR_OK) {
        free(message);
    }
}

} // namespace udplog
#define MATTER_STEP(X) udplog::send(X)
#else
#define MATTER_STEP(X)
#endif

String node_label;
uint8_t plug_count { 1 };

// device type of the relay endpoints from the factory certification declaration, 0 = the library's own (plug)
uint16_t device_type { 0 };

// names of the bridged plugs, filled by backend::start() in the loop for the Matter task (matter_plug_name())
struct PlugName {
    char label[33];
    char unique[33];
};

PlugName plug_names[4];

// Commissioning in progress, from the Matter task: the message trace is only logged then
std::atomic<bool> commissioning_active { false };

// The lwIP port of the BDK brings the netif up without NETIF_FLAG_IGMP (MLD6 is set), joining an IPv4 multicast
// group then fails with ERR_VAL, and the minimal mDNS of Matter cannot answer on 224.0.0.251. Runs in the tcpip thread
void enable_igmp(void*) {
    for (auto* netif = netif_list; netif; netif = netif->next) {
        if ((netif->flags & NETIF_FLAG_ETHARP) && !(netif->flags & NETIF_FLAG_IGMP)) {
            netif->flags |= NETIF_FLAG_IGMP;
            igmp_start(netif);
        }
    }
}

#if TERMINAL_SUPPORT
PROGMEM_STRING(Factory, "MATTER.FACTORY");

// Public part only, no keys, passcode or verifier
void command_factory(::terminal::CommandContext&& ctx) {
    std::vector<uint8_t> dac;
    std::vector<uint8_t> pai;
    std::vector<uint8_t> cd;
    factory::read("dac-cert", dac);
    factory::read("pai-cert", pai);
    factory::read("cert-dclrn", cd);

    const auto source = factory::source();
    if (!source.length()) {
        ctx.output.printf_P(PSTR("identity: own (development DAC made on the board), no factory data\n"));
        terminalOK(ctx);
        return;
    }

    ctx.output.printf_P(PSTR("identity: factory, %s\n"), source.c_str());
    ctx.output.printf_P(PSTR("vendor: %s (0x%04X), product: %s (0x%04X)\n"),
        factory::read_text("vendor-name").c_str(), factory::read_number("vendor-id"),
        factory::read_text("product-name").c_str(), factory::read_number("product-id"));
    ctx.output.printf_P(PSTR("certificates: DAC %u, PAI %u, declaration %u bytes\n"),
        dac.size(), pai.size(), cd.size());

    terminalOK(ctx);
}

PROGMEM_STRING(Reset, "MATTER.RESET");

// Forget every controller (fabrics, ACL), keep the own identity and its pairing code. Not while Matter runs: it
// holds all of that in memory and writes it back
void command_reset(::terminal::CommandContext&& ctx) {
    if (matterStarted()) {
        terminalError(ctx, F("Matter is running, reboot first (matterAuto 0 keeps it off)"));
        return;
    }

    if (kv::clear_all() != BkOk) {
        terminalError(ctx, F("Key-value store failed"));
        return;
    }

    terminalOK(ctx);
    prepareReset(CustomResetReason::Terminal);
}

#if MATTER_TAIL_SUPPORT
PROGMEM_STRING(Tail, "MATTER.TAIL");

void command_tail(::terminal::CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("matter code: %s\n"), tail::status().c_str());
    terminalOK(ctx);
}
#endif

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Factory, command_factory},
    {Reset, command_reset},
#if MATTER_TAIL_SUPPORT
    {Tail, command_tail},
#endif
};
#endif

} // namespace

// ---------------------------------------------------------------------------------------------------------------------

namespace backend {

// A plug endpoint per relay, as many as the data model of libMatterApp has. Asked in start(): the library may live in
// the Matter tail, not callable before it was checked
namespace {
size_t library_plugs { 1 };
} // namespace

size_t endpoints() {
    return std::min(relayCount(), library_plugs);
}

void setup() {
#if TERMINAL_SUPPORT
    terminal::add(Commands);
#endif
}

bool start() {
    MATTER_STEP("start");
#if MATTER_TAIL_SUPPORT
    if (!tail::ready()) {
        return false;
    }
    MATTER_STEP("tail ready, constructors done");
#endif

    if (!kv::init()) {
        DEBUG_MSG_P(PSTR("[MATTER] Key-value store failed\n"));
        return false;
    }

    // Tuya factory identity decrypted here in the loop, the Matter task then only reads the copy
    factory::tuya::get();
    device_type = factory::tuya::declared_device_type();
    if (device_type) {
        DEBUG_MSG_P(PSTR("[MATTER] Device type 0x%04X of the certification declaration\n"), device_type);
    }

    node_label = systemHostname();
    library_plugs = matter_app_plugs ? matter_app_plugs() : 1;
    plug_count = endpoints();
    for (size_t index = 0; (index < plug_count) && (index < (sizeof(plug_names) / sizeof(plug_names[0]))); ++index) {
        plug_name(index, plug_names[index].label, sizeof(plug_names[index].label),
            plug_names[index].unique, sizeof(plug_names[index].unique));
    }
    MATTER_STEP("kv ready, matter_app_start");
    const auto out = matter_app_start() == 0;
    MATTER_STEP(out ? "matter_app_start OK" : "matter_app_start FAILED");
    return out;
}

void network(bool up) {
    if (up) {
        events::connected = true;
        tcpip_callback(enable_igmp, nullptr);
        events::notify(EVENT_MOD_WIFI, EventWifiStaConnected, nullptr);
        events::notify(EVENT_MOD_NETIF, EventNetifGotIp4, nullptr);
        return;
    }

    if (events::connected) {
        events::connected = false;
        matter_sta_disconnected_t data {};
        events::notify(EVENT_MOD_WIFI, EventWifiStaDisconnected, &data);
    }
}

void set_onoff(size_t index, bool on) {
    matter_set_onoff(static_cast<uint8_t>(index + 1), on);
}

bool codes(char* manual, size_t manual_size, char* qr, size_t qr_size) {
    return matter_onboarding_codes(manual, manual_size, qr, qr_size);
}

int fabrics() {
    return matter_fabric_count();
}

// only called while Matter runs (matter.cpp): the library may live in the Matter tail
size_t controllers(Controller* out, size_t size) {
    if (!matter_fabric_info) {
        return 0;
    }

    size_t count { 0 };
    while (count < size) {
        auto& entry = out[count];
        if (!matter_fabric_info(count, &entry.index, &entry.vendor, &entry.node, entry.label, sizeof(entry.label))) {
            break;
        }
        ++count;
    }

    return count;
}

bool remove_controller(uint8_t index) {
    if (!matter_remove_fabric) {
        return false;
    }

    const auto out = matter_remove_fabric(index);
    if (out) {
        DEBUG_MSG_P(PSTR("[MATTER] Removing controller #%u\n"), index);
    }

    return out;
}

void open_window() {
    matter_open_window();
}

void ping() {
    matter_ping();
}

String product() {
    const auto source = factory::source();
    if (!source.length()) {
        return F("ESPurna (0xfff1) / ESPurna plug (0x8000), own identity (development DAC)");
    }

    String out;
    out.reserve(112);
    out += factory::read_text("vendor-name");
    out += F(" (0x");
    out += String(factory::read_number("vendor-id"), 16);
    out += F(") / ");
    out += factory::read_text("product-name");
    out += F(" (0x");
    out += String(factory::read_number("product-id"), 16);
    out += F("), factory identity: ");
    out += source;

    return out;
}

size_t heap() {
    return lt_heap_get_free();
}

size_t heap_min() {
    return lt_heap_get_min_free();
}

} // namespace backend
} // namespace matter
} // namespace espurna

// ---------------------------------------------------------------------------------------------------------------------
// Callbacks of matter_app.cpp and the bk_* platform API of the Matter library

extern "C" {

// Every message in and out is an "EM" progress line, a dozen per exchange: they flood the 8 slots and make
// the Matter task wait. Errors of every module pass
void matter_log(const char* line) {
    // web socket clients are paused while commissioning, the message trace only goes to the serial port then
    if ((line[0] == 'E') && (line[1] == 'M') && (line[2] == ' ') && !espurna::matter::commissioning_active) {
        return;
    }

    // free heap at the time of the line: commissioning runs out of it, this shows which step takes how much
    char tagged[120];
    snprintf(tagged, sizeof(tagged), "(%u) %s", static_cast<unsigned>(lt_heap_get_free()), line);
    espurna::matter::log_line(tagged);
#if defined(MATTER_UDP_LOG)
    espurna::matter::udplog::send(line);
#endif
}

// filled by backend::start(), in the loop: settings are not read from the Matter task
const char* matter_node_label(void) {
    return espurna::matter::node_label.c_str();
}

// plug endpoints in use (the relays), filled by backend::start() like the label
uint8_t matter_plug_count(void) {
    return espurna::matter::plug_count;
}

// device type of the relay endpoints, 0 = keep On/Off Plug-in Unit (filled by backend::start() like the label)
uint16_t matter_device_type(void) {
    return espurna::matter::device_type;
}

// plug = relay + 1
bool matter_plug_name(uint8_t plug, char* label, size_t label_size, char* unique, size_t unique_size) {
    using namespace espurna::matter;
    if ((plug < 1) || (plug > plug_count) || (plug > (sizeof(plug_names) / sizeof(plug_names[0]))) || !label_size || !unique_size) {
        return false;
    }

    const auto& name = plug_names[plug - 1];
    std::strncpy(label, name.label, label_size - 1);
    label[label_size - 1] = '\0';
    std::strncpy(unique, name.unique, unique_size - 1);
    unique[unique_size - 1] = '\0';

    return true;
}

// The certificate steps of a pairing (attestation, CSR, AddNOC: ECDSA ~2 s per sign / verify on this chip) keep the
// higher priority Matter task busy longer than the 10 s the loop has to feed the watchdog: a 4-relay bridge reset at
// the hub's step 3. 30 s while a controller commissions, the loop value (system_beken.cpp LoopWdtTimeout) after it
void matter_commissioning(bool active) {
    constexpr uint32_t LoopWdtMs { 10000 };
    constexpr uint32_t CommissioningWdtMs { 30000 };
    lt_wdt_enable(active ? CommissioningWdtMs : LoopWdtMs);

    espurna::matter::commissioning_active = active;
    espurna::matter::commissioning(active);
}

void matter_pong(void) {
    espurna::matter::pong();
}

void matter_window(bool open) {
    espurna::matter::window(open);
}

void matter_onoff_changed(uint8_t endpoint, bool on) {
    if (endpoint >= 1) {
        espurna::matter::onoff_changed(endpoint - 1, on);
    }
}

// "chip-factory": the keys of the stock factory table are read only, the ones Matter adds there itself (uniqueId)
// go to the key-value store like every other namespace
int bk_read_data(const char* ns, const char* name, char* buf, uint32_t len, uint32_t* out_len) {
    using namespace espurna::matter;
    if (std::strcmp(ns, FactoryNamespace) == 0) {
        size_t length { 0 };
        if (factory::read(name, reinterpret_cast<uint8_t*>(buf), len, length)) {
            if (out_len) {
                *out_len = std::min(static_cast<size_t>(len), length);
            }
            return BkOk;
        }

        // Basic Information SerialNumber: the stock factory data has none, Matter then reports the SDK test value.
        // the speaker hub reads it while commissioning. The chip id (MAC) is unique and stays across updates
        if (std::strcmp(name, "serial-num") == 0) {
            const auto serial = systemChipId();
            const auto copy = std::min(static_cast<size_t>(len), static_cast<size_t>(serial.length()));
            std::memcpy(buf, serial.data(), copy);
            if (out_len) {
                *out_len = copy;
            }
            return BkOk;
        }
    }

    return kv::read(ns, name, buf, len, out_len);
}

namespace {

bool factory_key(const char* ns, const char* name) {
    using namespace espurna::matter;
    size_t length { 0 };
    return (std::strcmp(ns, FactoryNamespace) == 0) && factory::read(name, nullptr, 0, length);
}

} // namespace

int bk_write_data(const char* ns, const char* name, const char* buf, uint32_t len) {
    using namespace espurna::matter;
    if (factory_key(ns, name)) {
        return BkFail;
    }

    return kv::write(ns, name, buf, len);
}

int bk_clean_data(const char* ns, const char* name) {
    using namespace espurna::matter;
    if (factory_key(ns, name)) {
        return BkFail;
    }

    return kv::remove(ns, name);
}

int bk_ensure_namespace(const char*) {
    return espurna::matter::BkOk;
}

int bk_ensure_name_data(const char* ns, const char* name) {
    using namespace espurna::matter;
    if (factory_key(ns, name)) {
        return BkOk;
    }

    return kv::exists(ns, name) ? BkOk : BkFail;
}

// Clears only what is in the key-value store, never the stock factory table
int bK_clear_namespace(const char* ns) {
    return espurna::matter::kv::clear(ns);
}

int bk_erase_all(int) {
    return espurna::matter::kv::clear_all();
}

bk_err_t bk_event_register_cb(event_module_t module, int, event_cb_t cb, void* arg) {
    espurna::matter::events::add(module, cb, arg);
    return espurna::matter::BkOk;
}

bk_err_t bk_event_unregister_cb(event_module_t module, int, event_cb_t cb) {
    espurna::matter::events::remove(module, cb);
    return espurna::matter::BkOk;
}

// Wi-Fi belongs to ESPurna, Matter never connects on its own (on-network commissioning only)
bk_err_t bk_wifi_sta_set_config(const wifi_sta_config_t*) {
    return espurna::matter::BkFail;
}

bk_err_t bk_wifi_sta_start(void) {
    return espurna::matter::BkFail;
}

bk_err_t bk_wifi_sta_connect(void) {
    return espurna::matter::BkFail;
}

bk_err_t bk_wifi_sta_get_link_status(wifi_link_status_t* status) {
    std::memset(status, 0, sizeof(*status));
    if (!espurna::matter::events::connected) {
        return espurna::matter::BkFail;
    }

    const auto ssid = WiFi.SSID();
    std::strncpy(status->ssid, ssid.c_str(), sizeof(status->ssid) - 1);

    const auto* bssid = WiFi.BSSID();
    if (bssid) {
        std::memcpy(status->bssid, bssid, sizeof(status->bssid));
    }

    status->channel = WiFi.channel();
    status->rssi = WiFi.RSSI();

    return espurna::matter::BkOk;
}

bk_err_t bk_wifi_sta_get_mac(uint8_t* mac) {
    WiFi.macAddress(mac);
    return espurna::matter::BkOk;
}

bk_err_t bk_wifi_scan_start(const wifi_scan_config_t*) {
    return espurna::matter::BkFail;
}

bk_err_t bk_wifi_scan_get_result(wifi_scan_result_t* result) {
    result->ap_num = 0;
    result->aps = nullptr;
    return espurna::matter::BkFail;
}

void bk_wifi_scan_free_result(wifi_scan_result_t*) {
}

// No OTA through Matter, ESPurna updates itself
int bk_write_ota_data_to_flash(char*, uint32_t, uint32_t) {
    return espurna::matter::BkFail;
}

int bk_read_ota_data_in_flash(char*, uint32_t, uint32_t) {
    return espurna::matter::BkFail;
}

void bk_erase_ota_data_in_flash(void) {
}

size_t prvHeapGetTotalSize(void) {
    return lt_heap_get_size();
}

int mbedtls_hardware_poll(void*, unsigned char* output, size_t len, size_t* olen) {
    espurna::matter::trng::fill(output, len);
    *olen = len;
    return 0;
}

} // extern "C"

#if MATTER_TAIL_SUPPORT
bool matterTailIsUpload(const uint8_t* data, size_t length) {
    return espurna::matter::tail::is_container(data, length);
}

// An OTA image in the 'download' partition would overwrite the code Matter runs
bool matterTailBlocksOta() {
    return matterStarted() && espurna::matter::tail::shares_download();
}

const char* matterTailBegin() {
    return espurna::matter::tail::upload_begin(matterStarted());
}

const char* matterTailWrite(const uint8_t* data, size_t length) {
    return espurna::matter::tail::upload_write(data, length);
}

const char* matterTailEnd() {
    return espurna::matter::tail::upload_end();
}

void matterTailAbort() {
    espurna::matter::tail::upload_abort();
}

const char* matterTailProblem() {
    return espurna::matter::tail::problem();
}
#endif

#endif
