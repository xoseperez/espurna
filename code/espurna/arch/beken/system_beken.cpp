/*

SYSTEM MODULE FOR BEKEN (LibreTiny)

Chip specific part, common part is in system.cpp

*/

#if defined(LIBRETINY)

#include <Arduino.h>
#include <WiFi.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <forward_list>
#include <mutex>
#include <vector>

#include "espurna.h"
#include "rtcmem.h"
#include "storage_eeprom.h"

// LibreTiny main task runs setup() and loop(), remembered by the first pre_setup() call.
// Same name as the ESP32 Arduino core one, debug.cpp uses it to tell the loop task apart
TaskHandle_t loopTaskHandle { nullptr };

namespace {

// hardware WDT, fed by LibreTiny yield() after every loop()
constexpr uint32_t LoopWdtTimeout { 10000 };

} // namespace

// -----------------------------------------------------------------------------
// MEMORY, CHIP ID, RANDOM
// -----------------------------------------------------------------------------

size_t systemFreeHeap() {
    return lt_heap_get_free();
}

// Same as esp8266, cached on the first call (which is the first thing setup() does)
size_t systemInitialFreeHeap() {
    static const size_t value = lt_heap_get_free();
    return value;
}

HeapStats systemHeapStats() {
    const uint32_t free = lt_heap_get_free();

    // FreeRTOS v9 of the BK72xx SDK does not track the largest free block, LibreTiny reports 0.
    // Treat the whole free heap as usable instead of showing 100% fragmentation
    const uint32_t block = lt_heap_get_max_alloc();
    if (!block) {
        return {free, free, 0};
    }

    const uint8_t frag = free ? (100 - (block * 100 / free)) : 0;
    return {free, block, frag};
}

unsigned long systemFreeStack() {
    return uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t);
}

// No VCC measurement, ADC_MODE_VALUE is forced to ADC_TOUT (ref. compat_beken.h)
uint16_t systemVcc() {
    return 0;
}

// -----------------------------------------------------------------------------
// MAC
// -----------------------------------------------------------------------------

// libcal, reads the MAC out of the RF calibration block (LibreTiny 'calibration' partition)
extern "C" int manual_cal_get_macaddr_from_flash(uint8_t* mac);

