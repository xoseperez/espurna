/*

SYSTEM MODULE

Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

Common part, shared by every architecture. Chip specific parts are implemented
in arch/<arch>/system_<arch>.cpp (ref. espurna::system::arch in system.h)

*/

#include "espurna.h"

#include "rtcmem.h"

#if WEB_SUPPORT
#include "ws.h"
#endif

#if LED_SUPPORT
#include "led.h"
#endif

#if BUTTON_SUPPORT
#include "button.h"
#endif

#include <cstdint>
#include <cstring>
#include <forward_list>
#include <random>
#include <vector>

// -----------------------------------------------------------------------------

namespace espurna {
namespace system {
namespace {

namespace settings {
namespace options {

PROGMEM_STRING(None, "none");
PROGMEM_STRING(Once, "once");
PROGMEM_STRING(Repeat, "repeat");

template <typename T>
using Enumeration = espurna::settings::options::Enumeration<T>;

static constexpr Enumeration<heartbeat::Mode> HeartbeatModeOptions[] PROGMEM {
    {heartbeat::Mode::None, None},
    {heartbeat::Mode::Once, Once},
    {heartbeat::Mode::Repeat, Repeat},
};

PROGMEM_STRING(Low, "low");
PROGMEM_STRING(High, "high");

static constexpr Enumeration<sleep::Interrupt> SleepInterruptOptions[] PROGMEM {
    {sleep::Interrupt::Low, Low},
    {sleep::Interrupt::High, High},
};

} // namespace options

namespace keys {

STRING_VIEW_INLINE(Hostname, "hostname");
STRING_VIEW_INLINE(Description, "desc");
STRING_VIEW_INLINE(Password, "adminPass");

} // namespace keys

} // namespace settings
} // namespace
} // namespace system

namespace settings {
namespace internal {

template <>
espurna::sleep::Interrupt convert(const String& value) {
    return convert(system::settings::options::SleepInterruptOptions, value, sleep::Interrupt::Low);
}

String serialize(espurna::sleep::Interrupt value) {
    return serialize(system::settings::options::SleepInterruptOptions, value);
}

template <>
espurna::heartbeat::Mode convert(const String& value) {
    return convert(system::settings::options::HeartbeatModeOptions, value, heartbeat::Mode::Repeat);
}

String serialize(espurna::heartbeat::Mode mode) {
    return serialize(system::settings::options::HeartbeatModeOptions, mode);
}

} // namespace internal
} // namespace settings

// -----------------------------------------------------------------------------

namespace sleep {
namespace settings {
namespace {
namespace build {

static constexpr auto DefaultInterrupt = espurna::sleep::Interrupt::Low;
static constexpr auto DefaultPin = uint8_t{ GPIO_NONE };

} // namespace build

namespace keys {

PROGMEM_STRING(Pin, "sleepPin");
PROGMEM_STRING(Interrupt, "sleepIntr");

} // namespace keys
} // namespace

uint8_t pin() {
    return getSetting(keys::Pin, build::DefaultPin);
}

espurna::sleep::Interrupt interrupt() {
    return getSetting(keys::Interrupt, build::DefaultInterrupt);
}

} // namespace settings
} // namespace sleep

// -----------------------------------------------------------------------------

namespace system {
namespace {

namespace internal {

STRING_VIEW_INLINE(Hostname, HOSTNAME);
STRING_VIEW_INLINE(Password, ADMIN_PASS);

} // namespace internal

StringView device() {
    const static String out = ([]() {
        String out;

        const auto hardware = buildHardware();
        out.concat(hardware.manufacturer.c_str(),
            hardware.manufacturer.length());
        out += '_';
        out.concat(hardware.device.c_str(),
            hardware.device.length());

        return out;
    })();

    return out;
}

StringView identifier() {
    const static String out = ([]() {
        String out;

        const auto app = buildApp();
        out.concat(app.name.c_str(), app.name.length());

        out += '-';

        const auto id = systemShortChipId();
        out.concat(id.c_str(), id.length());

        return out;
    })();

    return out;
}

String description() {
    return getSetting(settings::keys::Description);
}

String hostname() {
    const auto defaultValue = (internal::Hostname.length() > 0)
        ? internal::Hostname
        : identifier();

    return getSetting(settings::keys::Hostname, defaultValue);
}

StringView default_password() {
    return internal::Password;
}

String password() {
    return getSetting(settings::keys::Password, default_password());
}

bool password_equals(StringView other) {
    const auto password = system::password();
    return other == password;
}

namespace settings {
namespace query {

#define EXACT_VALUE(NAME, FUNC)\
String NAME () {\
    return espurna::settings::internal::serialize(FUNC());\
}

EXACT_VALUE(sleepPin, sleep::settings::pin);
EXACT_VALUE(sleepInterrupt, sleep::settings::interrupt);

#undef EXACT_VALUE

static constexpr std::array<espurna::settings::query::Setting, 5> Settings PROGMEM {{
     {keys::Description, system::description},
     {keys::Hostname, system::hostname},
     {keys::Password, system::password},

     {sleep::settings::keys::Pin, query::sleepPin},
     {sleep::settings::keys::Interrupt, query::sleepInterrupt},
}};

espurna::settings::query::Result findFrom(StringView key) {
    return espurna::settings::query::findFrom(Settings, key);
}

void setup() {
    settingsRegisterQueryHandler({
        .check = nullptr,
        .get = findFrom,
    });
}

} // namespace query
} // namespace settings

} // namespace
} // namespace system

// -----------------------------------------------------------------------------

bool ReadyFlag::wait(duration::Milliseconds interval) {
    if (_ready) {
        _ready = false;
        _timer.schedule_once(
            interval,
            [&]() {
                _ready = true;
            });

        return true;
    }

    return false;
}

void ReadyFlag::stop() {
    _timer.stop();
    _ready = true;
}

constexpr auto PolledReadFlagHalfInterval = time::SystemClock::duration::max() / 2;

bool PolledReadyFlag::wait(duration::Milliseconds interval) {
    if (_ready) {
        const auto now = time::SystemClock::now();
        _ready = false;
        _until = now + interval;
        return true;
    }

    return false;
}

void PolledReadyFlag::stop() {
    _ready = true;
}

bool PolledReadyFlag::ready() {
    if (!_ready) {
        const auto now = time::SystemClock::now();
        _ready = (now - _until) < PolledReadFlagHalfInterval;
    }

    return _ready;
}

template struct PolledFlag<time::CoreClock>;

// -----------------------------------------------------------------------------

namespace {

namespace boot {

String serialize(CustomResetReason reason) {
    const char* ptr { PSTR("None") };

    switch (reason) {
    case CustomResetReason::None:
        break;
    case CustomResetReason::Button:
        ptr = PSTR("Hardware button");
        break;
    case CustomResetReason::Factory:
        ptr = PSTR("Factory reset");
        break;
    case CustomResetReason::Hardware:
        ptr = PSTR("Reboot from a Hardware request");
        break;
    case CustomResetReason::Mqtt:
        ptr = PSTR("Reboot from MQTT");
        break;
    case CustomResetReason::Ota:
        ptr = PSTR("Reboot after a successful OTA update");
        break;
    case CustomResetReason::Rpc:
        ptr = PSTR("Reboot from a RPC action");
        break;
    case CustomResetReason::Rule:
        ptr = PSTR("Reboot from an automation rule");
        break;
    case CustomResetReason::Scheduler:
        ptr = PSTR("Reboot from a scheduler action");
        break;
    case CustomResetReason::Terminal:
        ptr = PSTR("Reboot from a terminal command");
        break;
    case CustomResetReason::Web:
        ptr = PSTR("Reboot from web interface");
        break;
    case CustomResetReason::Stability:
        ptr = PSTR("Reboot after changing stability counter");
        break;
    }

    return ptr;
}

// The ESPLive has an ADC MUX which needs to be configured.
// Default CT input (pin B, solder jumper B)
void hardware() {
#if defined(MANCAVEMADE_ESPLIVE)
    pinMode(16, OUTPUT);
    digitalWrite(16, HIGH);
#endif
}

// If the counter reaches SYSTEM_CHECK_MAX then the system is flagged as unstable
// When it that mode, system will only have minimal set of services available
struct Data {
    Data() = delete;
    explicit Data(volatile uint32_t* ptr) :
        _ptr(ptr)
    {}

