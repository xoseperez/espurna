/*

MATTER BACKEND FOR BEKEN (LibreTiny)

Matter On/Off plug (connectedhomeip, prebuilt libMatterApp.a of matter-bk7231n) on top of ESPurna. ESPurna keeps
Wi-Fi, the relay and the web UI (matter.cpp); Matter gets the bk_* platform API it was written against (matter_pal.h
of the BK7231N port):
- "chip-factory" namespace: read only, straight from the factory data the stock firmware left in flash (Quectel SDK
  on the Sparkleiot XH-CB2S): DAC and its key, PAI, certification declaration, passcode verifier, discriminator.
  Nothing is ever written there
- every other namespace (chip-config, chip-counters, the KVS of fabrics, ACL ...): FlashDB on the 'kvs' partition
- Wi-Fi status and the connected / got IP events from the ESPurna wifi module
- entropy from the TRNG of the chip

*/

#include "espurna.h"

#if defined(LIBRETINY) && MATTER_SUPPORT

#include "espurna_matter.h"
#include "relay.h"
#include "terminal.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <flashdb.h>

#include <lwip/igmp.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>

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
void matter_set_onoff(uint8_t endpoint, bool on);
bool matter_onboarding_codes(char* manual, size_t manual_size, char* qr, size_t qr_size);
int matter_fabric_count(void);
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
constexpr uint32_t TableOffset { FLASH_MATTER_OFFSET + 0x7000 };
constexpr uint32_t TableSize { 0x1000 };
constexpr uint32_t DataOffset { FLASH_MATTER_OFFSET + 0x8000 };
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

// Value of a key, or false when there is none. Copies at most size bytes, length is the full one
bool read(const char* name, uint8_t* out, size_t size, size_t& length) {
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

int clear_all() {
    if (!init()) {
        return BkFail;
    }

    return (fdb_kv_set_default(&db) == FDB_NO_ERR) ? BkOk : BkFail;
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

String node_label;

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

    ctx.output.printf_P(PSTR("vendor: %s (0x%04X), product: %s (0x%04X)\n"),
        factory::read_text("vendor-name").c_str(), factory::read_number("vendor-id"),
        factory::read_text("product-name").c_str(), factory::read_number("product-id"));
    ctx.output.printf_P(PSTR("certificates: DAC %u, PAI %u, declaration %u bytes\n"),
        dac.size(), pai.size(), cd.size());

    terminalOK(ctx);
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Factory, command_factory},
};
#endif

} // namespace

// ---------------------------------------------------------------------------------------------------------------------

namespace backend {

// The data model of libMatterApp has a single plug endpoint
size_t endpoints() {
    return std::min(relayCount(), static_cast<size_t>(1));
}

void setup() {
#if TERMINAL_SUPPORT
    terminal::add(Commands);
#endif
}

bool start() {
    if (!kv::init()) {
        DEBUG_MSG_P(PSTR("[MATTER] Key-value store failed\n"));
        return false;
    }

    node_label = systemHostname();
    return matter_app_start() == 0;
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

void open_window() {
    matter_open_window();
}

void ping() {
    matter_ping();
}

String product() {
    String out;
    out.reserve(64);
    out += factory::read_text("vendor-name");
    out += F(" (0x");
    out += String(factory::read_number("vendor-id"), 16);
    out += F(") / ");
    out += factory::read_text("product-name");
    out += F(" (0x");
    out += String(factory::read_number("product-id"), 16);
    out += ')';

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
}

// filled by backend::start(), in the loop: settings are not read from the Matter task
const char* matter_node_label(void) {
    return espurna::matter::node_label.c_str();
}

void matter_commissioning(bool active) {
    espurna::matter::commissioning_active = active;
    espurna::matter::commissioning(active);
}

void matter_pong(void) {
    espurna::matter::pong();
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

#endif