namespace {
namespace mac {

// The SDK reads the MAC from the RF calibration block. Boards that have no such block (no Tuya config, e.g. the Sparkleiot
// XH-CB2S) all fall back to the same C8:47:8C:42:00:48, and two of them on one network get one IP. Such a board gets its
// own locally administered address once (CA:47:8C + 3 random bytes), in a block of its own. The SDK cannot add one, its
// manual_cal_write_macaddr_to_flash() only updates the MAC of a block that is already there
enum class Result {
    Stored,
    Generated,
    Replaced,
    Failed,
};

Result result { Result::Stored };
uint8_t generated[6] {};

// Same layout as a Tuya CB3S / CBU block: "TLV\0" and the length of the rest, then groups of items, each one a tag,
// a length and the data. MAC is the item 0x11111102 of the chip info group 0x11111100
struct __attribute__((packed)) Block {
    uint32_t magic;
    uint32_t length;
    uint32_t group;
    uint32_t group_length;
    uint32_t item;
    uint32_t item_length;
    uint8_t mac[6];
};

constexpr uint32_t BlockMagic { 0x00564c54 };
constexpr uint32_t ChipInfoGroup { 0x11111100 };
constexpr uint32_t MacItem { 0x11111102 };

// The first build took the random bytes from rand(), which nothing seeds, and every board got this one
constexpr uint8_t Unseeded[6] { 0xca, 0x47, 0x8c, 0x2d, 0xf4, 0x51 };

Block make_block(const uint8_t* mac) {
    Block out {
        .magic = BlockMagic,
        .length = sizeof(Block) - 8,
        .group = ChipInfoGroup,
        .group_length = sizeof(Block) - 16,
        .item = MacItem,
        .item_length = sizeof(Block::mac),
        .mac = {},
    };
    std::memcpy(out.mac, mac, sizeof(out.mac));

    return out;
}

// the sector holds nothing but this block (or nothing at all, without one)
bool sector_holds(const Block* block) {
    uint8_t chunk[256];
    for (uint32_t offset = 0; offset < FLASH_CALIBRATION_LENGTH; offset += sizeof(chunk)) {
        if (lt_flash_read(FLASH_CALIBRATION_OFFSET + offset, chunk, sizeof(chunk)) != sizeof(chunk)) {
            return false;
        }

        size_t from = 0;
        if (block && (offset == 0)) {
            if (std::memcmp(chunk, block, sizeof(Block)) != 0) {
                return false;
            }

            from = sizeof(Block);
        }

        if (std::any_of(std::begin(chunk) + from, std::end(chunk), [](uint8_t value) { return value != 0xff; })) {
            return false;
        }
    }

    return true;
}

bool write_block(const uint8_t* mac, bool replace) {
    if (replace) {
        if (!lt_flash_erase(FLASH_CALIBRATION_OFFSET, FLASH_CALIBRATION_LENGTH)) {
            return false;
        }
    } else if (!sector_holds(nullptr)) {
        return false;
    }

    const auto block = make_block(mac);
    const auto* data = reinterpret_cast<const uint8_t*>(&block);
    if (lt_flash_write(FLASH_CALIBRATION_OFFSET, data, sizeof(block)) != sizeof(block)) {
        return false;
    }

    return sector_holds(&block);
}

// BK7231N TRNG, the registers the SDK irda driver reads for TRNG_CMD_GET (its bk_rand() is not in the LibreTiny build).
// rand() of the libc is never seeded here and gives every board the same numbers
constexpr uint32_t TrngCtrl { 0x00802480 };
constexpr uint32_t TrngData { 0x00802484 };
constexpr uint32_t TrngEnable { 1 };

uint32_t hardware_random() {
    auto* ctrl = reinterpret_cast<volatile uint32_t*>(TrngCtrl);
    const auto* data = reinterpret_cast<const volatile uint32_t*>(TrngData);

    *ctrl = *ctrl | TrngEnable;

    uint32_t out { 0 };
    for (size_t index = 0; index < 8; ++index) {
        delayMicroseconds(100);
        out = (out << 7) ^ (out >> 25) ^ *data;
    }

    *ctrl = *ctrl & ~TrngEnable;

    return out;
}

// once, from whichever comes first: the chip id (the boot log asks for it before system setup) or pre_setup()
void ensure_unique() {
    static bool done { false };
    if (done) {
        return;
    }

    done = true;

    // lt_rand_bytes() and our RandomDevice are rand() as well
    srand(hardware_random());

    bool replace { false };

    uint8_t stored[6];
    if (manual_cal_get_macaddr_from_flash(stored)) {
        if (std::memcmp(stored, Unseeded, sizeof(stored)) != 0) {
            return;
        }

        // only our own block, a Tuya one with this MAC (not likely) stays as it is
        const auto ours = make_block(Unseeded);
        if (!sector_holds(&ours)) {
            return;
        }

        replace = true;
    }

    const auto random = hardware_random();
    generated[0] = 0xca;
    generated[1] = 0x47;
    generated[2] = 0x8c;
    generated[3] = (random >> 16) & 0xff;
    generated[4] = (random >> 8) & 0xff;
    generated[5] = random & 0xff;

    // not written means the default stays, a new random one on every boot would be worse
    result = write_block(generated, replace)
        ? (replace ? Result::Replaced : Result::Generated)
        : Result::Failed;
}

// the SDK reads the block once, the new MAC is used from the next boot on and set by hand for this one
void apply() {
    if ((result == Result::Generated) || (result == Result::Replaced)) {
        WiFi.setMacAddress(generated);
    }
}

void report() {
    switch (result) {
    case Result::Stored:
        break;
    case Result::Generated:
        DEBUG_MSG_P(PSTR("[MAIN] No RF calibration block, MAC %s written to a new one\n"),
            WiFi.macAddress().c_str());
        break;
    case Result::Replaced:
        DEBUG_MSG_P(PSTR("[MAIN] MAC of the first build was the same on every board, %s written instead\n"),
            WiFi.macAddress().c_str());
        break;
    case Result::Failed:
        DEBUG_MSG_P(PSTR("[MAIN] No MAC in the RF calibration block and none could be written, using the SDK default\n"));
        break;
    }
}

} // namespace mac
} // namespace

