/*

RELAY MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if RELAY_SUPPORT

#if API_SUPPORT
#include "api.h"
#endif

#if MQTT_SUPPORT
#include "api_path.h"
#endif

#if WEB_SUPPORT
#include "ws.h"
#endif

#include "mqtt.h"
#include "relay.h"
#include "fan.h"
#include "tuya.h"
#include "rpc.h"
#include "rtcmem.h"
#include "light.h"
#include "settings.h"
#include "storage_eeprom.h"
#include "terminal.h"
#include "utils.h"

#include <ArduinoJson.h>

#include <bitset>
#include <cstring>
#include <functional>
#include <vector>

// -----------------------------------------------------------------------------

enum class RelayBoot {
    Off,
    On,
    Same,
    Toggle,
    LockedOff,
    LockedOn
};

enum class RelayLock {
    None,
    Off,
    On,
};

enum class RelayType {
    Normal,
    Inverse,
    Latched,
    LatchedInverse
};

enum class RelayMqttTopicMode {
    Normal,
    Inverse
};

enum class RelayProvider {
    None,
    Dummy,
    Gpio,
    Dual,
    Stm,
    LightState,
    Fan,
    Lightfox,
    Tuya,
};

enum class RelaySync {
    None,
    ZeroOrOne,
    JustOne,
    All,
    First
};

namespace {

// XXX flags need some care, as not resetting some of them in-between calls would cause some weird errors w/ state

// Allow to forcibly change status even when current == target
// When processing, this flag would make relay globally accessible (which is not otherwise)
constexpr auto RelayFlagBoot = uint8_t{ 1 << 0 };

// Distinguish status calls from the synchronization step and everything else
constexpr auto RelayFlagSync = uint8_t{ 1 << 1 };

// External reporting. Currently, MQTT status & custom topic
constexpr auto RelayFlagReport = uint8_t{ 1 << 2 };
constexpr auto RelayFlagReportCustom = uint8_t{ 1 << 3 };

// Set from the timer, before changing status. Should skip setting any implicit delays
constexpr auto RelayFlagTimerDelay = uint8_t{ 1 << 4 };

// Set either from the timer, or the relay object. Should skip setting any implicit delays.
constexpr auto RelayFlagTimerPulse = uint8_t{ 1 << 5 };

constexpr auto RelayCommonStatusFlags = uint8_t{ RelayFlagReport | RelayFlagReportCustom };

using RelayMask = std::bitset<RelaysMax>;

struct RelayStatusMask {
    bool status;
    RelayMask value;
};

struct RelayMaskPair {
    RelayMask on;
    RelayMask off;
};

bool operator==(const RelayMaskPair& lhs, const RelayMaskPair& rhs) {
    return lhs.on == rhs.on
        && lhs.off == rhs.off;
}

RelayMask operator&(const RelayMaskPair& lhs, const RelayMaskPair& rhs) {
    return (lhs.on | lhs.off) & (rhs.on | rhs.off);
}

} // namespace

// nb. settings / convert should not use type alias for bitset<32>, make sure this is used instead
struct RelayMaskHelper {
    using unsigned_type = uint32_t;
    static_assert(sizeof(RelayMask) == sizeof(unsigned_type), "");

    RelayMask value;

    RelayMaskHelper() = default;
    explicit RelayMaskHelper(RelayMask value) :
        value(value)
    {}

    constexpr size_t size() const {
        return value.size();
    }

    unsigned_type toUnsigned() const {
        return value.to_ulong();
    }

    String toString() const {
        return formatUnsigned(toUnsigned(), 2);
    }

    RelayMask::reference operator[](size_t id) {
        return value[id];
    }

    bool operator[](size_t id) const {
        return value[id];
    }

    void reset() noexcept {
        value.reset();
    }

    template <typename T>
    static void for_each(RelayMask mask, T&& callback) {
        if (!mask.any()) {
            return;
        }

        for (auto tmp = mask; tmp.any();) {
            auto value = tmp.to_ulong();

            size_t bit = __builtin_ctz(value);
            callback(bit);

            tmp[bit] = false;
        }
    }

    template <typename T>
    void for_each(T&& callback) {
        for_each(value, std::forward<T>(callback));
    }
};

namespace {

size_t _relayCount();

bool _relayStatus(size_t id);
bool _relayTargetStatus(size_t id);
uint8_t _relayTargetFlags(size_t id);

bool _relayStatusChange(size_t id, bool status, uint8_t flags);
bool _relayStatus(size_t id, bool status);
bool _relayStatus(size_t id, bool status, uint8_t flags);

bool _relayToggle(size_t id, uint8_t flags);
bool _relayToggle(size_t id);

RelayLock _relayLock(size_t id);
void _relayLock(size_t id, RelayLock);
void _relayLockStatus(size_t id, RelayLock);
void _relayLockSync(const RelayMaskPair&);

RelayMaskPair _relaySyncPair(RelaySync, size_t, bool, size_t);

void _relayStatusPair(const RelayMaskPair& pair, uint8_t flags) {
    RelayMaskHelper::for_each(
        pair.off,
        [&](size_t id) {
            _relayStatus(id, false, flags);
        });
    RelayMaskHelper::for_each(
        pair.on,
        [&](size_t id) {
            _relayStatus(id, true, flags);
        });
}

} // namespace

namespace espurna {
namespace relay {
namespace settings {

STRING_VIEW_INLINE(Prefix, "relay");

} // namespace settings

namespace flood {

using Duration = espurna::duration::Milliseconds;
using Seconds = std::chrono::duration<float>;

namespace build {
namespace {

static constexpr auto Window = std::clamp(
    Seconds{ RELAY_FLOOD_WINDOW }, Seconds::zero(), Seconds::max());
static constexpr auto WindowNative =
    std::chrono::duration_cast<Duration>(Window);

static constexpr auto Changes = (unsigned char){ RELAY_FLOOD_CHANGES };

} // namespace
} // namespace build

namespace settings {
namespace keys {
namespace {

PROGMEM_STRING(Time, "relayFloodTime");
PROGMEM_STRING(Changes, "relayFloodChanges");

} // namespace
} // namespace keys

namespace {

Duration window() {
    return getSetting(keys::Time, build::WindowNative);
}

unsigned char changes() {
    return getSetting(keys::Changes, build::Changes);
}

} // namespace
} // namespace settings
} // namespace flood

namespace build {
namespace {

constexpr espurna::duration::Milliseconds saveDelay() {
    return espurna::duration::Milliseconds(RELAY_SAVE_DELAY);
}

constexpr size_t dummyCount() {
    return DUMMY_RELAY_COUNT;
}

constexpr size_t syncId() {
    return (RELAY_SYNC_ID) > 0
        ? (RELAY_SYNC_ID - 1)
        : RelaysMax;
}

constexpr RelaySync syncMode() {
    return RELAY_SYNC;
}

constexpr espurna::duration::Milliseconds latchingPulse() {
    return espurna::duration::Milliseconds(RELAY_LATCHING_PULSE);
}

constexpr espurna::duration::Milliseconds interlockDelay() {
    return espurna::duration::Milliseconds(RELAY_DELAY_INTERLOCK);
}

constexpr espurna::duration::Milliseconds delayOn() {
    return espurna::duration::Milliseconds{ RELAY_DELAY_ON };
}

constexpr espurna::duration::Milliseconds delayOn(size_t index) {
    return espurna::duration::Milliseconds{
        (index == 0) ? RELAY1_DELAY_ON :
        (index == 1) ? RELAY2_DELAY_ON :
        (index == 2) ? RELAY3_DELAY_ON :
        (index == 3) ? RELAY4_DELAY_ON :
        (index == 4) ? RELAY5_DELAY_ON :
        (index == 5) ? RELAY6_DELAY_ON :
        (index == 6) ? RELAY7_DELAY_ON :
        (index == 7) ? RELAY8_DELAY_ON : RELAY_DELAY_ON };
}

constexpr espurna::duration::Milliseconds delayOff() {
    return espurna::duration::Milliseconds{ RELAY_DELAY_OFF };
}

constexpr espurna::duration::Milliseconds delayOff(size_t index) {
    return espurna::duration::Milliseconds{
        (index == 0) ? RELAY1_DELAY_OFF :
        (index == 1) ? RELAY2_DELAY_OFF :
        (index == 2) ? RELAY3_DELAY_OFF :
        (index == 3) ? RELAY4_DELAY_OFF :
        (index == 4) ? RELAY5_DELAY_OFF :
        (index == 5) ? RELAY6_DELAY_OFF :
        (index == 6) ? RELAY7_DELAY_OFF :
        (index == 7) ? RELAY8_DELAY_OFF : RELAY_DELAY_OFF };
}

constexpr unsigned char pin(size_t index) {
    return (
        (index == 0) ? RELAY1_PIN :
        (index == 1) ? RELAY2_PIN :
        (index == 2) ? RELAY3_PIN :
        (index == 3) ? RELAY4_PIN :
        (index == 4) ? RELAY5_PIN :
        (index == 5) ? RELAY6_PIN :
        (index == 6) ? RELAY7_PIN :
        (index == 7) ? RELAY8_PIN : GPIO_NONE
    );
}

constexpr RelayType type(size_t index) {
    return (
        (index == 0) ? RELAY1_TYPE :
        (index == 1) ? RELAY2_TYPE :
        (index == 2) ? RELAY3_TYPE :
        (index == 3) ? RELAY4_TYPE :
        (index == 4) ? RELAY5_TYPE :
        (index == 5) ? RELAY6_TYPE :
        (index == 6) ? RELAY7_TYPE :
        (index == 7) ? RELAY8_TYPE : RELAY_TYPE_NORMAL
    );
}

constexpr GpioType pinType(size_t index) {
    return (
        (index == 0) ? RELAY1_PIN_TYPE :
        (index == 1) ? RELAY2_PIN_TYPE :
        (index == 2) ? RELAY3_PIN_TYPE :
        (index == 3) ? RELAY4_PIN_TYPE :
        (index == 4) ? RELAY5_PIN_TYPE :
        (index == 5) ? RELAY6_PIN_TYPE :
        (index == 6) ? RELAY7_PIN_TYPE :
        (index == 7) ? RELAY8_PIN_TYPE : GPIO_TYPE_NONE
    );
}

constexpr unsigned char resetPin(size_t index) {
    return (
        (index == 0) ? RELAY1_RESET_PIN :
        (index == 1) ? RELAY2_RESET_PIN :
        (index == 2) ? RELAY3_RESET_PIN :
        (index == 3) ? RELAY4_RESET_PIN :
        (index == 4) ? RELAY5_RESET_PIN :
        (index == 5) ? RELAY6_RESET_PIN :
        (index == 6) ? RELAY7_RESET_PIN :
        (index == 7) ? RELAY8_RESET_PIN : GPIO_NONE
    );
}

constexpr RelayBoot bootMode(size_t index) {
    return (
        (index == 0) ? RELAY1_BOOT_MODE :
        (index == 1) ? RELAY2_BOOT_MODE :
        (index == 2) ? RELAY3_BOOT_MODE :
        (index == 3) ? RELAY4_BOOT_MODE :
        (index == 4) ? RELAY5_BOOT_MODE :
        (index == 5) ? RELAY6_BOOT_MODE :
        (index == 6) ? RELAY7_BOOT_MODE :
        (index == 7) ? RELAY8_BOOT_MODE : RELAY_BOOT_OFF
    );
}

constexpr RelayProvider provider(size_t index) {
    return (
        (index == 0) ? (RELAY1_PROVIDER) :
        (index == 1) ? (RELAY2_PROVIDER) :
        (index == 2) ? (RELAY3_PROVIDER) :
        (index == 3) ? (RELAY4_PROVIDER) :
        (index == 4) ? (RELAY5_PROVIDER) :
        (index == 5) ? (RELAY6_PROVIDER) :
        (index == 6) ? (RELAY7_PROVIDER) :
        (index == 7) ? (RELAY8_PROVIDER) : RELAY_PROVIDER_NONE
    );
}

#if MQTT_SUPPORT || API_SUPPORT
PROGMEM_STRING(PayloadOn, RELAY_MQTT_ON);
PROGMEM_STRING(PayloadOff, RELAY_MQTT_OFF);
PROGMEM_STRING(PayloadToggle, RELAY_MQTT_TOGGLE);
#endif

#if MQTT_SUPPORT

constexpr RelayMqttTopicMode mqttTopicMode(size_t index) {
    return (
        (index == 0) ? (RELAY1_MQTT_TOPIC_MODE) :
        (index == 1) ? (RELAY2_MQTT_TOPIC_MODE) :
        (index == 2) ? (RELAY3_MQTT_TOPIC_MODE) :
        (index == 3) ? (RELAY4_MQTT_TOPIC_MODE) :
        (index == 4) ? (RELAY5_MQTT_TOPIC_MODE) :
        (index == 5) ? (RELAY6_MQTT_TOPIC_MODE) :
        (index == 6) ? (RELAY7_MQTT_TOPIC_MODE) :
        (index == 7) ? (RELAY8_MQTT_TOPIC_MODE) : RELAY_MQTT_TOPIC_MODE
    );
}

#define RELAY_SETTING_STRING_RESULT(FIRST, SECOND, THIRD, FOURTH, FIFTH, SIXTH, SEVENTH, EIGHTH)\
    (index == 0) ? STRING_VIEW_SETTING(FIRST) :\
    (index == 1) ? STRING_VIEW_SETTING(SECOND) :\
    (index == 2) ? STRING_VIEW_SETTING(THIRD) :\
    (index == 3) ? STRING_VIEW_SETTING(FOURTH) :\
    (index == 4) ? STRING_VIEW_SETTING(FIFTH) :\
    (index == 5) ? STRING_VIEW_SETTING(SIXTH) :\
    (index == 6) ? STRING_VIEW_SETTING(SEVENTH) :\
    (index == 7) ? STRING_VIEW_SETTING(EIGHTH) : StringView()

StringView mqttTopicSub(size_t index) {
    return RELAY_SETTING_STRING_RESULT(
        RELAY1_MQTT_TOPIC_SUB,
        RELAY2_MQTT_TOPIC_SUB,
        RELAY3_MQTT_TOPIC_SUB,
        RELAY4_MQTT_TOPIC_SUB,
        RELAY5_MQTT_TOPIC_SUB,
        RELAY6_MQTT_TOPIC_SUB,
        RELAY7_MQTT_TOPIC_SUB,
        RELAY8_MQTT_TOPIC_SUB
    );
}

StringView mqttTopicPub(size_t index) {
    return RELAY_SETTING_STRING_RESULT(
        RELAY1_MQTT_TOPIC_PUB,
        RELAY2_MQTT_TOPIC_PUB,
        RELAY3_MQTT_TOPIC_PUB,
        RELAY4_MQTT_TOPIC_PUB,
        RELAY5_MQTT_TOPIC_PUB,
        RELAY6_MQTT_TOPIC_PUB,
        RELAY7_MQTT_TOPIC_PUB,
        RELAY8_MQTT_TOPIC_PUB
    );
}

#undef RELAY_SETTING_STRING_RESULT

constexpr PayloadStatus mqttDisconnectionStatus(size_t index) {
    return (
        (index == 0) ? (RELAY1_MQTT_DISCONNECT_STATUS) :
        (index == 1) ? (RELAY2_MQTT_DISCONNECT_STATUS) :
        (index == 2) ? (RELAY3_MQTT_DISCONNECT_STATUS) :
        (index == 3) ? (RELAY4_MQTT_DISCONNECT_STATUS) :
        (index == 4) ? (RELAY5_MQTT_DISCONNECT_STATUS) :
        (index == 5) ? (RELAY6_MQTT_DISCONNECT_STATUS) :
        (index == 6) ? (RELAY7_MQTT_DISCONNECT_STATUS) :
        (index == 7) ? (RELAY8_MQTT_DISCONNECT_STATUS) : RELAY_MQTT_DISCONNECT_NONE
    );
}

constexpr duration::Seconds mqttDisconnectionDelay() {
    return RELAY_MQTT_DISCONNECT_DELAY;
}

#endif

} // namespace
} // namespace build
} // namespace relay
} // namespace espurna

namespace espurna {
namespace relay {
namespace timer {

using Duration = espurna::duration::Milliseconds;
using Seconds = std::chrono::duration<float>;

namespace settings {
namespace {

using DurationPair = espurna::duration::Pair;
using ParseResult = espurna::duration::PairResult;

Duration native_duration(DurationPair pair) {
    using namespace espurna::duration;
    return to_chrono<Duration>(pair);
}

Duration native_duration(ParseResult result) {
    return result.ok
        ? native_duration(result.value)
        : Duration::min();
}

ParseResult parse_time(StringView view) {
    using namespace espurna::duration;
    return parse(view, Seconds::period{});
}

Duration native_duration(StringView view) {
    return native_duration(parse_time(view));
}

} // namespace
} // namespace settings

namespace {

struct BulkTimer {
    using TimerImpl = ::espurna::timer::SystemTimer;

    using TimePoint = TimerImpl::TimeSource::time_point;
    using Duration = TimerImpl::Duration;

    BulkTimer() = delete;

    BulkTimer(const BulkTimer&) = delete;
    BulkTimer& operator=(const BulkTimer&) = delete;

    BulkTimer(BulkTimer&&) = default;
    BulkTimer& operator=(BulkTimer&&) = default;

    BulkTimer(RelayMaskPair pair, Duration duration, uint8_t flags) :
        _pair(pair),
        _duration(minimal_duration(duration)),
        _flags(flags)
    {}

    ~BulkTimer() {
        _timer.stop();
    }

    explicit operator bool() const {
        return static_cast<bool>(_timer);
    }

    bool operator==(const BulkTimer& other) const {
        return (_duration == other._duration)
            && (_pair == other._pair)
            && (_flags == other._flags);
    }

    BulkTimer& update(RelayMaskPair pair, Duration duration, uint8_t flags) {
        stop();
        _pair = pair;
        _duration = minimal_duration(duration);
        _flags = flags;
        _started = false;
        _start_time = TimePoint::min();
        return *this;
    }

    const RelayMask& on() const {
        return _pair.on;
    }

    const RelayMask& off() const {
        return _pair.off;
    }

    const RelayMaskPair& mask() const {
        return _pair;
    }

    bool finished() const {
        return _started && !_timer.armed();
    }

    bool started() const {
        return _started;
    }

    TimePoint start_time() const {
        return _start_time;
    }

    Duration duration() const {
        return _duration;
    }

    uint8_t flags() const {
        return _flags;
    }

    bool contains(size_t id) const {
        return _pair.on[id] || _pair.off[id];
    }

    bool contains(size_t id, bool status) const {
        if (status && _pair.on[id]) {
            return true;
        }

        if (!status && _pair.off[id]) {
            return true;
        }

        return false;
    }

    bool status(size_t id) const {
        if (_pair.on[id]) {
            return true;
        }

        return false;
    }

    void stop() {
        _timer.stop();
    }

    void start();

private:
    // CANNOT *NOT* run, TimerImpl is allowed to do nothing w/ zero duration
    static Duration minimal_duration(Duration duration) {
        return duration < TimerImpl::DurationMin
            ? TimerImpl::DurationMin
            : duration;
    }

    RelayMaskPair _pair;
    Duration _duration;

    uint8_t _flags{};
    bool _started{ false };

    TimePoint _start_time;
    TimerImpl _timer;
};

namespace internal {

std::forward_list<BulkTimer> timers;

} // namespace internal

using Iterator = decltype(internal::timers)::iterator;

// Always expect that relay ID is unique across all instances
template <typename T>
BulkTimer* find(T&& callback) {
    auto it = std::find_if(
        internal::timers.begin(),
        internal::timers.end(),
        std::forward<T>(callback));

    if (it != internal::timers.end()) {
        return std::addressof(*it);
    }

    return nullptr;
}

// ...first match is always the one we'd want to use
BulkTimer* find(size_t id) {
    return find([&](const BulkTimer& timer) {
        return timer.contains(id);
    });
}

// ...especially w/ masks containing multiple IDs
BulkTimer* find(RelayMaskPair pair) {
    return find([&](const BulkTimer& timer) {
        return (timer.mask() & pair).any();
    });
}

// ...which also requires deleting active timer whenever API accepts overlaping IDs
void cancel(size_t id) {
    internal::timers.remove_if(
        [&](const BulkTimer& timer) {
            return timer.contains(id);
        });
}

void cancel(const BulkTimer& timer) {
    internal::timers.remove(timer);
}

bool is_pulse(const BulkTimer& timer) {
    return (timer.flags() & RelayFlagTimerPulse) > 0;
}

// PULSE timer instance lingers until relay status is processed
BulkTimer* schedule(RelayMaskPair pair, Duration duration, uint8_t flags) {
    auto it = find(pair);
    if (it) {
        (*it).update(pair, duration, flags);
    } else {
        internal::timers.emplace_front(pair, duration, flags);
        it = std::addressof(internal::timers.front());
    }

    return it;
}

BulkTimer* schedule(size_t id, Duration duration, bool target, uint8_t flags) {
    RelayMaskPair pair;

    auto& mask = target ? pair.on : pair.off;
    mask[id] = true;

    return schedule(pair, duration, flags);
}

// DELAY timer instance does not wait and starts immediately
BulkTimer* schedule_and_start(RelayMaskPair pair, Duration duration, uint8_t flags) {
    auto it = schedule(pair, duration, flags);
    (*it).start();

    return it;
}

BulkTimer* schedule_and_start(size_t id, Duration duration, bool target, uint8_t flags) {
    auto it = schedule(id, duration, target, flags);
    (*it).start();

    return it;
}

void BulkTimer::start() {
    const auto pair = _pair;
    const auto flags = _flags;

    _start_time = TimePoint::clock::now();
    _started = true;
    _timer.once(
        _duration,
        [pair, flags]() {
            _relayStatusPair(pair, flags);
        });
}

} // namespace
} // namespace timer

namespace pulse {

enum class Mode {
    None,
    Off,
    On,
};

namespace {
namespace build {

constexpr timer::Seconds time(size_t index) {
    return timer::Seconds(
        (index == 0) ? RELAY1_PULSE_TIME :
        (index == 1) ? RELAY2_PULSE_TIME :
        (index == 2) ? RELAY3_PULSE_TIME :
        (index == 3) ? RELAY4_PULSE_TIME :
        (index == 4) ? RELAY5_PULSE_TIME :
        (index == 5) ? RELAY6_PULSE_TIME :
        (index == 6) ? RELAY7_PULSE_TIME :
        (index == 7) ? RELAY8_PULSE_TIME : RELAY_PULSE_TIME
    );
}

static_assert(time(0) >= timer::Seconds::zero(), "");
static_assert(time(1) >= timer::Seconds::zero(), "");
static_assert(time(2) >= timer::Seconds::zero(), "");
static_assert(time(3) >= timer::Seconds::zero(), "");
static_assert(time(4) >= timer::Seconds::zero(), "");
static_assert(time(5) >= timer::Seconds::zero(), "");
static_assert(time(6) >= timer::Seconds::zero(), "");
static_assert(time(7) >= timer::Seconds::zero(), "");

constexpr Mode mode(size_t index) {
    return (
        (index == 0) ? RELAY1_PULSE_MODE :
        (index == 1) ? RELAY2_PULSE_MODE :
        (index == 2) ? RELAY3_PULSE_MODE :
        (index == 3) ? RELAY4_PULSE_MODE :
        (index == 4) ? RELAY5_PULSE_MODE :
        (index == 5) ? RELAY6_PULSE_MODE :
        (index == 6) ? RELAY7_PULSE_MODE :
        (index == 7) ? RELAY8_PULSE_MODE : RELAY_PULSE_NONE
    );
}

} // namespace build

namespace settings {
namespace keys {

PROGMEM_STRING(Time, "relayTime");
PROGMEM_STRING(Mode, "relayPulse");

} // namespace keys

timer::Duration time(size_t index) {
    const auto time = espurna::settings::get(
        espurna::settings::Key{keys::Time, index}.value());

    if (!time) {
        return std::chrono::duration_cast<timer::Duration>(build::time(index));
    }

    return timer::settings::native_duration(time.view());
}

Mode mode(size_t index) {
    return getSetting({keys::Mode, index}, build::mode(index));
}

} // namespace settings

bool isNormalStatus(Mode pulse, bool status) {
    switch (pulse) {
    case Mode::None:
        break;
    case Mode::On:
        return status;
    case Mode::Off:
        return !status;
    }

    return false;
}

bool isActive(Mode mode) {
    return mode != Mode::None;
}

bool wouldChange(Mode mode, bool status) {
    return isActive(mode) && !isNormalStatus(mode, status);
}

} // namespace
} // namespace pulse

namespace settings {
namespace options {
namespace {

using espurna::settings::options::Enumeration;

PROGMEM_STRING(TristateNone, "none");
PROGMEM_STRING(TristateOff, "off");
PROGMEM_STRING(TristateOn, "on");

template <typename T>
struct RelayTristateHelper {
    static constexpr std::array<Enumeration<T>, 3> Options PROGMEM {
        {{T::None, TristateNone},
         {T::Off, TristateOff},
         {T::On, TristateOn}}
    };

    static T convert(const String& value) {
        return espurna::settings::internal::convert(Options, value, T::None);
    }

    static String serialize(T value) {
        return espurna::settings::internal::serialize(Options, value);
    }
};

template <typename T>
constexpr std::array<Enumeration<T>, 3> RelayTristateHelper<T>::Options;

PROGMEM_STRING(PayloadStatusOff, "off");
PROGMEM_STRING(PayloadStatusOn, "on");
PROGMEM_STRING(PayloadStatusToggle, "toggle");
PROGMEM_STRING(PayloadStatusUnknown, "unknown");

static constexpr std::array<Enumeration<PayloadStatus>, 4> PayloadStatusOptions PROGMEM {
    {{PayloadStatus::Off, PayloadStatusOff},
     {PayloadStatus::On, PayloadStatusOn},
     {PayloadStatus::Toggle, PayloadStatusToggle},
     {PayloadStatus::Unknown, PayloadStatusUnknown}}
};

PROGMEM_STRING(Normal, "normal");
PROGMEM_STRING(Inverse, "inverse");

static constexpr std::array<Enumeration<RelayMqttTopicMode>, 2> RelayMqttTopicModeOptions PROGMEM {
    {{RelayMqttTopicMode::Normal, Normal},
     {RelayMqttTopicMode::Inverse, Inverse}}
};

PROGMEM_STRING(RelayBootOff, "off");
PROGMEM_STRING(RelayBootOn, "on");
PROGMEM_STRING(RelayBootSame, "same");
PROGMEM_STRING(RelayBootToggle, "toggle");
PROGMEM_STRING(RelayBootLockedOff, "locked-off");
PROGMEM_STRING(RelayBootLockedOn, "locked-on");

static constexpr std::array<Enumeration<RelayBoot>, 6> RelayBootOptions PROGMEM {
    {{RelayBoot::Off, RelayBootOff},
     {RelayBoot::On, RelayBootOn},
     {RelayBoot::Same, RelayBootSame},
     {RelayBoot::Toggle, RelayBootToggle},
     {RelayBoot::LockedOff, RelayBootLockedOff},
     {RelayBoot::LockedOn, RelayBootLockedOn}}
};

PROGMEM_STRING(RelayProviderNone, "none");
PROGMEM_STRING(RelayProviderDummy, "dummy");
PROGMEM_STRING(RelayProviderGpio, "gpio");
PROGMEM_STRING(RelayProviderDual, "dual");
PROGMEM_STRING(RelayProviderStm, "stm");
PROGMEM_STRING(RelayProviderLightState, "light-state");
PROGMEM_STRING(RelayProviderFan, "fan");
PROGMEM_STRING(RelayProviderLightfox, "lightfox");
PROGMEM_STRING(RelayProviderTuya, "tuya");

static constexpr std::array<Enumeration<RelayProvider>, 9> RelayProviderOptions PROGMEM {
    {{RelayProvider::None, RelayProviderNone},
    {RelayProvider::Dummy, RelayProviderDummy},
    {RelayProvider::Gpio, RelayProviderGpio},
    {RelayProvider::Dual, RelayProviderDual},
    {RelayProvider::Stm, RelayProviderStm},
    {RelayProvider::LightState, RelayProviderLightState},
    {RelayProvider::Fan, RelayProviderFan},
    {RelayProvider::Lightfox, RelayProviderLightfox},
    {RelayProvider::Tuya, RelayProviderTuya}}
};

PROGMEM_STRING(RelayTypeNormal, "normal");
PROGMEM_STRING(RelayTypeInverse, "inverse");
PROGMEM_STRING(RelayTypeLatched, "latched");
PROGMEM_STRING(RelayTypeLatchedInverse, "latched-inverse");

static constexpr std::array<Enumeration<RelayType>, 4> RelayTypeOptions PROGMEM {
    {{RelayType::Normal, RelayTypeNormal},
     {RelayType::Inverse, RelayTypeInverse},
     {RelayType::Latched, RelayTypeLatched},
     {RelayType::LatchedInverse, RelayTypeLatchedInverse}}
};

PROGMEM_STRING(None, "none");
PROGMEM_STRING(ZeroOrOne, "zero-or-one");
PROGMEM_STRING(JustOne, "just-one");
PROGMEM_STRING(All, "all");
PROGMEM_STRING(First, "first");

static constexpr std::array<Enumeration<RelaySync>, 5> RelaySyncOptions PROGMEM {
    {{RelaySync::None, None},
     {RelaySync::ZeroOrOne, ZeroOrOne},
     {RelaySync::JustOne, JustOne},
     {RelaySync::All, All},
     {RelaySync::First, First}}
};

} // namespace
} // namespace options
} // namespace settings
} // namespace relay
} // namespace espurna

namespace espurna {
namespace settings {
namespace internal {
namespace {

using relay::settings::options::RelayTristateHelper;
using relay::settings::options::PayloadStatusOptions;
using relay::settings::options::RelayMqttTopicModeOptions;
using relay::settings::options::RelayBootOptions;
using relay::settings::options::RelayProviderOptions;
using relay::settings::options::RelayTypeOptions;
using relay::settings::options::RelaySyncOptions;

} // namespace

template <>
PayloadStatus convert(const String& value) {
    return convert(PayloadStatusOptions, value, PayloadStatus::Unknown);
}

String serialize(PayloadStatus value) {
    return serialize(PayloadStatusOptions, value);
}

template <>
RelayMqttTopicMode convert(const String& value) {
    return convert(RelayMqttTopicModeOptions, value, RelayMqttTopicMode::Normal);
}

String serialize(RelayMqttTopicMode value) {
    return serialize(RelayMqttTopicModeOptions, value);
}

template <>
espurna::relay::pulse::Mode convert(const String& value) {
    return RelayTristateHelper<espurna::relay::pulse::Mode>::convert(value);
}

String serialize(espurna::relay::pulse::Mode value) {
    return RelayTristateHelper<espurna::relay::pulse::Mode>::serialize(value);
}

template <>
RelayBoot convert(const String& value) {
    return convert(RelayBootOptions, value, RelayBoot::Off);
}

String serialize(RelayBoot value) {
    return serialize(RelayBootOptions, value);
}

template <>
RelayLock convert(const String& value) {
    return RelayTristateHelper<RelayLock>::convert(value);
}

template <>
RelayProvider convert(const String& value) {
    return convert(RelayProviderOptions, value, RelayProvider::None);
}

String serialize(RelayProvider value) {
    return serialize(RelayProviderOptions, value);
}

template <>
RelayType convert(const String& value) {
    return convert(RelayTypeOptions, value, RelayType::Normal);
}

String serialize(RelayType value) {
    return serialize(RelayTypeOptions, value);
}

template<>
RelayMaskHelper convert(const String& value) {
    RelayMaskHelper out;
    out.value = convert<RelayMaskHelper::unsigned_type>(value);
    return out;
}

String serialize(RelayMaskHelper mask) {
    return mask.toString();
}

template <>
RelaySync convert(const String& value) {
    return convert(RelaySyncOptions, value, RelaySync::None);
}

String serialize(RelaySync value) {
    return serialize(RelaySyncOptions, value);
}

} // namespace internal
} // namespace settings

namespace relay {
namespace settings {
namespace keys {
namespace {

PROGMEM_STRING(Name, "relayName");
PROGMEM_STRING(Provider, "relayProv");
PROGMEM_STRING(Type, "relayType");
PROGMEM_STRING(GpioType, "relayGpioType");
PROGMEM_STRING(Gpio, "relayGpio");
PROGMEM_STRING(ResetGpio, "relayResetGpio");
PROGMEM_STRING(Boot, "relayBoot");
PROGMEM_STRING(DelayOn, "relayDelayOn");
PROGMEM_STRING(DelayOff, "relayDelayOff");

#if MQTT_SUPPORT
PROGMEM_STRING(TopicPub, "relayTopicPub");
PROGMEM_STRING(TopicSub, "relayTopicSub");
PROGMEM_STRING(TopicMode, "relayTopicMode");
PROGMEM_STRING(MqttDelay, "relayMqttDelay");
PROGMEM_STRING(MqttStatus, "relayMqttDisc");
#endif

PROGMEM_STRING(Dummy, "relayDummy");
PROGMEM_STRING(BootMask, "relayBootMask");
PROGMEM_STRING(Interlock, "relayIlkDelay");
PROGMEM_STRING(Sync, "relaySync");
PROGMEM_STRING(SyncId, "relaySyncId");

#if MQTT_SUPPORT || API_SUPPORT
PROGMEM_STRING(PayloadOn, "relayPayloadOn");
PROGMEM_STRING(PayloadOff, "relayPayloadOff");
PROGMEM_STRING(PayloadToggle, "relayPayloadToggle");
#endif

} // namespace
} // namespace keys

namespace {

size_t dummyCount() {
    return getSetting(keys::Dummy, build::dummyCount());
}

[[gnu::unused]]
String name(size_t index) {
    return getSetting({keys::Name, index});
}

RelayProvider provider(size_t index) {
    return getSetting({keys::Provider, index}, build::provider(index));
}

RelayType type(size_t index) {
    return getSetting({keys::Type, index}, build::type(index));
}

GpioType pinType(size_t index) {
    return getSetting({keys::GpioType, index}, build::pinType(index));
}

unsigned char pin(size_t index) {
    return getSetting({keys::Gpio, index}, build::pin(index));
}

unsigned char resetPin(size_t index) {
    return getSetting({keys::ResetGpio, index}, build::resetPin(index));
}

RelayBoot bootMode(size_t index) {
    return getSetting({keys::Boot, index}, build::bootMode(index));
}

RelayMaskHelper bootMask() {
    return getSetting(keys::BootMask, RelayMaskHelper{});
}

void bootMask(const String& mask) {
    setSetting(keys::BootMask, mask);
}

void bootMask(const RelayMaskHelper& mask) {
    bootMask(mask.toString());
}

espurna::duration::Milliseconds delayOn() {
    return getSetting(keys::DelayOn, build::delayOn());
}

espurna::duration::Milliseconds delayOn(size_t index) {
    return getSetting({keys::DelayOn, index}, build::delayOn(index));
}

espurna::duration::Milliseconds delayOff() {
    return getSetting(keys::DelayOff, build::delayOff());
}

espurna::duration::Milliseconds delayOff(size_t index) {
    return getSetting({keys::DelayOff, index}, build::delayOff(index));
}

espurna::duration::Milliseconds interlockDelay() {
    return getSetting(keys::Interlock, build::interlockDelay());
}

size_t syncId() {
    return getSetting(keys::SyncId, build::syncId());
}

RelaySync syncMode() {
    return getSetting(keys::Sync, build::syncMode());
}

#if MQTT_SUPPORT || API_SUPPORT
String payloadOn() {
    return getSetting(keys::PayloadOn, StringView(build::PayloadOn));
}

String payloadOff() {
    return getSetting(keys::PayloadOff, StringView(build::PayloadOff));
}

String payloadToggle() {
    return getSetting(keys::PayloadToggle, StringView(build::PayloadToggle));
}
#endif

#if MQTT_SUPPORT
String mqttTopicSub(size_t index) {
    return getSetting({keys::TopicSub, index}, build::mqttTopicSub(index));
}

String mqttTopicPub(size_t index) {
    return getSetting({keys::TopicPub, index}, build::mqttTopicPub(index));
}

RelayMqttTopicMode mqttTopicMode(size_t index) {
    return getSetting({keys::TopicMode, index}, build::mqttTopicMode(index));
}

duration::Seconds mqttDisconnectionDelay() {
    return getSetting(keys::MqttDelay, build::mqttDisconnectionDelay());
}

PayloadStatus mqttDisconnectionStatus(size_t index) {
    return getSetting({keys::MqttStatus, index}, build::mqttDisconnectionStatus(index));
}

#endif

} // namespace

namespace query {
namespace {

#define EXACT_VALUE(NAME, FUNC)\
String NAME () {\
    return espurna::settings::internal::serialize(FUNC());\
}

#define ID_VALUE(NAME, FUNC)\
String NAME (size_t id) {\
    return espurna::settings::internal::serialize(FUNC(id));\
}

namespace internal {

EXACT_VALUE(dummyCount, settings::dummyCount)
EXACT_VALUE(bootMask, settings::bootMask)
EXACT_VALUE(interlockDelay, settings::interlockDelay)
EXACT_VALUE(syncMode, settings::syncMode)
EXACT_VALUE(syncId, settings::syncId)
EXACT_VALUE(delayOn, settings::delayOn)
EXACT_VALUE(delayOff, settings::delayOff)

ID_VALUE(provider, settings::provider)
ID_VALUE(type, settings::type)
ID_VALUE(pinType, settings::pinType)
ID_VALUE(pin, settings::pin)
ID_VALUE(resetPin, settings::resetPin)
ID_VALUE(bootMode, settings::bootMode)
ID_VALUE(delayOn, settings::delayOn)
ID_VALUE(delayOff, settings::delayOff)

ID_VALUE(pulseMode, pulse::settings::mode)
String pulseTime(size_t index) {
    const auto result = pulse::settings::time(index);
    const auto as_seconds =
        std::chrono::duration_cast<timer::Seconds>(result);

    return espurna::settings::internal::serialize(as_seconds.count());
}

#if MQTT_SUPPORT
EXACT_VALUE(mqttDisconnectionDelay, settings::mqttDisconnectionDelay)
ID_VALUE(mqttDisconnectionStatus, settings::mqttDisconnectionStatus)
ID_VALUE(mqttTopicMode, settings::mqttTopicMode)
#endif

#undef ID_VALUE
#undef EXACT_VALUE

} // namespace internal

static constexpr espurna::settings::query::Setting Settings[] PROGMEM {
    {keys::BootMask, internal::bootMask},
    {keys::Dummy, internal::dummyCount},
    {keys::DelayOn, internal::delayOn},
    {keys::DelayOff, internal::delayOff},
    {keys::Interlock, internal::interlockDelay},
#if MQTT_SUPPORT
    {keys::MqttDelay, internal::mqttDisconnectionDelay},
#endif
    {keys::Sync, internal::syncMode},
    {keys::SyncId, internal::syncId},
};

static constexpr espurna::settings::query::IndexedSetting IndexedSettings[] PROGMEM {
    {keys::Name, settings::name},
    {keys::Provider, internal::provider},
    {keys::Type, internal::type},
    {keys::GpioType, internal::pinType},
    {keys::Gpio, internal::pin},
    {keys::ResetGpio, internal::resetPin},
    {keys::Boot, internal::bootMode},
    {keys::DelayOn, internal::delayOn},
    {keys::DelayOff, internal::delayOff},
    {pulse::settings::keys::Time, internal::pulseTime},
    {pulse::settings::keys::Mode, internal::pulseMode},
#if MQTT_SUPPORT
    {keys::TopicPub, settings::mqttTopicPub},
    {keys::TopicSub, settings::mqttTopicSub},
    {keys::TopicMode, internal::mqttTopicMode},
    {keys::MqttStatus, internal::mqttDisconnectionStatus},
#endif
};

} // namespace
} // namespace query
} // namespace settings
} // namespace relay
} // namespace espurna

// -----------------------------------------------------------------------------
// RELAY CONTROL
// -----------------------------------------------------------------------------

// No-op provider, available for purely virtual relays that are controlled only via API

struct DummyProvider : public RelayProviderBase {
    espurna::StringView id() const override {
        return espurna::relay::settings::options::RelayProviderDummy;
    }

    void change(bool) override {
    }

    static RelayProviderBase* sharedInstance() {
        static DummyProvider provider;
        return &provider;
    }
};

class Relay {
public:
    using TimeSource = espurna::time::CoreClock;
    using Delay = espurna::relay::timer::Duration;
    using TimePoint = TimeSource::time_point;
    using PulseMode = espurna::relay::pulse::Mode;

    Relay() = default;

    explicit Relay(RelayProviderBasePtr&& ptr) :
        provider(ptr.release())
    {}

    explicit Relay(RelayProviderBase* ptr) :
        provider(ptr)
    {}

    // Defaults to 'DummyProvider', since we don't want to check provider existence every time
    RelayProviderBase* provider { DummyProvider::sharedInstance() };

    // *Before* changing status, waits for the specified time (ms)
    Delay delay_on { Delay::zero() };
    Delay delay_off { Delay::zero() };

    // *After* changing status, checks whether we should remain in it
    // If not, starts a timer for the specified time (ms)
    PulseMode pulse { PulseMode::None };
    Delay pulse_time { Delay::zero() };

    // Flood window start time
    TimePoint fw_start{};

    // Number of changes within the current flood window
    unsigned char fw_count { 0u };

    // Applied status, latest status passed to the provider implementation
    bool current_status { false };

    // Pending status, waiting to be applied
    bool target_status { false };

    // Holds the value of target status that persists and cannot be changed from
    RelayLock lock { RelayLock::None };

    // Various implementation-dependant flags, see Flag* values
    uint8_t flags{};
};

namespace {

struct RelaySaveTimer {
    using Timer = ::espurna::timer::SystemTimer;
    using Duration = Timer::Duration;

    RelaySaveTimer() = default;

    RelaySaveTimer(const RelaySaveTimer&) = delete;
    RelaySaveTimer& operator=(const RelaySaveTimer&) = delete;

    RelaySaveTimer(RelaySaveTimer&&) = delete;
    RelaySaveTimer& operator=(RelaySaveTimer&&) = delete;

    ~RelaySaveTimer() {
        _timer.stop();
    }

    void schedule(Duration duration) {
        _timer.once(
            duration,
            [&]() {
                _ready = true;
            });
    }

    void stop() {
        _timer.stop();
        _persist = false;
    }

    template <typename T>
    void process(T&& callback) {
        if (_ready) {
            callback(_persist);
            _persist = false;
            _ready = false;
        }
    }

    void persist() {
        if (!_persist) {
            _persist = true;
        }
    }

private:
    bool _persist { false };
    bool _ready { false };
    Timer _timer;
};

struct RelayDelayedTimer {
    using Timer = espurna::timer::SystemTimer;
    using Duration = Timer::Duration;
    using Callback = espurna::Callback;

    RelayDelayedTimer() = default;

    RelayDelayedTimer(const RelayDelayedTimer&) = delete;
    RelayDelayedTimer& operator=(const RelayDelayedTimer&) = delete;

    RelayDelayedTimer(RelayDelayedTimer&&) = delete;
    RelayDelayedTimer& operator=(RelayDelayedTimer&&) = delete;

    ~RelayDelayedTimer() {
        stop();
    }

    bool scheduled() const {
        return static_cast<bool>(_timer);
    }

    bool prepared() const {
        return !_callback.isEmpty();
    }

    void prepare(Callback callback) {
        _callback = std::move(callback);
    }

    void schedule(Duration duration) {
        if (_callback.isEmpty()) {
            return;
        }

        if (duration != Duration::zero()) {
            _timer.once(
                duration,
                [&]() {
                    _ready = true;
                });
        } else {
            _ready = true;
        }
    }

    void stop() {
        _timer.stop();
        _ready = false;
    }

    void process() {
        if (_ready) {
            _callback();
            _ready = false;
        }
    }

private:
    bool _ready { false };
    Callback _callback;
    Timer _timer;
};

using Relays = std::vector<Relay>;
Relays _relays;

size_t _relayCount() {
    return _relays.size();
}

RelayMaskHelper _relayMaskCurrent(size_t relays) {
    RelayMaskHelper out;

    for (size_t id = 0; id < relays; ++id) {
        out.value[id] = _relays[id].current_status;
    }

    return out;
}

RelayMask _relays_boot{};
RelayMask _relays_ready{};
RelayMask _relays_retained{};
size_t _relays_dummy{};

Relay::Delay _relay_flood_window { espurna::relay::flood::build::WindowNative };
unsigned char _relay_flood_changes { espurna::relay::flood::build::Changes };

Relay::Delay _relay_delay_interlock;
Relay::Delay _relay_delay_off;
Relay::Delay _relay_delay_on;

RelaySync _relay_sync_mode { RelaySync::None };
size_t _relay_sync_id = RelaysMax;

struct RelaySyncUnlock {
    RelaySyncUnlock() {
        reset();
    }

    template <typename T>
    void save(T&& relays, const RelayMaskPair& pair) {
        if (!_saved) {
            RelayMaskHelper::for_each(
                pair.on,
                [&](size_t id) {
                    _locks[id] = _relays[id].lock;
                    _relays[id].lock = RelayLock::On;
                });
            RelayMaskHelper::for_each(
                pair.off,
                [&](size_t id) {
                    _locks[id] = _relays[id].lock;
                    _relays[id].lock = RelayLock::Off;
                });
            _saved = true;
        }
    }

    template <typename T>
    void restore(T&& relays) {
        if (_saved) {
            for (size_t id = 0; id < _relays.size(); ++id) {
                relays[id].lock = _locks[id];
            }
            reset();
        }
    }

    void reset() {
        std::fill(_locks.begin(), _locks.end(), RelayLock::None);
        _saved = false;
    }

    bool saved() const {
        return _saved;
    }

private:
    std::array<RelayLock, RelaysMax> _locks;
    bool _saved{ false };
};

RelaySyncUnlock _relay_sync_unlock;

RelayDelayedTimer _relay_unlock_timer;
RelaySaveTimer _relay_save_timer;

#if WEB_SUPPORT

bool _relay_report_ws { false };

void _relayScheduleWebSocketReport() {
    _relay_report_ws = true;
}

#endif // WEB_SUPPORT

#if MQTT_SUPPORT || API_SUPPORT

String _relay_payload_on;
String _relay_payload_off;
String _relay_payload_toggle;

#endif // MQTT_SUPPORT || API_SUPPORT

} // namespace

// -----------------------------------------------------------------------------
// RELAY PROVIDERS
// -----------------------------------------------------------------------------

// 'anchor' default virtual implementations to the relay.cpp.o

RelayProviderBase::~RelayProviderBase() = default;

bool RelayProviderBase::setup() {
    return true;
}

void RelayProviderBase::boot(bool) {
}

void RelayProviderBase::notify(bool) {
}

// Direct status notifications

namespace {

using RelayStatusCallbacks = std::forward_list<RelayStatusCallback>;

RelayStatusCallbacks _relays_on_status_notify;
RelayStatusCallbacks _relays_on_status_change;
RelayStatusCallbacks _relays_on_ready;

void _relayNotifyReadyStatus(RelayStatusCallback callback) {
    for (size_t index = 0; index < _relays.size(); ++index) {
        if (_relays_ready[index]) {
            callback(index, _relays[index].current_status);
        }
    }
}

} // namespace

void relayOnStatusNotify(RelayStatusCallback callback) {
    _relays_on_status_notify.push_front(callback);
}

void relayOnStatusChange(RelayStatusCallback callback) {
    _relays_on_status_change.push_front(callback);
}

void relayOnReady(RelayStatusCallback callback) {
    _relays_on_ready.push_front(callback);
    _relayNotifyReadyStatus(callback);
}

namespace {

// Real GPIO provider, using BasePin interface to implement writers
struct GpioProvider : public RelayProviderBase {
    GpioProvider(RelayType type, std::unique_ptr<BasePin>&& pin, std::unique_ptr<BasePin>&& reset_pin) :
        _type(type),
        _pin(std::move(pin)),
        _reset_pin(std::move(reset_pin))
    {}

    espurna::StringView id() const override {
        return espurna::relay::settings::options::RelayProviderGpio;
    }

    bool setup() override {
        if (!_pin) {
            return false;
        }

        _pin->pinMode(OUTPUT);
        if (_reset_pin) {
            _reset_pin->pinMode(OUTPUT);
        }

        return true;
    }

    void change(bool status) override {
        switch (_type) {
        case RelayType::Normal:
            _pin->digitalWrite(status);
            break;
        case RelayType::Inverse:
            _pin->digitalWrite(!status);
            break;
        case RelayType::Latched:
        case RelayType::LatchedInverse: {
            bool pulse = (_type == RelayType::Latched) ? HIGH : LOW;
            _pin->digitalWrite(!pulse);
            if (_reset_pin) {
                _reset_pin->digitalWrite(!pulse);
            }
            if (status || (!_reset_pin)) {
                _pin->digitalWrite(pulse);
            } else {
                _reset_pin->digitalWrite(pulse);
            }

            // notice that this stalls loop() execution, since
            // we need to ensure only relay task is active
            espurna::time::blockingDelay(espurna::relay::build::latchingPulse());

            _pin->digitalWrite(!pulse);
            if (_reset_pin) {
                _reset_pin->digitalWrite(!pulse);
            }
        }
        }
    }

private:
    RelayType _type { RelayType::Normal };
    std::unique_ptr<BasePin> _pin;
    std::unique_ptr<BasePin> _reset_pin;
};

#if RELAY_PROVIDER_DUAL_SUPPORT

// Special provider for Sonoff Dual, using serial protocol
class DualProvider : public RelayProviderBase {
public:
    DualProvider() = delete;
    explicit DualProvider(size_t id) : _id(id) {
        _instances.push_back(this);
        if (_instances.size() > Mask{}.size()) {
            DEBUG_MSG_P(PSTR("[RELAY] DUAL instances limit reached (%zu)\n"),
                Mask{}.size());
        }
    }

    ~DualProvider() {
        _instances.erase(
            std::remove(_instances.begin(), _instances.end(), this),
            _instances.end());
    }

    espurna::StringView id() const override {
        return espurna::relay::settings::options::RelayProviderDual;
    }

    bool setup() override {
        static bool once = ([]() {
            const auto port = uartPort(RELAY_PROVIDER_DUAL_PORT - 1);
            if (port) {
                DualProvider::_port = port->stream;
                espurnaRegisterLoop(loop);
                return true;
            }

            return false;
        })();

        return once;
    }

    void change(bool) override {
        espurnaRegisterOnceUnique(flush);
    }

    size_t relayId() const {
        return _id;
    }

    static std::vector<DualProvider*>& instances() {
        return _instances;
    }

    // Porting the old masking code from buttons
    // (no guarantee that this actually works, based on hearsay and some 3rd-party code)
    // | first | second |  mask |
    // |  OFF  |  OFF   |  0x0  |
    // |  ON   |  OFF   |  0x1  |
    // |  OFF  |  ON    |  0x2  |
    // |  ON   |  ON    |  0x3  |
    // i.e. set status bit mask[INSTANCE] for each relay
    // unless everything is ON, then *only* send mask[SIZE] bit and erase the rest

    static void flush() {
        bool sync { true };
        Mask mask;

        for (size_t index = 0; (index < _instances.size()) && (index < mask.size()); ++index) {
            const auto status = _relayStatus(_instances[index]->relayId());
            sync = sync && status;
            mask[index] = status;
        }

        static_assert(mask.size() > 0, "");
        if (sync) {
            mask.reset();
            mask[std::min(_instances.size(), mask.size() - 1)] = true;
        }

        DEBUG_MSG_P(PSTR("[RELAY] Sending DUAL mask: %s\n"),
            RelayMaskHelper(mask.to_ulong()).toString().c_str());

        uint8_t value = static_cast<uint8_t>(mask.to_ulong());
        uint8_t buffer[4] { 0xa0, 0x04, value, 0xa1 };
        _port->write(buffer, sizeof(buffer));
        _port->flush();
    }

    static void loop() {
        if (_port->available() < 4) {
            return;
        }

        unsigned char bytes[4] = {0};
        _port->readBytes(bytes, 4);
        if ((bytes[0] != 0xA0) && (bytes[1] != 0x04) && (bytes[3] != 0xA1)) {
            return;
        }

        // RELAYs and BUTTONs are synchonized in the SIL F330
        // Make sure we handle SYNC action first
        Mask mask(bytes[2]);
        const auto sync_index =
            _instances.size() < mask.size()
                ? _instances.size()
                : mask.size() - 1;

        RelayMaskPair pair;
        uint8_t flags = RelayCommonStatusFlags;

        if (mask[sync_index]) {
            for (auto& instance : _instances) {
                pair.on[instance->relayId()] = true;
            }
            flags |= RelayFlagSync;

        // Then, manage relays individually
        } else {
            for (size_t index = 0; (index < _instances.size()) && (index < mask.size()); ++index) {
                auto& pair_mask = mask[index] ? pair.on : pair.off;
                pair_mask[_instances[index]->relayId()] = true;
            }
        }

        _relayStatusPair(pair, flags);
    }

private:
    using Mask = std::bitset<8>;
    size_t _id;

    static std::vector<DualProvider*> _instances;
    static Stream* _port;
};

std::vector<DualProvider*> DualProvider::_instances;
Stream* DualProvider::_port = nullptr;

#endif // RELAY_PROVIDER_DUAL_SUPPORT

#if RELAY_PROVIDER_STM_SUPPORT

// Special provider for ESP01-relays with STM co-MCU driving the relays
class StmProvider : public RelayProviderBase {
public:
    StmProvider() = delete;
    explicit StmProvider(size_t id) :
        _id(id)
    {}

    espurna::StringView id() const override {
        return espurna::relay::settings::options::RelayProviderStm;
    }


    bool setup() override {
        static bool once = ([]() {
            const auto port = uartPort(RELAY_PROVIDER_STM_PORT - 1);
            if (port) {
                StmProvider::_port = port->stream;
                espurnaRegisterLoop(loop);
                return true;
            }

            return false;
        })();

        return once;
    }

    void change(bool status) override {
        if (_port) {
            _port->flush();
            _port->write(0xA0);
            _port->write(_id + 1);
            _port->write(status);
            _port->write(0xA1 + status + _id);

            // TODO: is this really solved via interlock delay, so we don't have to switch contexts here?
            //delay(100);

            _port->flush();
        }
    }

private:
    size_t _id;
    static Stream* _port;
};

Stream* StmProvider::_port = nullptr;

#endif // RELAY_PROVIDER_STM_SUPPORT

// -----------------------------------------------------------------------------
// UTILITY
// -----------------------------------------------------------------------------

bool _relayTryParseId(espurna::StringView value, size_t& id) {
    return tryParseId(value, _relayCount(), id);
}

[[gnu::unused]]
bool _relayTryParseIdFromPath(espurna::StringView value, size_t& id) {
    return tryParseIdPath(value, _relayCount(), id);
}

void _relayHandleStatus(size_t id, PayloadStatus status, uint8_t flags) {
    switch (status) {
    case PayloadStatus::Off:
        _relayStatus(id, false, flags);
        break;

    case PayloadStatus::On:
        _relayStatus(id, true, flags);
        break;

    case PayloadStatus::Toggle:
        _relayToggle(id, flags);
        break;

    case PayloadStatus::Unknown:
        break;
    }
}

void _relayHandleStatus(size_t id, PayloadStatus status) {
    _relayHandleStatus(id, status, RelayCommonStatusFlags);
}

[[gnu::unused]]
bool _relayHandlePayload(size_t id, espurna::StringView payload, uint8_t flags) {
    const auto status = relayParsePayload(payload);
    if (status != PayloadStatus::Unknown) {
        _relayHandleStatus(id, status, flags);
        return true;
    }

    return false;
}

[[gnu::unused]]
bool _relayHandlePayload(size_t id, espurna::StringView payload) {
    return _relayHandlePayload(id, payload, RelayCommonStatusFlags);
}

// Process lingering timer objects *after* relay changes state

void _relayProcessTimer(size_t id, Relay::PulseMode mode, Relay::Delay delay, bool status) {
    using namespace espurna::relay::timer;

    auto* timer = find(id);
    if (timer) {
        // Pending PULSE w/ an explicit timer, expected to be generated by an API call
        if (is_pulse(*timer) && timer->contains(id, !status)) {
            timer->start();
            DEBUG_MSG_P(PSTR("[RELAY] #%zu %s scheduled in %lu (ms)\n"),
                id, !status ? PSTR("ON") : PSTR("OFF"),
                timer->duration().count());
        // ...otherwise, type does not matter and it should be canceled
        } else {
            cancel(*timer);
        }
    }

    using namespace espurna::relay::pulse;

    // Note that timer invocation below only expected to happen after API calls, where timer is either canceled or does not exist yet
    // Pulse time is set up via configure() and should switch relay back to normal state after this timer expires
    if ((delay > Relay::Delay::zero()) && wouldChange(mode, status)) {
        schedule_and_start(id, delay, !status, RelayCommonStatusFlags | RelayFlagTimerPulse);
        DEBUG_MSG_P(PSTR("[RELAY] #%zu %s scheduled in %lu (ms)\n"),
            id, status ? PSTR("ON") : PSTR("OFF"),
            delay.count());
    }
}

// duration equal to 0 would cancel existing timer
// duration greater than 0 would toggle relay and schedule a timer
[[gnu::unused]]
void _relayHandleTimerNative(size_t id, espurna::relay::timer::Duration duration, bool toggle) {
    using namespace espurna::relay::timer;

    if (duration == decltype(duration)::zero()) {
        cancel(id);
        return;
    }

    const auto status = _relayStatus(id);
    const auto target = toggle ? status : !status;

    uint8_t flags = RelayCommonStatusFlags;
    if (toggle) {
        flags |= RelayFlagTimerPulse;
    } else {
        flags |= RelayFlagTimerDelay;
    }

    RelayMaskPair pair;

    auto& mask = target ? pair.on : pair.off;
    mask[id] = true;

    if (!toggle && (_relay_sync_mode != RelaySync::None)) {
        pair = _relaySyncPair(_relay_sync_mode, id, target, _relays.size());
        flags |= RelayFlagSync;
    }

    auto* timer = schedule(pair, duration, flags);

    if (toggle) {
        _relayToggle(id, RelayCommonStatusFlags);
    } else {
        (*timer).start();
    }
}

[[gnu::unused]]
void _relayHandleTimerResult(size_t id, espurna::duration::PairResult result, bool toggle) {
    using namespace espurna::relay::timer;

    const auto native = settings::native_duration(result);
    _relayHandleTimerNative(id, native, toggle);
}

// expects generic time input which is converted to float first
[[gnu::unused]]
bool _relayHandleTimerPayloadImpl(size_t id, espurna::StringView payload, bool toggle) {
    using namespace espurna::relay::timer;

    const auto result = settings::parse_time(payload);
    if (result.ok) {
        _relayHandleTimerResult(id, result, toggle);
        return true;
    }

    return false;
}

[[gnu::unused]]
bool _relayHandlePulsePayload(size_t id, espurna::StringView payload) {
    return _relayHandleTimerPayloadImpl(id, payload, true);
}

[[gnu::unused]]
bool _relayHandleTimerPayload(size_t id, espurna::StringView payload) {
    return _relayHandleTimerPayloadImpl(id, payload, false);
}

[[gnu::unused]]
PayloadStatus _relayInvertStatus(PayloadStatus status) {
    switch (status) {
    case PayloadStatus::On:
        return PayloadStatus::Off;
    case PayloadStatus::Off:
        return PayloadStatus::On;
    case PayloadStatus::Toggle:
    case PayloadStatus::Unknown:
        break;
    }

    return PayloadStatus::Unknown;
}

[[gnu::unused]]
PayloadStatus _relayPayloadStatus(size_t id) {
    if (_relays_ready[id]) {
        return _relays[id].current_status
            ? PayloadStatus::On
            : PayloadStatus::Off;
    }

    return PayloadStatus::Unknown;
}

template <typename T>
T _relayTristateFromPayload(const String& value) {
    return espurna::settings::internal::RelayTristateHelper<T>::convert(value);
}

template <typename T>
String _relayTristateToPayload(T value) {
    return espurna::settings::internal::RelayTristateHelper<T>::serialize(value);
}

RelayLock _relayLock(size_t id) {
    const auto target_status = _relayTargetStatus(id);
    const auto lock = target_status ? RelayLock::On : RelayLock::Off;
    _relayLock(id, lock);

    return lock;
}

void _relayLock(size_t id, RelayLock lock) {
    _relays[id].lock = lock;
}

void _relayLockStatus(size_t id, RelayLock lock) {
    bool status;

    switch (lock) {
    case RelayLock::None:
        status = _relayTargetStatus(id);
        break;

    case RelayLock::Off:
    case RelayLock::On:
        status = RelayLock::On == lock;
        break;

    default:
        __builtin_unreachable();
    }

    _relayLock(id, lock);
    _relayStatus(id, status);
}

void _relayLockStatus(size_t id) {
    const auto lock = _relayLock(id);
    _relayStatus(id, (RelayLock::On == lock));
}

[[gnu::unused]]
bool _relayHandleLockPayload(size_t id, espurna::StringView payload) {
    if (_relays_ready[id]) {
        const auto lock = _relayTristateFromPayload<RelayLock>(payload.toString());
        _relayLockStatus(id, lock);
        return true;
    }

    return false;
}

void _relayLockSync(const RelayMaskPair& pair) {
    _relay_sync_unlock.restore(_relays);
    _relay_sync_unlock.save(_relays, pair);
    _relay_unlock_timer.prepare(
        []() {
            _relay_sync_unlock.restore(_relays);
#if WEB_SUPPORT
            _relayScheduleWebSocketReport();
#endif
        });
}

bool _relayLockedStatus(const Relay& relay, bool status) {
    switch (relay.lock) {
    case RelayLock::None:
        break;

    case RelayLock::On:
        status = true;
        break;

    case RelayLock::Off:
        status = false;
        break;
    }

    return status;
}

// https://github.com/xoseperez/espurna/issues/1510#issuecomment-461894516
// activate interlock delay when current status is ON, and immediate change otherwise
void _relayPrepareUnlock() {
    if (_relay_unlock_timer.prepared() && !_relay_unlock_timer.scheduled()) {
        bool interlock { false };

        for (const auto& relay : _relays) {
            if (relay.current_status != relay.target_status) {
                return;
            }

            if (relay.current_status) {
                interlock = true;
            }
        }

        _relay_unlock_timer.schedule(
            interlock
                ? _relay_delay_interlock
                : RelayDelayedTimer::Duration::zero());
    }
}

void _relayProcessUnlock() {
    _relay_unlock_timer.process();
}

} // namespace

// -----------------------------------------------------------------------------
// RELAY
// -----------------------------------------------------------------------------

namespace {

inline RelayMaskHelper _relayMaskRtcmem() {
    RelayMaskHelper out;
    out.value = Rtcmem->relay;
    return out;
}

inline void _relayMaskRtcmem(uint32_t mask) {
    Rtcmem->relay = mask;
}

inline void _relayMaskRtcmem(const RelayMaskHelper& mask) {
    _relayMaskRtcmem(mask.toUnsigned());
}

} // namespace

void relayTimer(size_t id, espurna::duration::Milliseconds duration) {
    if (id < _relays.size()) {
        _relayHandleTimerNative(id, duration, false);
    }
}

void relayPulse(size_t id, espurna::duration::Milliseconds duration) {
    if (id < _relays.size()) {
        _relayHandleTimerNative(id, duration, true);
    }
}

// -----------------------------------------------------------------------------

namespace {

bool _relayStatus(size_t id) {
    return _relays[id].current_status;
}

bool _relayTargetStatus(size_t id) {
    auto timer = espurna::relay::timer::find(id);
    if (timer && timer->contains(id)) {
        return timer->status(id);
    }

    return _relays[id].target_status;
}

uint8_t _relayTargetFlags(size_t id) {
    auto timer = espurna::relay::timer::find(id);
    if (timer && timer->contains(id)) {
        return timer->flags();
    }

    return _relays[id].flags;
}

// When any source goes ON or OFF, sync with other relays
RelayMaskPair _relaySyncAll(size_t source, bool status, size_t relays) {
    RelayMaskPair out;
    auto& mask = status ? out.on : out.off;

    for (size_t id = 0; id < relays; ++id) {
        mask[id] = true;
    }

    return out;
}

// When source is 0 and goes ON or OFF, sync with other relays
// When source is anything else, fall through to the normal status change
RelayMaskPair _relaySyncFirst(size_t source, bool status, size_t relays) {
    RelayMaskPair out;
    auto& mask = status ? out.on : out.off;

    if (source == 0) {
        for (size_t id = 0; id < relays; ++id) {
            mask[id] = true;
        }
    }

    return out;
}

// When source goes ON, turn everything else OFF
// When source goes OFF, still turn everything else OFF
RelayMaskPair _relaySyncZeroOrOne(size_t source, bool status, size_t relays) {
    RelayMaskPair out;

    auto& source_mask = status ? out.on : out.off;
    source_mask[source] = true;

    for (size_t id = 0; id < relays; ++id) {
        if (id != source) {
            out.off[id] = true;
        }
    }

    return out;
}

// When source goes ON, turn everything else OFF
// When source goes OFF, select *next* relay and turn it ON instead
// (current implementation for *next* is really just next relay by ID; starting from 0 when reaching last one)
RelayMaskPair _relaySyncJustOne(size_t source, bool status, size_t relays) {
    if (status) {
        return _relaySyncZeroOrOne(source, status, relays);
    }

    RelayMaskPair out;

    out.off[source] = true;

    const auto id = (source + 1) % relays;
    out.on[id] = true;

    return out;
}

void _relaySyncScheduleOrStatus(RelayMaskPair pair, Relay::Delay delay, uint8_t flags) {
    // same as status, skip delay for retained outputs
    // but, only when *all* relays in the mask are affected
    if (flags & RelayFlagBoot) {
        const auto mask_all = pair.on | pair.off;
        const auto retained = mask_all & ~_relays_retained;
        if (!retained.any()) {
            delay = Relay::Delay::zero();
        }
    }

    if (delay != Relay::Delay::zero()) {
        espurna::relay::timer::schedule_and_start(pair, delay, flags);
        DEBUG_MSG_P(PSTR("[RELAY] Sync scheduled in %u (ms)\n"), delay.count());
    } else {
        _relayStatusPair(pair, flags);
    }
}

bool _relaySyncSchedule(RelayMaskPair pair, uint8_t flags) {
    bool out = false;

    const auto pair_on = pair.on.any();
    const auto pair_off = pair.off.any();

    if (pair_on || pair_off) {
        const auto pair_delay = (pair_on && pair_off)
            ? std::max(_relay_delay_on, _relay_delay_off) :
                (pair_on ? _relay_delay_on :
                 pair_off ? _relay_delay_off : Relay::Delay::zero());
        _relaySyncScheduleOrStatus(pair, pair_delay, flags | RelayFlagSync);
        out = true;
    }

    return out;
}

using RelaySyncFunc = RelayMaskPair (*)(size_t, bool, size_t);

constexpr RelaySyncFunc _relaySyncFunc(RelaySync mode) {
    return (RelaySync::All == mode)
            ? _relaySyncAll :
        (RelaySync::First == mode)
            ? _relaySyncFirst :
        (RelaySync::ZeroOrOne == mode)
            ? _relaySyncZeroOrOne :
        (RelaySync::JustOne == mode)
            ? _relaySyncJustOne
            : nullptr;
}

RelayMaskPair _relaySyncPair(RelaySync mode, size_t source, bool source_target_status, size_t relays) {
    RelayMaskPair out;

    const auto sync = _relaySyncFunc(mode);
    if (sync) {
        out = sync(source, source_target_status, relays);
    }

    return out;
}

// Usually called from status function when not flagged w/ RelayFlagSync
// Mode check happens here, so could be left off without any guards
bool _relaySync(size_t source, bool source_target_status, uint8_t flags) {
    // Don't sync when disabled
    if (_relay_sync_mode == RelaySync::None) {
        return false;
    }

    // Don't sync when there are no or less than 2 relays available
    const auto relays = _relays.size();
    if (relays < 2) {
        return false;
    }

    flags |= RelayFlagSync;

    const auto mode = _relay_sync_mode;
    const auto pair = _relaySyncPair(mode, source, source_target_status, relays);

    if (_relaySyncSchedule(pair, flags)) {
        switch (mode) {
        case RelaySync::ZeroOrOne:
        case RelaySync::JustOne:
            _relayLockSync(pair);
            break;

        default:
            break;
        }

        return true;
    }

    return false;
}

bool _relayStatusNotify(size_t id, bool status) {
    bool changed = false;

    auto& relay = _relays[id];
    auto* timer = espurna::relay::timer::find(id);

    if (timer) {
        // Restart PULSE polling 'current' status (#454)
        if (espurna::relay::timer::is_pulse(*timer) && timer->started()) {
            timer->start();
        // Cancel DELAY polling 'current' status, avoid repeating this 'notify'
        } else {
            espurna::relay::timer::cancel(*timer);
            changed = true;
        }
    }

    if (relay.target_status != status) {
        relay.target_status = status;
        relay.flags = 0;
        changed = true;
    }

    relay.provider->notify(status);
    for (auto& notify : _relays_on_status_notify) {
        notify(id, status);
    }

    if (changed) {
        DEBUG_MSG_P(PSTR("[RELAY] #%u scheduled change cancelled\n"), id);
    }

    return changed;
}

bool _relayStatusChange(size_t id, bool status, uint8_t flags) {
    bool changed = false;

    auto change_delay = Relay::Delay::zero();
    auto& relay = _relays[id];

    constexpr auto FlagsScheduled = uint8_t{ RelayFlagSync | RelayFlagTimerPulse | RelayFlagTimerDelay };

    if (0 == (flags & FlagsScheduled)) {
        change_delay = status
            ? relay.delay_on
            : relay.delay_off;

        ++relay.fw_count;
        relay.fw_count = std::clamp(
            relay.fw_count,
            uint8_t{ 1 }, _relay_flood_changes);

        // Reset counter when flood window is no longer needed
        const auto fw_now = Relay::TimeSource::now();
        const auto fw_diff = fw_now - relay.fw_start;
        if (fw_diff > _relay_flood_window) {
            relay.fw_start = fw_now;
            relay.fw_count = 1;

        // Set up an explicit delay otherwise
        } else if (relay.fw_count >= _relay_flood_changes) {

            // We schedule the changes to the end of the floodWindow
            // unless it's already delayed beyond that point
            change_delay = std::max(change_delay, _relay_flood_window - fw_diff);

            // Another option is to always move it forward, starting from current time
            // relay.fw_start = current_time;
        }
    }

    // Previously scheduled timer already spent 'change_delay' time waiting, and now it can finally be processed
    if (flags & FlagsScheduled) {
        change_delay = Relay::Delay::zero();
        flags &= ~FlagsScheduled;
    }

    // Or, when relay status is retained between software resets
    if ((flags & RelayFlagBoot) && _relays_retained[id]) {
        change_delay = Relay::Delay::zero();
    }

    // Otherwise, schedule delay is expected to be placed here
    if (change_delay != Relay::Delay::zero()) {
        espurna::relay::timer::schedule_and_start(
            id, change_delay, status, flags | RelayFlagTimerDelay);
        DEBUG_MSG_P(PSTR("[RELAY] #%u scheduled %s in %u (ms)\n"),
            id, status ? PSTR("ON") : PSTR("OFF"),
            change_delay.count());

    // Or, relay is going to be switched ON / OFF next processing loop
    } else {
        relay.target_status = status;
        relay.flags = flags;
        changed = true;
    }

    return changed;
}

bool _relayStatus(size_t id, bool status, uint8_t flags) {
    auto& relay = _relays[id];

    if (!_relays_ready[id] && (0 == (flags & RelayFlagBoot))) {
        flags |= RelayFlagBoot;
    }

    const auto flag_boot = (flags & RelayFlagBoot) > 0;
    if (!flag_boot) {
        const auto lock_status = _relayLockedStatus(relay, status);
        if (lock_status != status) {
            relay.flags |= RelayCommonStatusFlags;
            status = lock_status;
        }
    }

    const auto flag_sync = (flags & RelayFlagSync) > 0;

    if (!flag_sync && _relaySync(id, status, flags)) {
        return true;
    }

    bool changed { false };

    if (!flag_boot && relay.current_status == status) {
        changed = _relayStatusNotify(id, status);
    } else {
        changed = _relayStatusChange(id, status, flags);
    }

    return changed;
}

bool _relayStatus(size_t id, bool status) {
    return _relayStatus(id, status, RelayCommonStatusFlags);
}

bool _relayToggle(size_t id, uint8_t flags) {
    const auto status = !_relays[id].current_status;
    return _relayStatus(id, status, flags);
}

bool _relayToggle(size_t id) {
    return _relayToggle(id, RelayCommonStatusFlags);
}

using RelayStatusFunc = bool (*)(size_t);

RelayStatus _relayStatusEnum(size_t id, RelayStatusFunc func) {
    if (id < _relays.size()) {
        if (!_relays_ready[id]) {
            return RelayStatus::NotReady;
        }

        return func(id)
            ? RelayStatus::On
            : RelayStatus::Off;
    }

    return RelayStatus::NotAvailable;
}

} // namespace

bool relayStatus(size_t id, bool status) {
    if (id < _relays.size()) {
        return _relayStatus(id, status, RelayCommonStatusFlags);
    }

    return false;
}

bool relayToggle(size_t id) {
    if (id < _relays.size()) {
        return _relayToggle(id);
    }

    return false;
}

bool relayStatus() {
    for (size_t id = 0; id < _relays.size(); ++id) {
        if (_relays_ready[id] && _relays[id].current_status) {
            return true;
        }
    }

    return false;
}

RelayStatus relayStatus(size_t id) {
    return _relayStatusEnum(id, _relayStatus);
}

RelayStatus relayTargetStatus(size_t id) {
    return _relayStatusEnum(id, _relayTargetStatus);
}

namespace {

void _relaySave(bool persist) {
    const auto relays_available = _relays.size();
    const auto relays_ready = _relays_ready.count();

    if (relays_available && (relays_available == relays_ready)) {
        const auto mask = _relayMaskCurrent(relays_available);

        // Persist only to rtcmem, unless requested to save to settings
        _relayMaskRtcmem(mask);

        // The 'persist' flag controls whether we are commiting this change or not.
        // It is useful to set it to 'false' if the relay change triggering the
        // save involves a relay whose boot mode is independent from current mode,
        // thus storing the last relay value is not absolutely necessary.
        // Nevertheless, we store the value in the EEPROM buffer so it will be written
        // on the next commit.
        if (persist) {
            espurna::relay::settings::bootMask(mask);
            autosaveSettings();
        }

        DEBUG_MSG_P(PSTR("[RELAY] Relay mask: %s\n"), mask.toString().c_str());
    }
}

void _relaySave() {
    _relay_save_timer.process([](bool persist) {
        _relaySave(persist);
    });
}

void _relayScheduleSave(size_t id) {
    switch (espurna::relay::settings::bootMode(id)) {
    case RelayBoot::Same:
    case RelayBoot::Toggle:
        _relay_save_timer.persist();
        break;
    case RelayBoot::Off:
    case RelayBoot::On:
    case RelayBoot::LockedOff:
    case RelayBoot::LockedOn:
        break;
    }

    _relay_save_timer.schedule(espurna::relay::build::saveDelay());
}

} // namespace

size_t relayCount() {
    return _relayCount();
}

PayloadStatus relayParsePayload(espurna::StringView payload) {
#if MQTT_SUPPORT || API_SUPPORT
    return rpcParsePayload(
        payload,
        [](espurna::StringView payload) {
            if (payload.equals(_relay_payload_off)) {
                return PayloadStatus::Off;
            } else if (payload.equals(_relay_payload_on)) {
                return PayloadStatus::On;
            } else if (payload.equals(_relay_payload_toggle)) {
                return PayloadStatus::Toggle;
            }

            return PayloadStatus::Unknown;
        });
#else
    return rpcParsePayload(payload);
#endif
}

namespace {

void _relaySettingsMigrate(int version) {
    if (version < 5) {
        using namespace espurna::relay::settings;
        // just a rename
        moveSetting("relayDelayInterlock", keys::Interlock);

        // groups use a new set of keys
#if MQTT_SUPPORT
        for (size_t index = 0; index < RelaysMax; ++index) {
            auto group = getSetting({"mqttGroup", index});
            if (!group.length()) {
                break;
            }

            auto syncKey = espurna::settings::Key(F("mqttGroupSync"), index);
            auto sync = getSetting(syncKey);

            setSetting({keys::TopicSub, index}, group);
            if (sync.length()) {
                if (sync != "2") { // aka RECEIVE_ONLY
                    setSetting(keys::TopicMode, sync);
                    setSetting(keys::TopicPub, group);
                }
            }
        }
#endif

        delSettingPrefix({
            STRING_VIEW("mqttGroup"),     // migrated to relayTopic
            STRING_VIEW("mqttGroupSync"), // migrated to relayTopic
            STRING_VIEW("relayOnDisc"),   // replaced with relayMqttDisc
            STRING_VIEW("relayGPIO"),     // avoid depending on migrate module
            STRING_VIEW("relayGpio"),     // avoid depending on migrate module
            STRING_VIEW("relayProvider"), // different type
            STRING_VIEW("relayType"),     // different type
        });
        delSetting("relays"); // does not do anything
    }
}

struct RelayBootContext {
    Relay& relay;
    size_t id;
    bool retain;
    RelayMask mask;
    RelayBoot mode;

    bool mask_status() const {
        return mask[id];
    }
};

bool _relayBoot(const RelayBootContext& ctx) {
    auto status = false;

    auto lock = RelayLock::None;

    switch (ctx.mode) {
    case RelayBoot::Same:
        status = ctx.mask_status();
        break;

    case RelayBoot::Toggle:
        status = !ctx.mask_status();
        break;

    case RelayBoot::On:
        status = true;
        break;

    case RelayBoot::LockedOn:
        status = true;
        lock = RelayLock::On;
        break;

    case RelayBoot::Off:
        status = false;
        break;

    case RelayBoot::LockedOff:
        status = false;
        lock = RelayLock::Off;
        break;
    }

    auto& relay = ctx.relay;

    relay.provider->boot(status);
    relay.lock = lock;

    return status;
}

void _relayBootAll() {
    const auto rtcmem_available = rtcmemStatus();
    const auto boot_mask = rtcmem_available
        ? _relayMaskRtcmem()
        : espurna::relay::settings::bootMask();

    bool log = false;

    const auto sync = _relay_sync_mode != RelaySync::None;

    constexpr auto Flags = uint8_t{ RelayCommonStatusFlags | RelayFlagBoot };
    RelayMaskPair pair;

    RelayMask relay_status;

    for (size_t id = 0; id < _relays.size(); ++id) {
        if (!_relays_boot[id]) {
            const auto mode = espurna::relay::settings::bootMode(id);
            const auto ctx = RelayBootContext{
                .relay = _relays[id],
                .id = id,
                .retain = rtcmem_available,
                .mask = boot_mask.value,
                .mode = mode,
            };

            relay_status[id] = _relayBoot(ctx);

            _relays[id].flags |= Flags;
            if (!rtcmem_available || relay_status[id] != ctx.mask_status()) {
                _relays_retained[id] = false;
            }

            auto& mask = relay_status[id] ? pair.on : pair.off;
            mask[id] = true;

            _relays_boot[id] = true;
            log = true;
        }
    }

    if (pair.on.any() || pair.off.any()) {
        size_t sync_id;
        if (_relay_sync_id != RelaysMax) {
            sync_id = _relay_sync_id; // invalid IDs get filtered out later
        } else {
            sync_id = _relays_boot.count() - 1; // set at the same time as pair.{on,off}, cannot be 0
        }

        if (sync && _relaySync(sync_id, relay_status[sync_id], _relays[sync_id].flags)) {
            // sync'ed status already queued, no need to use boot on/off pair
            log = false;
        } else {
            _relayStatusPair(pair, Flags);
        }
    }

    if (log) {
        DEBUG_MSG_P(PSTR("[RELAY] Boot mask: %s (%zu total)\n"),
            boot_mask.toString().c_str(), _relays.size());
    }
}

void _relayConfigureGlobal() {
    _relay_flood_window = espurna::relay::flood::settings::window();
    _relay_flood_changes = espurna::relay::flood::settings::changes();

    _relay_delay_interlock = espurna::relay::settings::interlockDelay();
    _relay_delay_off = espurna::relay::settings::delayOff();
    _relay_delay_on = espurna::relay::settings::delayOn();

    _relay_sync_mode = espurna::relay::settings::syncMode();
    _relay_sync_id = espurna::relay::settings::syncId();

#if MQTT_SUPPORT || API_SUPPORT
    _relay_payload_on = espurna::relay::settings::payloadOn();
    _relay_payload_off = espurna::relay::settings::payloadOff();
    _relay_payload_toggle = espurna::relay::settings::payloadToggle();
#endif // MQTT_SUPPORT
}

void _relayConfigure(Relay& relay, size_t index) {
    relay.pulse = espurna::relay::pulse::settings::mode(index);
    relay.pulse_time = (relay.pulse != espurna::relay::pulse::Mode::None)
        ? espurna::relay::pulse::settings::time(index)
        : espurna::relay::timer::Duration::min();

    relay.delay_on = espurna::relay::settings::delayOn(index);
    relay.delay_off = espurna::relay::settings::delayOff(index);
}

void _relayConfigure() {
    for (size_t index = 0; index < _relays.size(); ++index) {
        _relayConfigure(_relays[index], index);
    }

    _relayConfigureGlobal();
}

} // namespace

//------------------------------------------------------------------------------
// WEBSOCKETS
//------------------------------------------------------------------------------

#if WEB_SUPPORT

namespace {

STRING_VIEW_INLINE(MultiRelay, "multirelay");

bool _relayWebSocketOnKeyCheck(espurna::StringView key, const JsonVariant&) {
    return key.startsWith(espurna::relay::settings::Prefix);
}

void _relayWebSocketUpdate(JsonObject& root) {
    espurna::web::ws::EnumerablePayload payload{root, STRING_VIEW("relayState")};
    payload(STRING_VIEW("values"), _relays.size(), {
        {STRING_VIEW("status"), [](JsonArray& out, size_t index) {
            out.add(_relays[index].target_status ? 1 : 0);
        }},
        {STRING_VIEW("lock"), [](JsonArray& out, size_t index) {
            out.add(static_cast<uint8_t>(_relays[index].lock));
        }},
    });
}

void _relayWebSocketSendRelays(JsonObject& root) {
    if (!_relays.size()) {
        return;
    }

    espurna::web::ws::EnumerableConfig config{root, STRING_VIEW("relayConfig")};

    auto& container = config.root();
    container[F("size")] = _relays.size();
    container[F("start")] = 0;

    config(STRING_VIEW("values"), _relays.size(),
        espurna::relay::settings::query::IndexedSettings);
}

void _relayWebSocketOnVisible(JsonObject& root) {
    const auto relays = _relays.size();
    if (!relays) {
        return;
    }

    if (relays > 1) {
        wsPayloadModule(root, MultiRelay);
        root[FPSTR(espurna::relay::settings::keys::Sync)] =
            espurna::settings::internal::serialize(espurna::relay::settings::syncMode());
        root[FPSTR(espurna::relay::settings::keys::Interlock)] =
            espurna::relay::settings::interlockDelay().count();
    }

    wsPayloadModule(root, espurna::relay::settings::Prefix);
}

void _relayWebSocketOnConnected(JsonObject& root) {
    _relayWebSocketSendRelays(root);
}

void _relayWebSocketOnAction(uint32_t, const char* action, JsonObject& data) {
    if (strncmp_P(action, PSTR("relay"), 5) == 0) {
        if (!data.is<size_t>(F("id")) || !data.is<String>(F("status"))) {
            return;
        }

        const auto id = data[F("id")].as<size_t>();
        const auto status = data[F("status")].as<String>();
        _relayHandlePayload(id, status);
    }
}

void _relayWebSocketReport() {
    if (_relay_report_ws) {
        wsPost(_relayWebSocketUpdate);
        _relay_report_ws = false;
    }
}

void _relaySetupWeb() {
    wsRegister()
        .onVisible(_relayWebSocketOnVisible)
        .onConnected(_relayWebSocketOnConnected)
        .onData(_relayWebSocketUpdate)
        .onAction(_relayWebSocketOnAction)
        .onKeyCheck(_relayWebSocketOnKeyCheck);
}

} // namespace

#endif // WEB_SUPPORT

//------------------------------------------------------------------------------
// API SHARED DATA
//------------------------------------------------------------------------------

namespace {

STRING_VIEW_INLINE(RelayTopicRelay, MQTT_TOPIC_RELAY);
STRING_VIEW_INLINE(RelayTopicRelaySub, MQTT_TOPIC_RELAY "/+");

STRING_VIEW_INLINE(RelayTopicPulse, MQTT_TOPIC_PULSE);
STRING_VIEW_INLINE(RelayTopicPulseSub, MQTT_TOPIC_PULSE "/+");

STRING_VIEW_INLINE(RelayTopicTimer, MQTT_TOPIC_TIMER);
STRING_VIEW_INLINE(RelayTopicTimerSub, MQTT_TOPIC_TIMER "/+");

STRING_VIEW_INLINE(RelayTopicLock, MQTT_TOPIC_LOCK);
STRING_VIEW_INLINE(RelayTopicLockSub, MQTT_TOPIC_LOCK "/+");

STRING_VIEW_INLINE(RelayTopicRelayDescription, MQTT_TOPIC_DESCRIPTION "/" MQTT_TOPIC_RELAY);

} // namespace

//------------------------------------------------------------------------------
// REST API
//------------------------------------------------------------------------------

#if API_SUPPORT

namespace {

template <typename T>
bool _relayApiTryHandle(ApiRequest& request, T&& callback) {
    const auto param = request.wildcard(0);

    size_t id;
    if (!_relayTryParseId(param, id)) {
        return false;
    }

    return callback(id);
}

espurna::relay::timer::Seconds _relayApiFindDuration(size_t id) {
    using namespace espurna::relay::timer;

    Seconds out{};

    auto it = find(id);
    if (it) {
        out = std::chrono::duration_cast<Seconds>((*it).duration());
    }

    return out;
}

bool _relayApiTimerGet(ApiRequest& request, size_t id) {
    using namespace espurna::relay::timer;
    const auto duration = _relayApiFindDuration(id);
    const auto seconds = std::chrono::duration_cast<Seconds>(duration);
    request.send(String(seconds.count(), 10));
    return true;
}

void _relaySetupApi() {
    apiRegister(RelayTopicRelay.toString(),
        [](ApiRequest&, JsonObject& root) {
            JsonArray& out = root.createNestedArray("relayStatus");
            for (auto& relay : _relays) {
                out.add(relay.target_status ? 1 : 0);
            }
            return true;
        },
        nullptr
    );

    apiRegister(RelayTopicRelaySub.toString(),
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                request.send(String(_relays[id].target_status ? 1 : 0));
                return true;
            });
        },
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                return _relayHandlePayload(id, request.param(F("value")));
            });
        }
    );

    apiRegister(RelayTopicPulseSub.toString(),
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                return _relayApiTimerGet(request, id);
            });
        },
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                return _relayHandlePulsePayload(id, request.param(F("value")));
            });
        }
    );

    apiRegister(RelayTopicTimerSub.toString(),
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                return _relayApiTimerGet(request, id);
            });
        },
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                return _relayHandleTimerPayload(id, request.param(F("value")));
            });
        }
    );

    apiRegister(RelayTopicLockSub.toString(),
        [](ApiRequest& request) {
            return _relayApiTryHandle(request,
                [&](size_t id) {
                    request.send(_relayTristateToPayload<RelayLock>(_relays[id].lock));
                    return true;
                });
        },
        [](ApiRequest& request) {
            return _relayApiTryHandle(request,
                [&](size_t id) {
                    return _relayHandleLockPayload(id, request.param(F("value")));
                });
        }
    );
}

} // namespace

#endif // API_SUPPORT

//------------------------------------------------------------------------------
// MQTT
//------------------------------------------------------------------------------

#if MQTT_SUPPORT || API_SUPPORT

espurna::StringView relayPayloadOn() {
    return _relay_payload_on;
}

espurna::StringView relayPayloadOff() {
    return _relay_payload_off;
}

espurna::StringView relayPayloadToggle() {
    return _relay_payload_toggle;
}

espurna::StringView relayPayload(PayloadStatus status) {
    switch (status) {
    case PayloadStatus::Off:
        return _relay_payload_off;
    case PayloadStatus::On:
        return _relay_payload_on;
    case PayloadStatus::Toggle:
        return _relay_payload_toggle;
    case PayloadStatus::Unknown:
        break;
    }

    return "";
}

#endif // MQTT_SUPPORT || API_SUPPORT

#if MQTT_SUPPORT

namespace {

// TODO: it *will* handle the duplicates, but we waste memory storing them
// TODO: mqttSubscribe(...) also happens multiple times
//
// this is not really an intended use-case though, but it is techically possible...

struct RelayCustomTopic {
    using Mode = RelayMqttTopicMode;

    RelayCustomTopic() = delete;
    RelayCustomTopic(const RelayCustomTopic&) = delete;

    RelayCustomTopic(RelayCustomTopic&& other) noexcept :
        _id(other._id),
        _topic(std::move(other._topic)),
        _parts(_topic, std::move(other._parts)),
        _mode(other._mode)
    {}

    RelayCustomTopic(size_t id, String topic, Mode mode) :
        _id(id),
        _topic(std::move(topic)),
        _parts(_topic),
        _mode(mode)
    {}

    size_t id() const {
        return _id;
    }

    const char* c_str() const {
        return _topic.c_str();
    }

    const String& topic() const {
        return _topic;
    }

    const PathParts& parts() const {
        return _parts;
    }

    Mode mode() const {
        return _mode;
    }

    bool match(const String& other) const {
        PathParts parts(other);
        return _parts.match(parts);
    }

    bool match(const PathParts& parts) const {
        return _parts.match(parts);
    }

private:
    size_t _id;
    String _topic;
    PathParts _parts;
    RelayMqttTopicMode _mode;
};

void _relayMqttSubscribeBaseTopics() {
    mqttSubscribe(RelayTopicRelaySub.toString().c_str());
    mqttSubscribe(RelayTopicPulseSub.toString().c_str());
    mqttSubscribe(RelayTopicTimerSub.toString().c_str());
    mqttSubscribe(RelayTopicLockSub.toString().c_str());
}

std::forward_list<RelayCustomTopic> _relay_custom_topics;

void _relayMqttSubscribeCustomTopics() {
    const auto relays = _relays.size();
    if (!relays) {
        return;
    }

    // TODO: previous version attempted to optimize the settings loop by creating a temporary
    // mapping of {id, topic, mode} from build values and then do settings::foreach with
    // matcher for topic & mode key prefixes. but, that also required parsing of the id,
    // which could be either avoided by creating something like {{key, topic}, {key, mode}} instead,
    // but the tradeoff would be searching that array for each key match. this one is *much* shorter

    _relay_custom_topics.clear();
    for (size_t id = 0; id < relays; ++id) {
        auto subscription = espurna::relay::settings::mqttTopicSub(id);
        if (!subscription.length()) {
            continue;
        }

        auto topic = RelayCustomTopic{
            id, std::move(subscription),
            espurna::relay::settings::mqttTopicMode(id)};
        if (!topic.parts()) {
            continue;
        }

        mqttSubscribeRaw(topic.topic().c_str());
        _relay_custom_topics.emplace_front(std::move(topic));
    }
}

void _relayMqttPublishCustomTopic(size_t id) {
    const auto topic = espurna::relay::settings::mqttTopicPub(id);
    if (!topic.length()) {
        return;
    }

    auto status = _relayPayloadStatus(id);

    auto mode = espurna::relay::settings::mqttTopicMode(id);
    if (mode == RelayMqttTopicMode::Inverse) {
        status = _relayInvertStatus(status);
    }

    mqttSendRaw(topic.c_str(), relayPayload(status).begin());
}

void _relayMqttPublish(size_t id) {
    const auto topic = RelayTopicRelay.toString();

    const auto status = _relayPayloadStatus(id);
    const auto payload = relayPayload(status);

    mqttSend(topic.c_str(), id, payload.c_str());
}

void _relayMqttReport(size_t id, uint8_t flags) {
    if (mqttForward() && (flags & RelayFlagReport)) {
        _relayMqttPublish(id);
    }

    if (flags & RelayFlagReportCustom) {
        _relayMqttPublishCustomTopic(id);
    }
}

void _relayMqttReportAll() {
    for (size_t id = 0; id < _relays.size(); ++id) {
        if (_relays_ready[id]) {
            _relayMqttPublish(id);
        }
    }
}

void _relayMqttReportDescription() {
    if (!_relays.size()) {
        return;
    }

    const auto topic = RelayTopicRelayDescription.toString();
    for (size_t id = 0; id < _relays.size(); ++id) {
        if (_relays_ready[id]) {
            const auto name = espurna::relay::settings::name(id);
            if (name.length()) {
                mqttSend(topic.c_str(), id, name.c_str());
            }
        }
    }
}

bool _relayHandleMqttPayload(size_t id, espurna::StringView payload) {
    return _relayHandlePayload(id, payload);
}

bool _relayMqttHeartbeat(espurna::heartbeat::Mask mask) {
    if (mask & espurna::heartbeat::Report::Relay) {
        _relayMqttReportAll();
    }

    if (mask & espurna::heartbeat::Report::Description) {
        _relayMqttReportDescription();
    }

    return mqttConnected();
}

void _relayMqttHandleCustomTopic(espurna::StringView topic, espurna::StringView payload) {
    PathParts received(topic);
    for (auto& topic : _relay_custom_topics) {
        if (topic.match(received)) {
            auto status = relayParsePayload(payload);
            if (topic.mode() == RelayMqttTopicMode::Inverse) {
                status = _relayInvertStatus(status);
            }

            _relayHandleStatus(topic.id(), status, RelayFlagReport);
        }
    }
}

void _relayMqttHandleDisconnectImmediate() {
    using namespace espurna::relay::settings;
    for (size_t id = 0; id < _relays.size(); ++id) {
        if (_relays_ready[id]) {
            _relayHandleStatus(id, mqttDisconnectionStatus(id));
        }
    }
}

void _relayMqttHandleDisconnect() {
    using namespace espurna::relay::settings;
    const auto duration = mqttDisconnectionDelay();

    if (duration == decltype(duration)::zero()) {
        _relayMqttHandleDisconnectImmediate();
        return;
    }

    RelayMaskPair pair;
    pair.off = _relays_ready;

    if (pair.off.any()) {
        espurna::relay::timer::schedule_and_start(
            pair, duration,
            RelayCommonStatusFlags | RelayFlagTimerDelay);
    }
}

void _relayMqttHandleConnect() {
    _relayMqttSubscribeBaseTopics();
    _relayMqttSubscribeCustomTopics();
}

struct RelayMqttTopicHandler {
    using Handler = bool(*)(size_t, espurna::StringView);
    espurna::StringView topic;
    Handler handler;
};

static constexpr RelayMqttTopicHandler RelayMqttTopicHandlers[] PROGMEM {
    {RelayTopicRelay, _relayHandleMqttPayload},
    {RelayTopicPulse, _relayHandlePulsePayload},
    {RelayTopicTimer, _relayHandleTimerPayload},
    {RelayTopicLock, _relayHandleLockPayload},
};

bool _relay_mqtt_connected;

void _relayMqttCallback(unsigned int type, espurna::StringView topic, espurna::StringView payload) {
    if (!_relays.size()) {
        return;
    }

    if (type == MQTT_CONNECT_EVENT) {
        _relayMqttHandleConnect();
        _relay_mqtt_connected = true;
        return;
    }

    if (type == MQTT_MESSAGE_EVENT) {
        const auto t = mqttMagnitude(topic);

        for (const auto pair: RelayMqttTopicHandlers) {
            if (t.startsWith(pair.topic)) {
                size_t id;
                if (!_relayTryParseIdFromPath(t, id)) {
                    return;
                }

                pair.handler(id, payload);
                return;
            }
        }

        _relayMqttHandleCustomTopic(topic, payload);
        return;
    }

    if (type == MQTT_DISCONNECT_EVENT) {
        if (_relay_mqtt_connected) {
            _relay_mqtt_connected = false;
            _relayMqttHandleDisconnect();
        }
        return;
    }
}

void _relaySetupMqtt() {
    mqttHeartbeat(_relayMqttHeartbeat);
    mqttRegister(_relayMqttCallback);
}

} // namespace

#endif

//------------------------------------------------------------------------------
// Settings
//------------------------------------------------------------------------------

#if TERMINAL_SUPPORT

namespace {

using TerminalRelayPrintExtra = void(*)(const Relay&, char* out, size_t size);

STRING_VIEW_INLINE(ErrTerminalRelayInvalidId, "Invalid relay ID");
STRING_VIEW_INLINE(ErrTerminalRelayInvalidStatus, "Invalid status");
STRING_VIEW_INLINE(ErrTerminalRelayInvalidTime, "Invalid time");
STRING_VIEW_INLINE(ErrTerminalRelayNoTimers, "No active timers");

String _relayErrorCommand(::terminal::CommandContext& ctx, espurna::StringView params) {
    String name = std::move(ctx.argv[0]);

    name.toUpperCase();
    name += ' ';
    name += params;

    return name;
}

String _relayLockPayload(const Relay& relay) {
    return _relayTristateToPayload(relay.lock);
}

constexpr char _relayFlagTag(uint8_t flag) {
    return (RelayFlagBoot == flag) ? 'B' :
        (RelayFlagSync == flag) ? 'S' :
        (RelayFlagReport == flag) ? 'R' :
        (RelayFlagReportCustom == flag) ? 'C' :
        (RelayFlagTimerDelay == flag) ? 'D' :
        (RelayFlagTimerPulse == flag) ? 'P' :
        '.';
}

String _relayFlagsPayload(uint8_t flags) {
    char tmp[8];

    for (size_t index = 0; index < std::size(tmp); ++index) {
        const uint8_t mask = 1 << index;
        const uint8_t flag = flags & mask;
        tmp[index] = _relayFlagTag(flag);
        flags &= ~mask;
    }

    String out;
    out.concat(std::begin(tmp), std::size(tmp));

    return out;
}

void _relayPrint(Print& out, const Relay& relay, size_t index) {
    const auto provider = relay.provider->id();

    const auto current_status = _relayStatus(index);
    const auto target_status = _relayTargetStatus(index);

    const auto status = (current_status != target_status)
        ? (current_status && !target_status)
           ? STRING_VIEW("{ON -> OFF}")
           : STRING_VIEW("{OFF -> ON}")
        : current_status
            ? STRING_VIEW("ON")
            : STRING_VIEW("OFF");

    const auto lock = _relayLockPayload(relay);

    String flags;
    if (current_status != target_status) {
        const auto target_flags = _relayTargetFlags(index);
        flags = _relayFlagsPayload(target_flags);
    }

    out.printf_P(PSTR("relay%zu\t{Prov=%.*s Status=%.*s Lock=%.*s Flags=[%.*s]}\n"),
        index,
        provider.length(), provider.begin(),
        status.length(), status.begin(),
        lock.length(), lock.begin(),
        flags.length(), flags.begin());
}

void _relayPrint(Print& out, size_t start, size_t stop) {
    for (size_t index = start; index < stop; ++index) {
        _relayPrint(out, _relays[index], index);
    }
}

STRING_VIEW_INLINE(RelayCommand, "RELAY");

static void _relayCommand(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 1) {
        _relayPrint(ctx.output, 0, _relays.size());
        terminalOK(ctx);
        return;
    }

    size_t id;
    if (!_relayTryParseId(ctx.argv[1], id)) {
        terminalError(ctx, ErrTerminalRelayInvalidId);
        return;
    }

    ctx.output.println(id);

    if (ctx.argv.size() > 2) {
        auto status = relayParsePayload(ctx.argv[2]);
        if (PayloadStatus::Unknown == status) {
            terminalError(ctx, ErrTerminalRelayInvalidStatus);
            return;
        }

        _relayHandleStatus(id, status);
        _relayPrint(ctx.output, _relays[id], id);
        terminalOK(ctx);
        return;
    }

    settingsDump(ctx, espurna::relay::settings::query::IndexedSettings, id);
    terminalOK(ctx);
}

STRING_VIEW_INLINE(PulseCommand, "PULSE");

static void _relayCommandDumpTimers(::terminal::CommandContext&& ctx) {
    using namespace espurna::relay;

    if (timer::internal::timers.empty()) {
        terminalError(ctx, ErrTerminalRelayNoTimers);
        return;
    }

    size_t index = 0;
    for (auto& timer : timer::internal::timers) {
        const auto type = static_cast<bool>(timer)
            ? STRING_VIEW("Active")
            : STRING_VIEW("Pending");

        const auto start_time = timer.start_time().time_since_epoch();
        const auto duration = timer.duration();

        const auto serialize_mask = [](RelayMask mask) {
            return formatUnsigned(mask.to_ulong(), 2);
        };

        const auto mask_on = serialize_mask(timer.on());
        const auto mask_off = serialize_mask(timer.off());

        const auto flags = _relayFlagsPayload(timer.flags());

        ctx.output.printf_P(
            PSTR("timer%zu\t{%.*s Duration=%u Started=%u On=%.*s Off=%.*s Flags=[%.*s]}\n"),
            index++,
            type.length(), type.begin(),
            duration.count(),
            start_time.count(),
            mask_on.length(), mask_on.begin(),
            mask_off.length(), mask_off.begin(),
            flags.length(), flags.begin());
    }

    terminalOK(ctx);
}

static void _relayCommandTimerImpl(::terminal::CommandContext&& ctx, bool toggle) {
    if (ctx.argv.size() > 3) {
        const auto error = _relayErrorCommand(ctx, STRING_VIEW("[<ID>] [<TIME>]"));
        terminalError(ctx, error);
        return;
    }

    using namespace espurna::relay;

    if (ctx.argv.size() == 1) {
        _relayCommandDumpTimers(std::move(ctx));
        return;
    }

    size_t id;
    if (!_relayTryParseId(ctx.argv[1], id)) {
        terminalError(ctx, ErrTerminalRelayInvalidId);
        return;
    }

    auto duration = timer::Duration::zero();

    if (ctx.argv.size() == 3) {
        const auto parsed = timer::settings::parse_time(ctx.argv[2]);
        if (!parsed.ok) {
            terminalError(ctx, ErrTerminalRelayInvalidTime);
            return;
        }

        duration = timer::settings::native_duration(parsed);
    }

    if (duration == decltype(duration)::zero()) {
        timer::cancel(id);
        terminalOK(ctx);
        return;
    }

    _relayHandleTimerNative(id, duration, toggle);
    terminalOK(ctx);
}

static void _relayCommandPulse(::terminal::CommandContext&& ctx) {
    _relayCommandTimerImpl(std::move(ctx), true);
}

STRING_VIEW_INLINE(TimerCommand, "TIMER");

static void _relayCommandTimer(::terminal::CommandContext&& ctx) {
    _relayCommandTimerImpl(std::move(ctx), false);
}

STRING_VIEW_INLINE(LockCommand, "LOCK");

static void _relayCommandLock(::terminal::CommandContext&& ctx) {
    const auto argc = ctx.argv.size();

    switch (argc) {
    case 2:
    case 3:
    {
        size_t id;
        if (!_relayTryParseId(ctx.argv[1], id)) {
            terminalError(ctx, ErrTerminalRelayInvalidId);
            return;
        }

        if (argc == 3) {
            const auto lock = _relayTristateFromPayload<RelayLock>(ctx.argv[2]);
            _relayLockStatus(id, lock);
        } else {
            _relayLockStatus(id);
        }

        terminalOK(ctx);
        return;
    }

    default:
        break;
    }

    const auto error = _relayErrorCommand(ctx, STRING_VIEW("<ID> [NONE | OFF | ON]"));
    terminalError(ctx, error);
}

STRING_VIEW_INLINE(UnlockCommand, "UNLOCK");

static void _relayCommandUnlock(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 2) {
        const auto error = _relayErrorCommand(ctx, STRING_VIEW("<ID"));
        terminalError(ctx, error);
        return;
    }

    size_t id;
    if (!_relayTryParseId(ctx.argv[1], id)) {
        terminalError(ctx, ErrTerminalRelayInvalidId);
        return;
    }

    _relayLock(id, RelayLock::None);
    _relayStatus(id, _relayTargetStatus(id));

    terminalOK(ctx);
}

static constexpr ::terminal::Command RelayCommands[] PROGMEM {
    {RelayCommand, _relayCommand},
    {PulseCommand, _relayCommandPulse},
    {TimerCommand, _relayCommandTimer},
    {LockCommand, _relayCommandLock},
    {UnlockCommand, _relayCommandUnlock},
};

void _relaySetupTerminal() {
    espurna::terminal::add(RelayCommands);
}

} // namespace

#endif // TERMINAL_SUPPORT

//------------------------------------------------------------------------------

namespace {

void _relayReport(size_t id [[gnu::unused]], bool status [[gnu::unused]], uint8_t flags [[gnu::unused]]) {
    for (auto& change : _relays_on_status_change) {
        change(id, status);
    }
#if MQTT_SUPPORT
    _relayMqttReport(id, flags);
#endif
#if WEB_SUPPORT
    _relayScheduleWebSocketReport();
#endif
#if DEBUG_SUPPORT
    DEBUG_MSG_P(PSTR("[RELAY] #%u set to %s\n"),
        id, status ? PSTR("ON") : PSTR("OFF"));
#endif
}

void _relayReport() {
#if WEB_SUPPORT
    _relayWebSocketReport();
#endif
}

void _relayActive(size_t id, bool status) {
    const auto prev = static_cast<bool>(_relays_ready[id]);
    _relays_ready[id] = true;

    if (prev) {
        return;
    }

    for (auto& callback : _relays_on_ready) {
        callback(id, status);
    }
}

/**
 * Walks the relay vector processing only those relays
 * that have to change to the requested mode
 * @bool mode Requested mode
 */