    bool status() const {
        return rtcmemStatus();
    }

    uint8_t counter() const {
        return read().counter;
    }

    void counter(uint8_t input) {
        auto value = read();
        value.counter = input;
        write(value);
    }

    CustomResetReason reason() const {
        return static_cast<CustomResetReason>(read().reason);
    }

    void reason(CustomResetReason input) {
        auto value = read();
        value.reason = static_cast<uint8_t>(input);
        write(value);
    }

    uint32_t value() const {
        return *_ptr;
    }

private:
    struct alignas(uint32_t) Raw {
        uint8_t counter;
        uint8_t reason;
        uint8_t _stub1;
        uint8_t _stub2;
    };

    static_assert(sizeof(Raw) == sizeof(uint32_t), "");
    static_assert(alignof(Raw) == alignof(uint32_t), "");

    void write(Raw raw) {
        uint32_t out{};
        std::memcpy(&out, &raw, sizeof(out));
        *_ptr = out;
    }

    Raw read() const {
        uint32_t value = *_ptr;

        Raw out{};
        std::memcpy(&out, &value, sizeof(out));

        return out;
    }

    volatile uint32_t* _ptr;
};

namespace internal {

Data persistent_data { &Rtcmem->sys };

timer::SystemTimer timer;
bool flag { true };

} // namespace internal

uint32_t system_reason() {
    return system::arch::reset_reason();
}

// prunes custom reason after accessing it once
CustomResetReason customReason() {
    static const CustomResetReason reason = ([]() {
        const auto out = internal::persistent_data.status()
            ? internal::persistent_data.reason()
            : CustomResetReason::None;
        internal::persistent_data.reason(CustomResetReason::None);
        return out;
    })();

    return reason;
}

void customReason(CustomResetReason reason) {
    internal::persistent_data.reason(reason);
}

#if SYSTEM_CHECK_ENABLED
namespace stability {
namespace build {

static constexpr auto ChecksMin = uint8_t{ 1 };
static constexpr auto ChecksMax = uint8_t{ SYSTEM_CHECK_MAX };

static_assert(ChecksMax > 1, "");
static_assert(ChecksMin < ChecksMax, "");
static_assert(ChecksMax != std::numeric_limits<decltype(ChecksMax)>::max(), "");

constexpr espurna::duration::Seconds CheckTime { SYSTEM_CHECK_TIME };
static_assert(CheckTime > decltype(CheckTime)::min(), "");

} // namespace build

bool is_stable(uint8_t count) {
    return count < build::ChecksMax;
}

bool check_unstable() {
    return internal::flag;
}

void force_stable() {
    internal::persistent_data.counter(build::ChecksMin);
    internal::flag = true;
}

void force_unstable() {
    internal::persistent_data.counter(build::ChecksMax);
    internal::flag = false;
}

uint8_t counter() {
    return internal::persistent_data.status()
        ? internal::persistent_data.counter()
        : build::ChecksMin;
}

bool is_unstable_reset() {
    return counter() > (build::ChecksMax + 1);
}

void reset() {
    DEBUG_MSG_P(PSTR("[MAIN] Resetting stability counter\n"));
    internal::persistent_data.counter(build::ChecksMin);
}

void init() {
    const auto count = std::clamp(
        counter(),
        build::ChecksMin, build::ChecksMax);

    // normally, check if the counter can still be incremented
    auto update_flag = [&]() {
        internal::flag = is_stable(count);
    };

    // if not, system is flagged as unstable
    // set up the timer to reset counting cycle after some time
    // note that count allows +1 over the max to detect unstable mode itself resetting
    auto update_persist = [&]() {
        const auto next = static_cast<uint8_t>(count + 1);
        internal::persistent_data.counter(next);

        internal::timer.once(build::CheckTime, reset);
    };

    switch (system_reason()) {
    // initial boot and rst are probably just fine
    case REASON_DEFAULT_RST:
    case REASON_EXT_SYS_RST:
        force_stable();
        return;
    // no need to run the timer when counter gets changed manually
    case REASON_SOFT_RESTART:
        if (customReason() == CustomResetReason::Stability) {
            update_flag();
            return;
        }
        break;
    }

    update_flag();
    update_persist();
}

void setup_unstable() {
    if (!is_unstable_reset()) {
#if LED_SUPPORT
        ledSetupUnstable();
#endif
#if BUTTON_SUPPORT
        buttonSetupUnstable();
#endif
    }
}

} // namespace stability
#endif

} // namespace boot

// -----------------------------------------------------------------------------

// Calculated load average of the loop() as a percentage (notice that this may not be accurate)
namespace load_average {
namespace build {

static constexpr size_t ValueMax { 100 };

static constexpr espurna::duration::Seconds Interval { LOADAVG_INTERVAL };
static_assert(Interval <= espurna::duration::Seconds(90), "");

} // namespace build

using TimeSource = espurna::time::SystemClock;
using Type = unsigned long;

struct Counter {
    TimeSource::time_point last;
    Type count;
    Type value;
    Type max;
};

namespace internal {

Type load_average { 0 };

} // namespace internal

Type value() {
    return internal::load_average;
}

void loop() {
    static Counter counter {
        .last = TimeSource::now(),
        .count = 0,
        .value = 0,
        .max = 0
    };

    ++counter.count;

    const auto timestamp = TimeSource::now();
    if (timestamp - counter.last < build::Interval) {
        return;
    }

    counter.last = timestamp;
    counter.value = counter.count;
    counter.count = 0;
    counter.max = std::max(counter.max, counter.value);

    internal::load_average = counter.max
        ? (build::ValueMax - (build::ValueMax * counter.value / counter.max))
        : 0;
}

} // namespace load_average
} // namespace

// -----------------------------------------------------------------------------

namespace heartbeat {
namespace {

namespace build {

constexpr Mode mode() {
    return HEARTBEAT_MODE;
}

constexpr espurna::duration::Seconds interval() {
    return espurna::duration::Seconds { HEARTBEAT_INTERVAL };
}

constexpr Mask value() {
    return (Report::Status * (HEARTBEAT_REPORT_STATUS))
        | (Report::Ssid * (HEARTBEAT_REPORT_SSID))
        | (Report::Ip * (HEARTBEAT_REPORT_IP))
        | (Report::Mac * (HEARTBEAT_REPORT_MAC))
        | (Report::Rssi * (HEARTBEAT_REPORT_RSSI))
        | (Report::Uptime * (HEARTBEAT_REPORT_UPTIME))
        | (Report::Datetime * (HEARTBEAT_REPORT_DATETIME))
        | (Report::Freeheap * (HEARTBEAT_REPORT_FREEHEAP))
        | (Report::Vcc * (HEARTBEAT_REPORT_VCC))
        | (Report::Relay * (HEARTBEAT_REPORT_RELAY))
        | (Report::Light * (HEARTBEAT_REPORT_LIGHT))
        | (Report::Hostname * (HEARTBEAT_REPORT_HOSTNAME))
        | (Report::Description * (HEARTBEAT_REPORT_DESCRIPTION))
        | (Report::App * (HEARTBEAT_REPORT_APP))
        | (Report::Version * (HEARTBEAT_REPORT_VERSION))
        | (Report::Board * (HEARTBEAT_REPORT_BOARD))
        | (Report::Loadavg * (HEARTBEAT_REPORT_LOADAVG))
        | (Report::Interval * (HEARTBEAT_REPORT_INTERVAL))
        | (Report::Range * (HEARTBEAT_REPORT_RANGE))
        | (Report::RemoteTemp * (HEARTBEAT_REPORT_REMOTE_TEMP))
        | (Report::Bssid * (HEARTBEAT_REPORT_BSSID));
}

} // namespace build

namespace settings {
namespace keys {

PROGMEM_STRING(Mode, "hbMode");
PROGMEM_STRING(Interval, "hbInterval");
PROGMEM_STRING(Report, "hbReport");

} // namespace keys

Mode mode() {
    return getSetting(keys::Mode, build::mode());
}

espurna::duration::Seconds interval() {
    return getSetting(keys::Interval, build::interval());
}

Mask value() {
    // because we start shifting from 1, we could use the
    // first bit as a flag to enable all of the messages
    static constexpr Mask MaskAll { 1 };

    auto value = getSetting(keys::Report, build::value());
    if (value == MaskAll) {
        value = std::numeric_limits<Mask>::max();
    }

    return value;
}

} // namespace settings

using TimeSource = espurna::time::CoreClock;

struct CallbackRunner {
    Callback callback;
    Mode mode;
    TimeSource::duration interval;
    TimeSource::time_point last;
};

namespace internal {

timer::SystemTimer timer;
std::vector<CallbackRunner> runners;
bool scheduled { false };

} // namespace internal

void schedule() {
    internal::scheduled = true;
}

bool scheduled() {
    if (internal::scheduled) {
        internal::scheduled = false;
        return true;
    }

    return false;
}

void run() {
    static constexpr duration::Milliseconds BeatMin { duration::Seconds(1) };
    static constexpr duration::Milliseconds BeatMax { BeatMin * 10 };

    auto next = duration::Milliseconds(settings::interval());

    if (internal::runners.size()) {
        auto mask = settings::value();

        auto it = internal::runners.begin();
        auto end = internal::runners.end();

        auto ts = TimeSource::now();
        while (it != end) {
            auto diff = ts - (*it).last;
            if (diff > (*it).interval) {
                auto result = (*it).callback(mask);
                if (result && ((*it).mode == Mode::Once)) {
                    it = internal::runners.erase(it);
                    end = internal::runners.end();
                    continue;
                }

                if (result) {
                    (*it).last = ts;
                } else if (diff < ((*it).interval + BeatMax)) {
                    next = BeatMin;
                }

                next = std::min(next, (*it).interval);
            } else {
                next = std::min(next, (*it).interval - diff);
            }
            ++it;
        }
    }

    if (next < BeatMin) {
        next = BeatMin;
    }

    internal::timer.once(next, schedule);
}

void stop(Callback callback) {
    auto found = std::remove_if(
        internal::runners.begin(),
        internal::runners.end(),
        [&](const CallbackRunner& runner) {
            return callback == runner.callback;
        });
    internal::runners.erase(found, internal::runners.end());
}

void push(Callback callback, Mode mode, duration::Seconds interval) {
    if (mode == Mode::None) {
        return;
    }

    auto msec = duration::Milliseconds(interval);
    if ((mode != Mode::Once) && !msec.count()) {
        return;
    }

    auto offset = TimeSource::now() - TimeSource::duration(1);
    internal::runners.push_back({
        callback, mode,
        msec,
        offset - msec
    });

    internal::timer.stop();
    schedule();
}

[[gnu::unused]]
void push_once(Callback callback) {
    push(callback, Mode::Once, espurna::duration::Seconds::min());
}

duration::Seconds interval() {
    TimeSource::duration result { settings::interval() };

    for (auto& runner : internal::runners) {
        if (runner.mode != Mode::Once) {
            result = std::min(result, runner.interval);
        }
    }

    return std::chrono::duration_cast<duration::Seconds>(result);
}

void reschedule() {
    static constexpr TimeSource::duration Offset { 1 };

    const auto ts = TimeSource::now();
    for (auto& runner : internal::runners) {
        runner.last = ts - runner.interval - Offset;
    }

    schedule();
}

void loop() {
    if (scheduled()) {
        run();
    }
}

void init() {
#if DEBUG_SUPPORT
    push_once([](Mask) {
        const auto mode = settings::mode();
        if (mode != Mode::None) {
            DEBUG_MSG_P(PSTR("[MAIN] Heartbeat \"%s\", every %u (seconds)\n"),
                espurna::settings::internal::serialize(mode).c_str(),
                settings::interval().count());
        } else {
            DEBUG_MSG_P(PSTR("[MAIN] Heartbeat disabled\n"));
        }
        return true;
    });
#if SYSTEM_CHECK_ENABLED
    push_once([](Mask) {
        if (!espurna::boot::stability::check_unstable()) {
            DEBUG_MSG_P(PSTR("[MAIN] System UNSTABLE\n"));
        } else if (espurna::boot::internal::timer) {
            DEBUG_MSG_P(PSTR("[MAIN] Pending stability counter reset...\n"));
        }
        return true;
    });
#endif
#endif
    schedule();
}

} // namespace

// system defaults, r/n this is used when providing module-specific settings

espurna::duration::Milliseconds currentIntervalMs() {
    return settings::interval();
}

espurna::duration::Seconds currentInterval() {
    return settings::interval();
}

Mask currentValue() {
    return settings::value();
}

Mode currentMode() {
    return settings::mode();
}

} // namespace heartbeat

// -----------------------------------------------------------------------------

namespace {

#if WEB_SUPPORT
namespace web {

void onConnected(JsonObject& root) {
  root[FPSTR(heartbeat::settings::keys::Report)] = heartbeat::settings::value();
  root[FPSTR(heartbeat::settings::keys::Interval)] =
      heartbeat::settings::interval().count();
  root[FPSTR(heartbeat::settings::keys::Mode)] =
      espurna::settings::internal::serialize(heartbeat::settings::mode());
}

bool onKeyCheck(StringView key, const JsonVariant& value) {
    if (key == system::settings::keys::Password) {
        const auto password = system::password();
#if defined(ESP8266)
        return !password.equalsConstantTime(value.as<String>());
#else
        return !password.equals(value.as<String>());
#endif
    }

    return (key == system::settings::keys::Description)
        || (key == system::settings::keys::Hostname)
        || key.startsWith(STRING_VIEW("hb"))
        || key.startsWith(STRING_VIEW("sleep"))
        || key.startsWith(STRING_VIEW("sys"));
}

void init() {
    wsRegister()
        .onConnected(onConnected)
        .onKeyCheck(onKeyCheck, ::espurna::web::ws::Callbacks::Prepend{});
}

} // namespace web
#endif

// Allow to schedule a reset at the next loop
// Store reset reason both here and in for the next boot
namespace internal {

timer::SystemTimer reset_timer;
auto reset_reason = CustomResetReason::None;

void reset(CustomResetReason reason) {
    ::espurna::boot::customReason(reason);
    reset_reason = reason;
}

} // namespace internal

// 'simple' reboot call with software controlled time
// always needs a reason, so it can be displayed in logs and / or trigger some actions on boot
void pending_reset_loop() {
    if (internal::reset_reason != CustomResetReason::None) {
        system::arch::restart();
    }
}

static constexpr espurna::duration::Milliseconds ShortDelayForReset { 500 };

void deferredReset(duration::Milliseconds delay, CustomResetReason reason) {
    DEBUG_MSG_P(PSTR("[MAIN] Requested reset: %s\n"),
        espurna::boot::serialize(reason).c_str());
    internal::reset_timer.once(
        delay,
        [reason]() {
            internal::reset(reason);
        });
}

// Accumulates only when called, make sure to do so periodically
// Even in 32bit range, seconds would take a lot of time to overflow
duration::Seconds uptime() {
    return std::chrono::duration_cast<duration::Seconds>(
        time::SystemClock::now().time_since_epoch());
}

void loop() {
    system::arch::loop();
    pending_reset_loop();
    load_average::loop();
    heartbeat::loop();
}

void setup() {
    system::arch::pre_setup();

    boot::hardware();
    boot::customReason();

#if SYSTEM_CHECK_ENABLED
    boot::stability::init();
#endif

#if WEB_SUPPORT
    web::init();
#endif

    system::settings::query::setup();

    espurnaRegisterLoop(loop);
    heartbeat::init();

    system::arch::setup();
}

} // namespace
} // namespace espurna