espurna::StringView systemChipId() {
    static String _chipid;
    if (!_chipid.length()) {
        mac::ensure_unique();

        uint8_t address[6];
        if ((mac::result == mac::Result::Generated) || (mac::result == mac::Result::Replaced)) {
            std::memcpy(address, mac::generated, sizeof(address));
        } else {
            lt_get_device_mac(address);
        }
        _chipid = hexEncode(address);
    }
    return _chipid;
}

espurna::StringView systemShortChipId() {
    static String _shortid;
    if (!_shortid.length()) _shortid = String(systemChipId().c_str() + 6);
    return _shortid;
}

uint32_t espurna::system::RandomDevice::operator()() const {
    uint32_t out;
    lt_rand_bytes(reinterpret_cast<uint8_t*>(&out), sizeof(out));
    return out;
}


// -----------------------------------------------------------------------------
// SLEEP
// -----------------------------------------------------------------------------

// Not implemented yet, callbacks are kept for the API
namespace sleep_impl {
namespace internal {

std::forward_list<SleepCallback> before;
std::forward_list<SleepCallback> after;

} // namespace internal
} // namespace sleep_impl

void systemBeforeSleep(SleepCallback callback) {
    sleep_impl::internal::before.push_front(callback);
}

void systemAfterSleep(SleepCallback callback) {
    sleep_impl::internal::after.push_front(callback);
}

bool instantLightSleep() {
    return false;
}

bool instantLightSleep(espurna::sleep::Microseconds) {
    return false;
}

bool instantLightSleep(uint8_t, espurna::sleep::Interrupt) {
    return false;
}

bool instantDeepSleep(espurna::sleep::Microseconds) {
    return false;
}

// -----------------------------------------------------------------------------
// SDK CONFIG
// -----------------------------------------------------------------------------

// esp8266 erases the SDK area, i.e. stored WiFi config and RF calibration.
// LibreTiny does not store WiFi config, RF calibration is kept
bool eraseSDKConfig() {
    return true;
}

[[noreturn]] void forceEraseSDKConfig() {
    eraseSDKConfig();
    customResetReason(CustomResetReason::Terminal);
    espurna::system::arch::restart();
}

// -----------------------------------------------------------------------------
// DEFERRED CALLBACKS (ref. system.h systemRunInLoop())
// -----------------------------------------------------------------------------

// Network handlers that could not take the loop lock in time (ref. AsyncGuard) hand
// their work over to the loop task. Queue is only ever touched under the mutex,
// callbacks themselves run outside of it, in the order they were queued.
namespace run_in_loop {
namespace {

std::mutex mutex;
std::vector<std::function<void()>> queue;

// Normally only a few callbacks are queued between two loop() iterations. A lot more means
// loop() is not keeping up with the network (or is stuck), which is reported once per burst.
constexpr size_t QueueWarning { 32 };
bool warned { false };

} // namespace

void push(std::function<void()> callback) {
    size_t size;
    bool warn { false };
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push_back(std::move(callback));
        size = queue.size();
        if (!warned && (size > QueueWarning)) {
            warned = true;
            warn = true;
        }
    }

    // outside of the queue mutex, debug output from other tasks is queued as well
    if (warn) {
        DEBUG_MSG_P(PSTR("[MAIN] Loop is falling behind, %u deferred callbacks queued\n"), size);
    }
}

void drain() {
    std::vector<std::function<void()>> current;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty()) {
            return;
        }
        current.swap(queue);
        if (current.size() <= QueueWarning) {
            warned = false;
        }
    }

    // anything queued while running goes into the next loop() iteration
    for (auto& callback : current) {
        callback();
    }
}

} // namespace run_in_loop

void systemRunInLoop(std::function<void()> callback) {
    if (callback) {
        run_in_loop::push(std::move(callback));
    }
}

// -----------------------------------------------------------------------------
// COMMON SYSTEM MODULE HOOKS (ref. system.h)
// -----------------------------------------------------------------------------