bool _relayProcess(bool mode) {
    bool changed { false };

    // Make sure target mode in the one requested via the argument
    // Also make sure that target status is different from the current one
    // *or* status processing was forced by any of the relay object flags
    for (size_t id = 0; id < _relays.size(); ++id) {
        const bool target { _relays[id].target_status };
        if (target != mode) {
            continue;
        }

        if ((target != _relays[id].current_status) || (_relays[id].flags & RelayFlagBoot)) {
            _relays[id].current_status = target;
            _relays[id].provider->change(target);

            // making sure any pending flags do not change the behaviour of the funcs below
            auto flags = _relays[id].flags;
            _relays[id].flags = 0;

            // before initial transition happens, relay should not be globally accessible
            if (flags & RelayFlagBoot) {
                flags &= ~RelayFlagBoot;
                _relayActive(id, target);
            }

            // persist status after provider applied it
            _relayScheduleSave(id);

            // try to immediately schedule 'normal' state
            _relayProcessTimer(id, _relays[id].pulse, _relays[id].pulse_time, target);

            // and report to everything else, including debug logs
            _relayReport(id, target, flags);

            changed = true;
        }
    }

    return changed;
}

} // namespace

//------------------------------------------------------------------------------
// Setup
//------------------------------------------------------------------------------