// -----------------------------------------------------------------------------

// using 'random device' as-is, while most common implementations
// would've used it as a seed for some generator func
// TODO notice that stdlib std::mt19937 struct needs ~2KiB for it's internal
// `result_type state[std::mt19937::state_size]` (ref. sizeof())
uint32_t randomNumber(uint32_t minimum, uint32_t maximum) {
    using Device = espurna::system::RandomDevice;
    using Type = Device::result_type;

    static Device random;
    auto distribution = std::uniform_int_distribution<Type>(minimum, maximum);

    return distribution(random);
}

uint32_t randomNumber() {
    return (espurna::system::RandomDevice{})();
}

unsigned long systemLoadAverage() {
    return espurna::load_average::value();
}

void factoryReset() {
    resetSettings();
    espurna::deferredReset(
        espurna::ShortDelayForReset,
        CustomResetReason::Factory);
}

void deferredReset(espurna::duration::Milliseconds delay, CustomResetReason reason) {
    espurna::deferredReset(delay, reason);
}

void prepareReset(CustomResetReason reason) {
    espurna::deferredReset(espurna::ShortDelayForReset, reason);
}

bool pendingDeferredReset() {
    return espurna::internal::reset_reason != CustomResetReason::None;
}

uint32_t systemResetReason() {
    return espurna::boot::system_reason();
}