namespace espurna {
namespace system {
namespace {
namespace pending_output {

// ws + telnet, only registered once from their setup()
constexpr size_t ChecksMax { 4 };
PendingOutput checks[ChecksMax] {};

bool any() {
    for (const auto& check : checks) {
        if (check && check()) {
            return true;
        }
    }

    return false;
}

} // namespace pending_output
} // namespace

void registerPendingOutput(PendingOutput check) {
    for (auto& slot : pending_output::checks) {
        if (!slot || (slot == check)) {
            slot = check;
            return;
        }
    }
}

namespace arch {

void pre_setup() {
    loopTaskHandle = xTaskGetCurrentTaskHandle();

    // AsyncTCP servers are started before WiFi is, make sure the TCP/IP stack is up.
    // LibreTiny only initializes the SDK (lwip tcpip thread included) on the first WiFi.mode()
    // change, until then any tcpip_api_call() waits forever. Station is not connected here,
    // WiFi module sets the mode it needs later on
    WiFi.mode(WIFI_MODE_STA);
    WiFi.mode(WIFI_MODE_NULL);

    // the chip id is made of the MAC, which has to be unique by now
    mac::ensure_unique();
    mac::apply();
}

void setup() {
    mac::report();

    // Same as esp8266 soft WDT, reset when loop() gets stuck.
    // Enabled from the first loop(), since the rest of setup() is allowed to take its time.
    espurnaRegisterOnce([]() {
        lt_wdt_enable(LoopWdtTimeout);
    });
}

void loop() {
    timer::SystemTimer::dispatch();
    run_in_loop::drain();
}

// Same values as esp8266, common code (e.g. the stability check) relies on them
uint32_t reset_reason() {
    // SDK sees the system word written by restart() or our crash handlers as an unknown start type,
    // i.e. a watchdog reset
    if (rtcmemSysStatus()) {
        return (rtcmemCrash() != RtcmemCrash::None)
            ? REASON_EXCEPTION_RST
            : REASON_SOFT_RESTART;
    }

    switch (lt_get_reboot_reason()) {
    case REBOOT_REASON_HARDWARE:
        return REASON_EXT_SYS_RST;
    case REBOOT_REASON_SOFTWARE:
        return REASON_SOFT_RESTART;
    case REBOOT_REASON_CRASH:
        return REASON_EXCEPTION_RST;
    case REBOOT_REASON_WATCHDOG:
        return REASON_WDT_RST;
    case REBOOT_REASON_SLEEP_GPIO:
    case REBOOT_REASON_SLEEP_RTC:
    case REBOOT_REASON_SLEEP_USB:
        return REASON_DEEP_SLEEP_AWAKE;
    // power related, not the firmware's fault
    default:
        break;
    }

    return REASON_DEFAULT_RST;
}

[[noreturn]] void restart() {
    // esp8266 ESP.restart() only takes effect once the SDK gets to run again, so network output
    // queued right before it (e.g. +OK of the RESET command) still has a chance to be sent.
    // lt_reboot() is immediate, give the network tasks a moment to flush when called from loop().
    if (xTaskGetCurrentTaskHandle() == loopTaskHandle) {
        constexpr uint32_t FlushMin { 250 };
        constexpr uint32_t FlushMax { 3000 };
        constexpr uint32_t FlushInterval { 50 };

        delay_unlocked(FlushMin);
        for (uint32_t waited = FlushMin; (waited < FlushMax) && pending_output::any(); waited += FlushInterval) {
            delay_unlocked(FlushInterval);
        }
    }

    // settings are committed by the loop, make sure the last change is not lost
    eepromForceCommit();

    // lt_reboot() -> bk_reboot() writes the reboot type into the retention register. Same reboot,
    // but with the system word (custom reset reason) there instead (ref. rtcmem_beken.cpp)
    constexpr uintptr_t RegRetention { 0x00800150 };

    portDISABLE_INTERRUPTS();
    *reinterpret_cast<volatile uint32_t*>(RegRetention) = rtcmemRetention();

    lt_wdt_disable();
    lt_wdt_enable(5);
    for (;;) {
    }
}

} // namespace arch
} // namespace system
} // namespace espurna

// -----------------------------------------------------------------------------
// LOOP / ASYNCTCP SERIALIZATION (ref. system_beken.h)
// -----------------------------------------------------------------------------