namespace {

void _relayLoop() {
    const bool changed[] {
        _relayProcess(false),
        _relayProcess(true),
    };

    if (changed[0] || changed[1]) {
        _relayPrepareUnlock();
    }

    _relayProcessUnlock();

    _relayReport();
    _relaySave();
}

} // namespace

// Dummy relays for virtual light switches (hardware-less), Sonoff Dual, Sonoff RF Bridge and Tuya

void relaySetupDummy(size_t size, bool reconfigure) {
    if (size == _relays_dummy) {
        return;
    }

    const size_t new_size = ((_relays.size() - _relays_dummy) + size);
    if (new_size > RelaysMax) {
        return;
    }

    _relays_dummy = size;
    _relays.resize(new_size);

    if (reconfigure) {
        _relayConfigure();
    }
}

namespace {

constexpr size_t _relayAdhocPins() {
    return 0
    #if RELAY1_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY2_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY3_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY4_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY5_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY6_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY7_PIN != GPIO_NONE
        + 1
    #endif
    #if RELAY8_PIN != GPIO_NONE
        + 1
    #endif
    ;
}

struct RelayGpioProviderCfg {
    GpioType type;
    uint8_t main;
    uint8_t reset;
};

RelayGpioProviderCfg _relayGpioProviderCfg(size_t index) {
    return RelayGpioProviderCfg{
        .type = espurna::relay::settings::pinType(index),
        .main = espurna::relay::settings::pin(index),
        .reset = espurna::relay::settings::resetPin(index),
    };
}

std::unique_ptr<GpioProvider> _relayGpioProvider(size_t index, RelayType type) {
    const auto cfg = _relayGpioProviderCfg(index);

    auto* base = gpioBase(cfg.type);
    if (!base) {
        return nullptr;
    }

    auto main = gpioRegister(*base, cfg.main);
    if (!main) {
        return nullptr;
    }

    auto reset = gpioRegister(*base, cfg.reset);
    if (GpioType::Hardware == cfg.type) {
        hardwareGpioIgnore(cfg.main);
        if (GPIO_NONE != cfg.reset) {
            hardwareGpioIgnore(cfg.reset);
        }
    }

    return std::make_unique<GpioProvider>(
        type, std::move(main), std::move(reset));
}

struct RelayProviderResult {
    RelayProvider provider{ RelayProvider::None };
    RelayProviderBasePtr ptr;
};

RelayProviderResult _relaySetupProvider(size_t index) {
    auto provider = espurna::relay::settings::provider(index);
    auto type = espurna::relay::settings::type(index);

    RelayProviderBasePtr ptr;

    switch (provider) {
    case RelayProvider::Dummy:
        ptr = std::make_unique<DummyProvider>();
        break;

    case RelayProvider::Gpio:
        ptr = _relayGpioProvider(index, type);
        break;

    case RelayProvider::Stm:
#if RELAY_PROVIDER_STM_SUPPORT
        ptr = std::make_unique<StmProvider>(index);
#endif
        break;

    case RelayProvider::Dual:
#if RELAY_PROVIDER_DUAL_SUPPORT
        ptr = std::make_unique<DualProvider>(index);
#endif
        break;

    case RelayProvider::LightState:
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        ptr = lightMakeStateRelayProvider(index);
#endif
        break;

    case RelayProvider::Fan:
#if FAN_SUPPORT
        ptr = fanMakeRelayProvider(index);
#endif
        break;

    case RelayProvider::Lightfox:
#ifdef FOXEL_LIGHTFOX_DUAL
        ptr = lightfoxMakeRelayProvider(index);
#endif
        break;

    case RelayProvider::Tuya:
#if TUYA_SUPPORT
        ptr = tuya::makeRelayProvider(index);
#endif
        break;

    case RelayProvider::None:
        break;
    }

    RelayProviderResult result;
    if (ptr) {
        result.provider = provider;
        result.ptr = std::move(ptr);
    }

    return result;
}

void _relaySetup() {
    auto relays = _relays.size();
    _relays.reserve(relays + _relayAdhocPins());

    for (size_t id = relays; id < RelaysMax; ++id) {
        auto result = _relaySetupProvider(id);
        if (!result.ptr) {
            break;
        }

        if (!result.ptr->setup()) {
            break;
        }

        auto relay = Relay(std::move(result.ptr));
        if (RelayProvider::Gpio == result.provider) {
            _relays_retained[id] = true;
        }

        _relays.emplace_back(std::move(relay));
    }

    relaySetupDummy(espurna::relay::settings::dummyCount());
}

} // namespace