CustomResetReason customResetReason() {
    return espurna::boot::customReason();
}

void customResetReason(CustomResetReason reason) {
    espurna::boot::customReason(reason);
}

String customResetReasonToPayload(CustomResetReason reason) {
    return espurna::boot::serialize(reason);
}

#if SYSTEM_CHECK_ENABLED
uint8_t systemStabilityCounter() {
    return espurna::boot::internal::persistent_data.counter();
}

void systemStabilityCounter(uint8_t count) {
    espurna::boot::internal::persistent_data.counter(count);
}

void systemForceUnstable() {
    espurna::boot::stability::force_unstable();
}

void systemForceStable() {
    espurna::boot::stability::force_stable();
}

bool systemCheck() {
    return espurna::boot::stability::check_unstable();
}

void systemSetupUnstable() {
    return espurna::boot::stability::setup_unstable();
}
#endif

void systemStopHeartbeat(espurna::heartbeat::Callback callback) {
    espurna::heartbeat::stop(callback);
}

void systemHeartbeat(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode, espurna::duration::Seconds interval) {
    espurna::heartbeat::push(callback, mode, interval);
}

void systemHeartbeat(espurna::heartbeat::Callback callback, espurna::heartbeat::Mode mode) {
    espurna::heartbeat::push(callback, mode,
        espurna::heartbeat::settings::interval());
}

void systemHeartbeat(espurna::heartbeat::Callback callback) {
    espurna::heartbeat::push(callback,
        espurna::heartbeat::settings::mode(),
        espurna::heartbeat::settings::interval());
}

espurna::duration::Seconds systemHeartbeatInterval() {
    return espurna::heartbeat::interval();
}

void systemScheduleHeartbeat() {
    espurna::heartbeat::reschedule();
}

espurna::duration::Seconds systemUptime() {
    return espurna::uptime();
}

espurna::StringView systemDevice() {
    return espurna::system::device();
}

espurna::StringView systemIdentifier() {
    return espurna::system::identifier();
}

espurna::StringView systemDefaultPassword() {
    return espurna::system::default_password();
}

String systemPassword() {
    return espurna::system::password();
}

bool systemPasswordEquals(espurna::StringView other) {
    return espurna::system::password_equals(other);
}

String systemHostname() {
    return espurna::system::hostname();
}

String systemDescription() {
    return espurna::system::description();
}

void systemSetup() {
    espurna::setup();
}