namespace espurna {
namespace system {
namespace {
namespace lock_impl {

constexpr auto AsyncTimeout = duration::Milliseconds{ 100 };

SemaphoreHandle_t mutex { nullptr };
std::atomic<TaskHandle_t> owner { nullptr };
size_t depth { 0 };

// First lock() happens from the loop task in setup(), before any network task is running
SemaphoreHandle_t handle() {
    if (!mutex) {
        mutex = xSemaphoreCreateMutex();
    }

    return mutex;
}

bool take(TickType_t ticks) {
    const auto self = xTaskGetCurrentTaskHandle();
    if (owner.load() == self) {
        ++depth;
        return true;
    }

    if (xSemaphoreTake(handle(), ticks) != pdTRUE) {
        return false;
    }

    owner.store(self);
    depth = 1;

    return true;
}

void give() {
    if (owner.load() != xTaskGetCurrentTaskHandle()) {
        return;
    }

    if (--depth == 0) {
        owner.store(nullptr);
        xSemaphoreGive(handle());
    }
}

} // namespace lock_impl
} // namespace

void lock() {
    lock_impl::take(portMAX_DELAY);
}

bool lock(duration::Milliseconds timeout) {
    return lock_impl::take(pdMS_TO_TICKS(timeout.count()));
}

void unlock() {
    lock_impl::give();
}

void delay_unlocked(uint32_t ms) {
    const auto self = xTaskGetCurrentTaskHandle();

    // Same as esp8266 delay(), sleeping in the loop task keeps the watchdog happy
    // (yield() only feeds it between loop() calls)
    if (self == loopTaskHandle) {
        lt_wdt_feed();
    }

    if (lock_impl::owner.load() != self) {
        ::delay(ms);
        return;
    }

    const auto depth = lock_impl::depth;
    lock_impl::depth = 0;
    lock_impl::owner.store(nullptr);
    xSemaphoreGive(lock_impl::handle());

    ::delay(ms);

    xSemaphoreTake(lock_impl::handle(), portMAX_DELAY);
    lock_impl::owner.store(self);
    lock_impl::depth = depth;
}

AsyncGuard::AsyncGuard() :
    _locked(lock(lock_impl::AsyncTimeout))
{
    // Safe from any task, debug output coming from outside of the loop task is queued
    // (ref. debug.cpp deferred::push()) and sent out by loop() later
    if (!_locked) {
        DEBUG_MSG_P(PSTR("[MAIN] Loop is busy, network handler could not take the lock\n"));
    }
}

AsyncGuard::~AsyncGuard() {
    if (_locked) {
        unlock();
    }
}

} // namespace system
} // namespace espurna

// -----------------------------------------------------------------------------
// SETTINGS HELPERS
// -----------------------------------------------------------------------------

namespace espurna {
namespace settings {
namespace internal {

// Same names as esp8266 ('hardware', 'mcp23s08', 'none'), numbers are accepted as well
template <>
GpioType convert(const String& v) {
    if (v.equalsIgnoreCase("hardware")) return GpioType::Hardware;
    if (v.equalsIgnoreCase("mcp23s08")) return GpioType::Mcp23s08;
    if (v.equalsIgnoreCase("none")) return GpioType::None;
    if (v.length() && isdigit(static_cast<unsigned char>(v[0]))) {
        return static_cast<GpioType>(v.toInt());
    }
    return GpioType::None;
}

String serialize(GpioType v) {
    switch (v) {
    case GpioType::Hardware: return F("hardware");
    case GpioType::Mcp23s08: return F("mcp23s08");
    case GpioType::None: break;
    }
    return F("none");
}

String serialize(std::array<unsigned char, 6u> mac) {
    return hexEncode(mac);
}

template <>
StringSumHelper convert(const String& v) {
    return StringSumHelper(v);
}

} // namespace internal
} // namespace settings
} // namespace espurna

// espurnaRegisterOnce / espurnaLoopDelay defined in main.cpp.
// migrateVersion / delSettingPrefix defined in migrate.cpp.
String getSetting(espurna::StringView key) { return ::getSetting(key.toString()); }
bool delSetting(espurna::StringView key) { return ::delSetting(key.toString()); }
bool hasSetting(espurna::StringView key) { return ::hasSetting(key.toString()); }

#endif // LIBRETINY