namespace espurna {
namespace relay {
namespace settings {
namespace query {
namespace {

bool checkSamePrefix(StringView key) {
    return key.startsWith(Prefix);
}

espurna::settings::query::Result findFromIndexed(StringView key) {
    return espurna::settings::query::findFrom(_relays.size(), IndexedSettings, key);
}

espurna::settings::query::Result findFrom(StringView key) {
    return espurna::settings::query::findFrom(Settings, key);
}

void setup() {
    ::settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findFromIndexed,
    });

    ::settingsRegisterQueryHandler({
        .check = nullptr,
        .get = findFrom,
    });
}

} // namespace
} // namespace query
} // namespace settings
} // namespace relay
} // namespace espurna

void relaySetup() {
    migrateVersion(_relaySettingsMigrate);

    _relaySetup();
    espurna::relay::settings::query::setup();

    _relayConfigure();
    _relayBootAll();
    _relayLoop();

    #if WEB_SUPPORT
        _relaySetupWeb();
    #endif
    #if API_SUPPORT
        _relaySetupApi();
    #endif
    #if MQTT_SUPPORT
        _relaySetupMqtt();
    #endif
    #if TERMINAL_SUPPORT
        _relaySetupTerminal();
    #endif

    espurnaRegisterLoop(_relayLoop);
    espurnaRegisterReload(_relayConfigure);
}

RelayAddResult relayAdd(RelayProviderBasePtr&& provider) {
    RelayAddResult out;

    const auto id = _relays.size();
    if (espurna::relay::settings::provider(id) != RelayProvider::None) {
        return out;
    }

    if (!provider) {
        return out;
    }

    if (!provider->setup()) {
        return out;
    }

    out.id = id;
    _relays.emplace_back(std::move(provider));
    espurnaRegisterOnceUnique([]() {
        _relayConfigure();
        _relayBootAll();
    });

    return out;
}

#endif // RELAY_SUPPORT == 1
