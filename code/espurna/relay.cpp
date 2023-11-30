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

namespace espurna {
namespace relay {
namespace flood {

using Duration = espurna::duration::Milliseconds;
using Seconds = std::chrono::duration<float>;

namespace build {
namespace {

constexpr Duration window() {
    static_assert(Seconds{RELAY_FLOOD_WINDOW}.count() >= 0.0f, "");
    return std::chrono::duration_cast<Duration>(Seconds { RELAY_FLOOD_WINDOW });
}

constexpr unsigned long changes() {
    return RELAY_FLOOD_CHANGES;
}

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
    return getSetting(keys::Time, build::window());
}

unsigned long changes() {
    return getSetting(keys::Changes, build::changes());
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

constexpr RelaySync syncMode() {
    return RELAY_SYNC;
}

constexpr espurna::duration::Milliseconds latchingPulse() {
    return espurna::duration::Milliseconds(RELAY_LATCHING_PULSE);
}

constexpr espurna::duration::Milliseconds interlockDelay() {
    return espurna::duration::Milliseconds(RELAY_DELAY_INTERLOCK);
}

constexpr espurna::duration::Milliseconds delayOn(size_t index) {
    return espurna::duration::Milliseconds(
        (index == 0) ? RELAY1_DELAY_ON :
        (index == 1) ? RELAY2_DELAY_ON :
        (index == 2) ? RELAY3_DELAY_ON :
        (index == 3) ? RELAY4_DELAY_ON :
        (index == 4) ? RELAY5_DELAY_ON :
        (index == 5) ? RELAY6_DELAY_ON :
        (index == 6) ? RELAY7_DELAY_ON :
        (index == 7) ? RELAY8_DELAY_ON : 0ul
    );
}

constexpr espurna::duration::Milliseconds delayOff(size_t index) {
    return espurna::duration::Milliseconds(
        (index == 0) ? RELAY1_DELAY_OFF :
        (index == 1) ? RELAY2_DELAY_OFF :
        (index == 2) ? RELAY3_DELAY_OFF :
        (index == 3) ? RELAY4_DELAY_OFF :
        (index == 4) ? RELAY5_DELAY_OFF :
        (index == 5) ? RELAY6_DELAY_OFF :
        (index == 6) ? RELAY7_DELAY_OFF :
        (index == 7) ? RELAY8_DELAY_OFF : 0ul
    );
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

#endif

} // namespace
} // namespace build

namespace pulse {

using Duration = espurna::duration::Milliseconds;
using Seconds = std::chrono::duration<float>;

enum class Mode {
    None,
    Off,
    On
};

} // namespace pulse
} // namespace relay
} // namespace espurna

namespace espurna {
namespace relay {
namespace pulse {
namespace build {
namespace {

constexpr Seconds time(size_t index) {
    return Seconds(
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

static_assert(time(0).count() >= 0.0f, "");
static_assert(time(1).count() >= 0.0f, "");
static_assert(time(2).count() >= 0.0f, "");
static_assert(time(3).count() >= 0.0f, "");
static_assert(time(4).count() >= 0.0f, "");
static_assert(time(5).count() >= 0.0f, "");
static_assert(time(6).count() >= 0.0f, "");
static_assert(time(7).count() >= 0.0f, "");

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

} // namespace
} // namespace build

namespace settings {
namespace keys {
namespace {

PROGMEM_STRING(Time, "relayTime");
PROGMEM_STRING(Mode, "relayPulse");

} // namespace
} // namespace keys

namespace {

using ParseResult = espurna::settings::internal::duration_convert::Result;

Duration native_duration(ParseResult result) {
    using namespace espurna::settings::internal;

    if (result.ok) {
        return duration_convert::to_chrono_duration<Duration>(result.value);
    }

    return Duration::min();
}

ParseResult parse_time(StringView view) {
    using namespace espurna::settings::internal;
    return duration_convert::parse(view, Seconds::period{});
}

Duration native_duration(StringView view) {
    return native_duration(parse_time(view));
}

Duration time(size_t index) {
    const auto time = espurna::settings::get(
        espurna::settings::Key{keys::Time, index}.value());

    if (!time) {
        return std::chrono::duration_cast<Duration>(build::time(index));
    }

    return native_duration(time.view());
}

Mode mode(size_t index) {
    return getSetting({keys::Mode, index}, build::mode(index));
}

} // namespace
} // namespace settings

namespace {

struct Timer {
    using Duration = timer::SystemTimer::Duration;

    Timer() = delete;
    Timer(const Timer&) = delete;
    Timer(Timer&&) = delete;

    Timer(Duration duration, size_t id, bool status) :
        _duration(duration),
        _id(id),
        _status(status)
    {}

    ~Timer() {
        _timer.stop();
    }

    Timer& operator=(const Timer&) = delete;
    Timer& operator=(Timer&&) = delete;

    explicit operator bool() const {
        return static_cast<bool>(_timer);
    }

    bool operator==(const Timer& other) const {
        return (_duration == other._duration)
            && (_id == other._id)
            && (_status == other._status);
    }

    Timer& update(Duration duration, bool status) {
        stop();
        _duration = duration;
        _status = status;
        return *this;
    }

    size_t id() const {
        return _id;
    }

    Duration duration() const {
        return _duration;
    }

    bool status() const {
        return _status;
    }

    void stop() {
        _timer.stop();
    }

    void start() {
        const auto id = _id;
        const auto status = _status;
        _timer.once(
            (_duration.count() > 0)
                ? _duration
                : timer::SystemTimer::DurationMin,
            [id, status]() {
                relayStatus(id, status);
            });
    }

private:
    Duration _duration;
    size_t _id;
    bool _status;

    timer::SystemTimer _timer;
};

namespace internal {

std::forward_list<Timer> timers;

} // namespace internal

auto find(size_t id) -> decltype(internal::timers.begin()) {
    return std::find_if(
        internal::timers.begin(),
        internal::timers.end(),
        [&](const Timer& timer) {
            return id == timer.id();
        });
}

void trigger(Duration duration, size_t id, bool target) {
    if (duration.count() == 0) {
        bool found { false };
        internal::timers.remove_if(
            [&](const Timer& timer) {
                if (id == timer.id()) {
                    found = true;
                    return true;
                }

                return false;
            });

        if (found) {
            DEBUG_MSG_P(PSTR("[RELAY] #%zu pulse stopped\n"), id);
        }

        return;
    }

    bool rescheduled [[gnu::unused]] { false };
    auto it = find(id);
    if (it != internal::timers.end()) {
        (*it).update(duration, target);
        rescheduled = true;
    } else {
        internal::timers.emplace_front(duration, id, target);
        it = internal::timers.begin();
    }

    (*it).start();

    DEBUG_MSG_P(PSTR("[RELAY] #%zu pulse %s %sscheduled in %lu (ms)\n"),
        id, target ? "ON" : "OFF",
        rescheduled ? "re" : "",
        duration.count());
}

// Update the pulse counter when the relay is already in the opposite state (#454)
void poll(size_t id, bool target) {
    auto it = find(id);
    if ((it != internal::timers.end()) && ((*it).status() != target)) {
        (*it).start();
    }
}

void expire() {
    internal::timers.remove_if([](const Timer& timer) {
        return !static_cast<bool>(timer)
            || (relayStatus(timer.id()) == timer.status());
    });
}

[[gnu::unused]]
Seconds findDuration(size_t id) {
    Seconds out{};

    auto it = find(id);
    if (it != internal::timers.end()) {
        out = std::chrono::duration_cast<Seconds>((*it).duration());
    }

    return out;
}

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

bool isActive(Mode pulse) {
    return pulse != Mode::None;
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

using RelayMask = std::bitset<RelaysMax>;

struct RelayMaskHelper {
    using IntegralType = uint32_t;
    static_assert(RelaysMax <= (sizeof(IntegralType) * 8), "");

    RelayMaskHelper() = default;
    RelayMaskHelper(const RelayMaskHelper&) = default;
    RelayMaskHelper(RelayMaskHelper&&) = default;

    explicit RelayMaskHelper(RelayMask mask) noexcept :
        _mask(mask)
    {}

    explicit RelayMaskHelper(IntegralType mask) noexcept :
        _mask(mask)
    {}

    IntegralType toUnsigned() const {
        return _mask.to_ulong();
    }

    String toString() const {
        return formatUnsigned(toUnsigned(), 2);
    }

    const RelayMask& mask() const {
        return _mask;
    }

    void reset() {
        _mask.reset();
    }

    void set(size_t id, bool status) {
        _mask.set(id, status);
    }

    bool operator[](size_t id) const {
        return _mask[id];
    }

private:
    RelayMask _mask {};
};

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
    return RelayMaskHelper { convert<RelayMaskHelper::IntegralType>(value) };
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
PROGMEM_STRING(MqttDisconnection, "relayMqttDisc");
#endif

PROGMEM_STRING(Dummy, "relayDummy");
PROGMEM_STRING(BootMask, "relayBootMask");
PROGMEM_STRING(Interlock, "relayIlkDelay");
PROGMEM_STRING(Sync, "relaySync");

PROGMEM_STRING(PayloadOn, "relayPayloadOn");
PROGMEM_STRING(PayloadOff, "relayPayloadOff");
PROGMEM_STRING(PayloadToggle, "relayPayloadToggle");

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
    static const RelayMaskHelper defaultMask;
    return getSetting(keys::BootMask, defaultMask);
}

void bootMask(const String& mask) {
    setSetting(keys::BootMask, mask);
}

void bootMask(const RelayMaskHelper& mask) {
    bootMask(mask.toString());
}

espurna::duration::Milliseconds delayOn(size_t index) {
    return getSetting({keys::DelayOn, index}, build::delayOn(index));
}

espurna::duration::Milliseconds delayOff(size_t index) {
    return getSetting({keys::DelayOff, index}, build::delayOff(index));
}

espurna::duration::Milliseconds interlockDelay() {
    return getSetting(keys::Interlock, build::interlockDelay());
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

PayloadStatus mqttDisconnectionStatus(size_t index) {
    return getSetting({keys::MqttDisconnection, index}, build::mqttDisconnectionStatus(index));
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
        std::chrono::duration_cast<pulse::Seconds>(result);

    return espurna::settings::internal::serialize(as_seconds.count());
}

#if MQTT_SUPPORT
ID_VALUE(mqttDisconnectionStatus, settings::mqttDisconnectionStatus)
ID_VALUE(mqttTopicMode, settings::mqttTopicMode)
#endif

#undef ID_VALUE
#undef EXACT_VALUE

} // namespace internal

static constexpr espurna::settings::query::Setting Settings[] PROGMEM {
    {keys::Dummy, internal::dummyCount},
    {keys::BootMask, internal::bootMask},
    {keys::Interlock, internal::interlockDelay},
    {keys::Sync, internal::syncMode}
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
    {keys::MqttDisconnection, internal::mqttDisconnectionStatus},
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
    using Delay = espurna::duration::Milliseconds;
    using TimePoint = TimeSource::time_point;

    using PulseMode = espurna::relay::pulse::Mode;
    using PulseTime = espurna::relay::pulse::Duration;

    Relay() = default;

    explicit Relay(RelayProviderBasePtr&& ptr) :
        provider(ptr.release())
    {}

    explicit Relay(RelayProviderBase* ptr) :
        provider(ptr)
    {}

    // ON / OFF actions implementation
    // Defaults to 'DummyProvider', since we don't want to check provider existence every time
    RelayProviderBase* provider { DummyProvider::sharedInstance() };

    // *Before* changing status, waits for the specified time (ms)
    Delay delay_on { 0ul };
    Delay delay_off { 0ul };

    // *After* changing status, checks whether we should remain in it
    // If not, starts a timer for the specified time (ms)
    PulseMode pulse { PulseMode::None };
    PulseTime pulse_time { 0ul };

    // Flood window start time
    TimePoint fw_start{};

    // Number of changes within the current flood window
    unsigned char fw_count { 0u };

    // Time when relay was scheduled to change
    TimePoint change_start{};

    // Delay until the next change
    Delay change_delay { 0ul };

    // Already applied status (passed to provider) and a pending one
    bool current_status { false };
    bool target_status { false };

    // Holds the value of target status that persists and cannot be changed from
    RelayLock lock { RelayLock::None };

    // MQTT report on relay and / or group topic
    bool report { false };
    bool group_report { false };
};

namespace {

struct RelaySaveTimer {
    using Timer = espurna::timer::SystemTimer;
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

        if (duration.count()) {
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
size_t _relayDummy { 0ul };

espurna::duration::Milliseconds _relay_flood_window { espurna::relay::flood::build::window() };
unsigned long _relay_flood_changes { espurna::relay::flood::build::changes() };

espurna::duration::Milliseconds _relay_delay_interlock;
RelaySync _relay_sync_mode { RelaySync::None };
bool _relay_sync_reent { false };

struct RelaySyncUnlock {
    void push_back(RelayLock lock) {
        _locks.push_back(lock);
    }

    void clear() {
        _locks.clear();
    }

    explicit operator bool() const {
        return _locks.size() > 0;
    }

    template <typename T>
    void operator()(T&& relays) const {
        for (size_t id = 0; id < _locks.size(); ++id) {
            relays[id].lock = _locks[id];
        }
    }

private:
    std::vector<RelayLock> _locks;
};

RelayDelayedTimer _relay_unlock_timer;
RelaySaveTimer _relay_save_timer;

std::forward_list<RelayStatusCallback> _relay_status_notify;
std::forward_list<RelayStatusCallback> _relay_status_change;

#if WEB_SUPPORT

bool _relay_report_ws { false };

void _relayScheduleWsReport() {
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

void relayOnStatusNotify(RelayStatusCallback callback) {
    _relay_status_notify.push_front(callback);
}

void relayOnStatusChange(RelayStatusCallback callback) {
    _relay_status_change.push_front(callback);
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
        RelayMaskHelper mask;
        for (size_t index = 0; index < _instances.size(); ++index) {
            bool status { relayStatus(_instances[index]->relayId()) };
            sync = sync && status;
            mask.set(index, status);
        }

        if (sync) {
            mask.reset();
            mask.set(_instances.size(), true);
        }

        DEBUG_MSG_P(PSTR("[RELAY] Sending DUAL mask: %s\n"), mask.toString().c_str());

        uint8_t buffer[4] { 0xa0, 0x04, static_cast<unsigned char>(mask.toUnsigned()), 0xa1 };
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
        RelayMaskHelper mask(bytes[2]);
        if (mask[_instances.size()]) {
            for (auto& instance : _instances) {
                relayStatus(instance->relayId(), true);
            }
            return;
        }

        // Then, manage relays individually
        for (size_t index = 0; index < _instances.size(); ++index) {
            relayStatus(_instances[index]->relayId(), mask[index]);
        }
    }

private:
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

    void boot(bool) override {
        // XXX: does this actually help with anything? remains as part of the
        // original implementation, quoting "because of broken stm relay firmware"
        _relays[_id].change_delay = espurna::duration::Seconds(3) + espurna::duration::Seconds(1) * _id;
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
    return tryParseId(value, relayCount(), id);
}

[[gnu::unused]]
bool _relayTryParseIdFromPath(espurna::StringView value, size_t& id) {
    return tryParseIdPath(value, relayCount(), id);
}

void _relayHandleStatus(size_t id, PayloadStatus status) {
    switch (status) {
    case PayloadStatus::Off:
        relayStatus(id, false);
        break;
    case PayloadStatus::On:
        relayStatus(id, true);
        break;
    case PayloadStatus::Toggle:
        relayToggle(id);
        break;
    case PayloadStatus::Unknown:
        break;
    }
}

[[gnu::unused]]
bool _relayHandlePayload(size_t id, espurna::StringView payload) {
    const auto status = relayParsePayload(payload);
    if (status != PayloadStatus::Unknown) {
        _relayHandleStatus(id, status);
        return true;
    }

    return false;
}

// Initialize pulse timers after ON or OFF event
// TODO: integrate with scheduled ON or OFF?

bool _relayPulseActive(size_t id, bool status) {
    using namespace espurna::relay::pulse;
    if (isActive(_relays[id].pulse)) {
        return isNormalStatus(_relays[id].pulse, status);
    }

    return false;
}

void _relayProcessActivePulse(const Relay& relay, size_t id, bool status) {
    using namespace espurna::relay::pulse;
    if (isActive(relay.pulse) && !isNormalStatus(relay.pulse, status)) {
        trigger(relay.pulse_time, id, !status);
    }
}

// start pulse for the current status as 'target'
[[gnu::unused]]
bool _relayHandlePulsePayload(size_t id, espurna::StringView payload) {
    const auto status = relayStatus(id);
    if (_relayPulseActive(id, status)) {
        return false;
    }

    using namespace espurna::relay::pulse::settings;
    const auto pulse = parse_time(payload);

    if (pulse.ok) {
        espurna::relay::pulse::trigger(native_duration(pulse), id, status);
        relayToggle(id, true, false);

        return true;
    }

    return false;
}

// Make sure expired pulse timers are removed, so any API calls don't try to re-use those
void _relayProcessPulse() {
    espurna::relay::pulse::expire();
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
    if (id < _relays.size()) {
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

[[gnu::unused]]
bool _relayHandleLockPayload(size_t id, espurna::StringView payload) {
    if (id < _relays.size()) {
        const auto status = relayStatusTarget(id);
        if (relayStatus(id) != status) {
            return false;
        }

        const auto lock = _relayTristateFromPayload<RelayLock>(payload.toString());
        _relays[id].lock = lock;

        switch (lock) {
        case RelayLock::None:
            relayStatus(id, status);
            break;
        case RelayLock::Off:
        case RelayLock::On:
            relayStatus(id, (RelayLock::On == lock));
            break;
        }

        return true;
    }

    return false;
}

void _relaySyncLockAll() {
    RelaySyncUnlock locks;

    for (auto& relay : _relays) {
        const auto lock = relay.lock;
        locks.push_back(lock);

        if (lock == RelayLock::None) {
            relay.lock = relay.target_status
                ? RelayLock::On
                : RelayLock::Off;
        }
    }

    _relay_unlock_timer.prepare(
        [locks]() {
            locks(_relays);
#if WEB_SUPPORT
            _relayScheduleWsReport();
#endif
        });
}

bool _relayStatusCheckLock(Relay& relay, bool status) {
    if (relay.lock != RelayLock::None) {
        bool lock = relay.lock == RelayLock::On;
        if ((lock != status) || (lock != relay.target_status)) {
            relay.target_status = lock;
            relay.change_delay = Relay::Delay::zero();
            return false;
        }
    }

    return true;
}

// https://github.com/xoseperez/espurna/issues/1510#issuecomment-461894516
// completely reset timing on the other relay to sync with this one
// to ensure that they change state sequentially
void _relaySyncRelaysDelay(size_t first, size_t second) {
    _relays[second].fw_start = _relays[first].change_start;
    _relays[second].fw_count = 1;
    _relays[second].change_delay = std::max({
        _relay_delay_interlock,
        _relays[first].change_delay,
        _relays[second].change_delay
    });
}

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
                : espurna::duration::Milliseconds{0});
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
    return RelayMaskHelper(Rtcmem->relay);
}

inline void _relayMaskRtcmem(uint32_t mask) {
    Rtcmem->relay = mask;
}

inline void _relayMaskRtcmem(const RelayMaskHelper& mask) {
    _relayMaskRtcmem(mask.toUnsigned());
}

} // namespace

void relayPulse(size_t id, espurna::duration::Milliseconds duration, bool normal) {
    if (id < _relays.size()) {
        relayStatus(id, normal);
        espurna::relay::pulse::trigger(duration, id, !relayStatus(id));
    }
}

void relayPulse(size_t id, espurna::duration::Milliseconds duration) {
    relayPulse(id, duration, !relayStatus(id));
}

void relayPulse(size_t id) {
    if (id < _relays.size()) {
        espurna::relay::pulse::trigger(_relays[id].pulse_time, id, _relays[id].current_status);
    }
}

// -----------------------------------------------------------------------------

namespace {

void _relaySync(size_t target) {
    // No sync if none or only one relay
    const auto relays = _relays.size();
    if (relays < 2) {
        return;
    }

    // Only call once when coming from the relayStatus(id, status)
    auto lock = espurna::ReentryLock{ _relay_sync_reent };
    if (!lock) {
        return;
    }

    bool status = _relays[target].target_status;

    switch (_relay_sync_mode) {
    case RelaySync::None:
        break;

    // aka all relays should have the same status
    case RelaySync::All:
        for (size_t id = 0; id < relays; ++id) {
            if (id != target) {
                relayStatus(id, status);
            }
        }
        break;

    // all relays should copy the first relay status
    case RelaySync::First:
        if (target == 0) {
            for (size_t id = 1; id < relays; ++id) {
                relayStatus(id, status);
            }
        }
        break;

    // If any of the 'One' modes and setting ON we should set OFF all the others
    case RelaySync::ZeroOrOne:
    case RelaySync::JustOne:
        if (status) {
            for (size_t id = 0; id < relays; ++id) {
                if (id != target) {
                    relayStatus(id, false);
                    if (relayStatus(id)) {
                        _relaySyncRelaysDelay(id, target);
                    }
                }
            }
        // If we only need a single one and setting OFF we should set ON the other one
        } else if (_relay_sync_mode == RelaySync::JustOne) {
            const auto id = (target + 1) % relays;
            _relaySyncRelaysDelay(target, id);
            relayStatus(id, true);
        }

        _relaySyncLockAll();
        break;
    }
}

bool _relayStatus(size_t id, bool status, bool report, bool group_report) {
    auto& relay = _relays[id];

    if (!_relayStatusCheckLock(relay, status)) {
        DEBUG_MSG_P(PSTR("[RELAY] #%u is locked to %s\n"),
            id, relay.current_status ? PSTR("ON") : PSTR("OFF"));
        relay.report = true;
        relay.group_report = true;
        return false;
    }

    bool changed { false };

    if (relay.current_status == status) {
        if (relay.target_status != status) {
            DEBUG_MSG_P(PSTR("[RELAY] #%u scheduled change cancelled\n"), id);
            relay.target_status = status;
            relay.report = false;
            relay.group_report = false;
            relay.change_delay = Relay::Delay::zero();
            changed = true;
        }

        relay.provider->notify(status);
        for (auto& notify : _relay_status_notify) {
            notify(id, status);
        }

        espurna::relay::pulse::poll(id, status);
    } else {
        auto current_time = Relay::TimeSource::now();
        auto change_delay = status
            ? relay.delay_on
            : relay.delay_off;

        relay.fw_count++;
        relay.change_start = current_time;
        relay.change_delay = std::max(relay.change_delay, change_delay);

        // If current_time is off-limits the floodWindow...
        const auto fw_diff = current_time - relay.fw_start;
        if (fw_diff > _relay_flood_window) {
            // We reset the floodWindow
            relay.fw_start = current_time;
            relay.fw_count = 1;

        // If current_time is in the floodWindow and there have been too many requests...
        } else if (relay.fw_count >= _relay_flood_changes) {

            // We schedule the changes to the end of the floodWindow
            // unless it's already delayed beyond that point
            relay.change_delay = std::max(change_delay, _relay_flood_window - fw_diff);

            // Another option is to always move it forward, starting from current time
            // relay.fw_start = current_time;
        }

        relay.target_status = status;
        relay.report = report;
        relay.group_report = group_report;

        _relaySync(id);
        changed = true;

        if (relay.change_delay.count()) {
            DEBUG_MSG_P(PSTR("[RELAY] #%u scheduled %s in %u (ms)\n"),
                id, status ? PSTR("ON") : PSTR("OFF"), relay.change_delay.count());
        }
    }

    return changed;
}

} // namespace

bool relayStatus(size_t id, bool status, bool report, bool group_report) {
    if (id < _relays.size()) {
        return _relayStatus(id, status, report, group_report);
    }

    return false;
}

bool relayStatus(size_t id, bool status) {
#if MQTT_SUPPORT
    return relayStatus(id, status, mqttForward(), true);
#else
    return relayStatus(id, status, false, true);
#endif
}

bool relayStatus(size_t id) {
    if (id < _relays.size()) {
        return _relays[id].current_status;
    }

    return false;
}

bool relayStatusTarget(size_t id) {
    if (id >= _relays.size()) return false;
    return _relays[id].target_status;
}

namespace {

RelayMaskHelper _relayMaskCurrent() {
    RelayMaskHelper mask;
    for (size_t id = 0; id < _relays.size(); ++id) {
        mask.set(id, _relays[id].current_status);
    }
    return mask;
}

void _relaySave(bool persist) {
    const auto mask = _relayMaskCurrent();
    if (_relays.size() > 1) {
        DEBUG_MSG_P(PSTR("[RELAY] Relay mask: %s\n"), mask.toString().c_str());
    }

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
        eepromCommit(); // TODO: should this respect settings auto-save?
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

void relaySave(bool persist) {
    _relaySave(persist);
}

void relaySave() {
    _relaySave(false);
}

void relayToggle(size_t id, bool report, bool group_report) {
    if (id < _relays.size()) {
        relayStatus(id, !relayStatus(id), report, group_report);
    }
}

void relayToggle(size_t id) {
#if MQTT_SUPPORT
    relayToggle(id, mqttForward(), true);
#else
    relayToggle(id, false, true);
#endif
}

size_t relayCount() {
    return _relays.size();
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

void _relayBoot(size_t index, const RelayMaskHelper& mask) {
    auto status = false;

    auto lock = RelayLock::None;

    switch (espurna::relay::settings::bootMode(index)) {
    case RelayBoot::Same:
        status = mask[index];
        break;
    case RelayBoot::Toggle:
        status = !mask[index];
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

    auto& relay = _relays[index];

    relay.current_status = !status;
    relay.target_status = status;

    relay.lock = lock;

    relay.change_start = Relay::TimeSource::now();
    relay.change_delay = status
        ? relay.delay_on
        : relay.delay_off;

    relay.provider->boot(status);
}

void _relayBootAll() {
    auto mask = rtcmemStatus()
        ? _relayMaskRtcmem()
        : espurna::relay::settings::bootMask();

    bool log { false };

    static RelayMask done;
    const auto relays = _relays.size();
    for (size_t id = 0; id < relays; ++id) {
        if (!done[id]) {
            done.set(id, true);
            _relayBoot(id, mask);
            log = true;
        }
    }

    if (log) {
        DEBUG_MSG_P(PSTR("[RELAY] Number of relays: %u, boot mask: %s\n"),
            relays, mask.toString().c_str());
    }
}

void _relayConfigure() {
    for (size_t id = 0; id < _relays.size(); ++id) {
        auto& relay = _relays[id];

        relay.pulse = espurna::relay::pulse::settings::mode(id);
        relay.pulse_time = (relay.pulse != espurna::relay::pulse::Mode::None)
            ? espurna::relay::pulse::settings::time(id)
            : espurna::relay::pulse::Duration::min();

        relay.delay_on = espurna::relay::settings::delayOn(id);
        relay.delay_off = espurna::relay::settings::delayOff(id);
    }

    _relay_flood_window = espurna::relay::flood::settings::window();
    _relay_flood_changes = espurna::relay::flood::settings::changes();

    _relay_delay_interlock = espurna::relay::settings::interlockDelay();
    _relay_sync_mode = espurna::relay::settings::syncMode();

#if MQTT_SUPPORT || API_SUPPORT
    _relay_payload_on = espurna::relay::settings::payloadOn();
    _relay_payload_off = espurna::relay::settings::payloadOff();
    _relay_payload_toggle = espurna::relay::settings::payloadToggle();
#endif // MQTT_SUPPORT
}

} // namespace

//------------------------------------------------------------------------------
// WEBSOCKETS
//------------------------------------------------------------------------------

#if WEB_SUPPORT

namespace {

bool _relayWebSocketOnKeyCheck(espurna::StringView key, const JsonVariant&) {
    return espurna::settings::query::samePrefix(key, STRING_VIEW("relay"));
}

void _relayWebSocketUpdate(JsonObject& root) {
    espurna::web::ws::EnumerablePayload payload{root, STRING_VIEW("relayState")};
    payload(STRING_VIEW("states"), _relays.size(), {
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

    config(STRING_VIEW("relays"), _relays.size(),
        espurna::relay::settings::query::IndexedSettings);
}

void _relayWebSocketOnVisible(JsonObject& root) {
    const auto relays = _relays.size();
    if (!relays) {
        return;
    }

    if (relays > 1) {
        wsPayloadModule(root, PSTR("multirelay"));
        root[FPSTR(espurna::relay::settings::keys::Sync)] =
            espurna::settings::internal::serialize(espurna::relay::settings::syncMode());
        root[FPSTR(espurna::relay::settings::keys::Interlock)] =
            espurna::relay::settings::interlockDelay().count();
    }

    wsPayloadModule(root, PSTR("relay"));
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

void _relayWsReport() {
    if (_relay_report_ws) {
        wsPost(_relayWebSocketUpdate);
        _relay_report_ws = false;
    }
}

} // namespace

void relaySetupWS() {
    wsRegister()
        .onVisible(_relayWebSocketOnVisible)
        .onConnected(_relayWebSocketOnConnected)
        .onData(_relayWebSocketUpdate)
        .onAction(_relayWebSocketOnAction)
        .onKeyCheck(_relayWebSocketOnKeyCheck);
}

#endif // WEB_SUPPORT

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

} // namespace

void relaySetupAPI() {

    if (!_relays.size()) {
        return;
    }

    apiRegister(F(MQTT_TOPIC_RELAY),
        [](ApiRequest&, JsonObject& root) {
            JsonArray& out = root.createNestedArray("relayStatus");
            for (auto& relay : _relays) {
                out.add(relay.target_status ? 1 : 0);
            }
            return true;
        },
        nullptr
    );

    apiRegister(F(MQTT_TOPIC_RELAY "/+"),
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

    apiRegister(F(MQTT_TOPIC_PULSE "/+"),
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                using namespace espurna::relay::pulse;
                const auto duration = findDuration(id);
                const auto seconds = std::chrono::duration_cast<Seconds>(duration);
                request.send(String(seconds.count(), 10));
                return true;
            });
        },
        [](ApiRequest& request) {
            return _relayApiTryHandle(request, [&](size_t id) {
                return _relayHandlePulsePayload(id, request.param(F("value")));
            });
        }
    );

    apiRegister(F(MQTT_TOPIC_LOCK "/+"),
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

std::forward_list<RelayCustomTopic> _relay_custom_topics;

void _relayMqttSubscribeCustomTopics() {
    const size_t relays { _relays.size() };
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

void _relayMqttReport(size_t id) {
    if (_relays[id].report) {
        _relays[id].report = false;
        mqttSend(MQTT_TOPIC_RELAY, id, relayPayload(_relayPayloadStatus(id)).c_str()); // TODO FIXED LENGTH
    }

    if (_relays[id].group_report) {
        _relays[id].group_report = false;
        _relayMqttPublishCustomTopic(id);
    }
}

void _relayMqttReportAll() {
    for (unsigned int id=0; id < _relays.size(); id++) {
        mqttSend(MQTT_TOPIC_RELAY, id, relayPayload(_relayPayloadStatus(id)).c_str()); // TODO FIXED LENGTH
    }
}

void _relayMqttReportDescription() {
    static const char Topic[] = MQTT_TOPIC_DESCRIPTION "/" MQTT_TOPIC_RELAY;
    for (size_t id = 0; id < _relays.size(); ++id) {
        const auto name = espurna::relay::settings::name(id);
        if (name.length()) {
            mqttSend(Topic, id, name.c_str());
        }
    }
}

} // namespace

void relayStatusWrap(size_t id, PayloadStatus value, bool is_group_topic) {
    #if MQTT_SUPPORT
        const auto forward = mqttForward();
    #else
        const auto forward = false;
    #endif
    switch (value) {
        case PayloadStatus::Off:
            relayStatus(id, false, forward, !is_group_topic);
            break;
        case PayloadStatus::On:
            relayStatus(id, true, forward, !is_group_topic);
            break;
        case PayloadStatus::Toggle:
            relayToggle(id, true, true);
            break;
        case PayloadStatus::Unknown:
        default:
            _relays[id].report = true;
            _relayMqttReport(id);
            break;
    }
}

namespace {

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

            const auto id = topic.id();
            _relayHandleStatus(id, status);
            _relays[id].group_report = false;
        }
    }
}

void _relayMqttHandleDisconnect() {
    using namespace espurna::relay::settings;
    for (size_t id = 0; id < _relays.size(); ++id) {
        _relayHandleStatus(id, mqttDisconnectionStatus(id));
    }
}

struct RelayMqttTopicHandler {
    using Handler = bool(*)(size_t, espurna::StringView);
    espurna::StringView topic;
    Handler handler;
};

PROGMEM_STRING(MqttTopicRelay, MQTT_TOPIC_RELAY);
PROGMEM_STRING(MqttTopicPulse, MQTT_TOPIC_PULSE);
PROGMEM_STRING(MqttTopicLock, MQTT_TOPIC_LOCK);

static constexpr RelayMqttTopicHandler RelayMqttTopicHandlers[] PROGMEM {
    {MqttTopicRelay, _relayHandlePayload},
    {MqttTopicPulse, _relayHandlePulsePayload},
    {MqttTopicLock, _relayHandleLockPayload},
};

} // namespace

void relayMQTTCallback(unsigned int type, espurna::StringView topic, espurna::StringView payload) {
    static bool connected { false };
    if (!_relays.size()) {
        return;
    }

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_RELAY "/+");
        mqttSubscribe(MQTT_TOPIC_PULSE "/+");
        mqttSubscribe(MQTT_TOPIC_LOCK "/+");
        _relayMqttSubscribeCustomTopics();
        connected = true;
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
                _relays[id].report = mqttForward();

                return;
            }
        }

        _relayMqttHandleCustomTopic(topic, payload);
        return;
    }

    if (type == MQTT_DISCONNECT_EVENT) {
        if (connected) {
            connected = false;
            _relayMqttHandleDisconnect();
        }
        return;
    }

}

void relaySetupMQTT() {
    mqttHeartbeat(_relayMqttHeartbeat);
    mqttRegister(relayMQTTCallback);
}

#endif

//------------------------------------------------------------------------------
// Settings
//------------------------------------------------------------------------------

#if TERMINAL_SUPPORT

namespace {

using TerminalRelayPrintExtra = void(*)(const Relay&, char* out, size_t size);

void _relayPrint(Print& out, const Relay& relay, size_t index) {
    const auto provider = relay.provider->id();

    const char* target_status = relay.target_status
        ? PSTR("ON") : PSTR("OFF");
    const char* current_status = relay.current_status
        ? PSTR("ON") : PSTR("OFF");

    const auto lock = _relayTristateToPayload(relay.lock);

    out.printf_P(PSTR("relay%u {Prov=%.*s TargetStatus=%s CurrentStatus=%s Lock=%.*s}\n"),
        index, provider.length(), provider.begin(),
        current_status, target_status,
        lock.length(), lock.c_str());
}

void _relayPrint(Print& out, size_t start, size_t stop) {
    for (size_t index = start; index < stop; ++index) {
        _relayPrint(out, _relays[index], index);
    }
}

PROGMEM_STRING(RelayCommand, "RELAY");

static void _relayCommand(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 1) {
        _relayPrint(ctx.output, 0, _relays.size());
        terminalOK(ctx);
        return;
    }

    size_t id;
    if (!_relayTryParseId(ctx.argv[1], id)) {
        terminalError(ctx, F("Invalid relayID"));
        return;
    }

    ctx.output.println(id);

    if (ctx.argv.size() > 2) {
        auto status = relayParsePayload(ctx.argv[2]);
        if (PayloadStatus::Unknown == status) {
            terminalError(ctx, F("Invalid status"));
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

PROGMEM_STRING(PulseCommand, "PULSE");

static void _relayCommandPulse(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() < 3) {
        terminalError(ctx, F("PULSE <ID> <TIME> [<TOGGLE>]"));
        return;
    }

    size_t id;
    if (!_relayTryParseId(ctx.argv[1], id)) {
        terminalError(ctx, F("Invalid relayID"));
        return;
    }

    const auto time = espurna::relay::pulse::settings::parse_time(ctx.argv[2]);
    if (!time.ok) {
        terminalError(ctx, F("Invalid pulse time"));
        return;
    }

    const auto duration = espurna::relay::pulse::settings::native_duration(time);

    bool toggle = true;
    if (ctx.argv.size() == 4) {
        auto* convert= espurna::settings::internal::convert<bool>;
        toggle = convert(ctx.argv[3]);
    }

    const auto status = relayStatus(id);
    if (toggle && _relayPulseActive(id, status)) {
        terminalError(ctx, F("Pulse already active!"));
        return;
    }

    const auto target = toggle ? status : !status;
    espurna::relay::pulse::trigger(duration, id, target);

    if ((duration.count() > 0) && toggle) {
        relayToggle(id, true, false);
    }

    terminalOK(ctx);
}

PROGMEM_STRING(LockCommand, "LOCK");

static void _relayCommandLock(::terminal::CommandContext&& ctx) {
    if ((ctx.argv.size() != 2) && (ctx.argv.size() != 3)) {
        terminalError(ctx, F("LOCK <ID> [<TARGET>]"));
        return;
    }

    size_t id;
    if (!_relayTryParseId(ctx.argv[1], id)) {
        terminalError(ctx, F("Invalid relayID"));
        return;
    }

    const auto status = relayStatus(id);
    if (relayStatusTarget(id) != status) {
        terminalError(ctx, F("Relay change in-progress"));
        return;
    }

    auto lock = (status) ? RelayLock::On : RelayLock::Off;
    if (ctx.argv.size() == 3) {
        lock = _relayTristateFromPayload<RelayLock>(ctx.argv[2]);
    }

    _relays[id].lock = lock;

    switch (lock) {
    case RelayLock::None:
        relayStatus(id, status);
        break;
    case RelayLock::Off:
    case RelayLock::On:
        relayStatus(id, (RelayLock::On == lock));
        break;
    }

    terminalOK(ctx);
}

PROGMEM_STRING(UnlockCommand, "UNLOCK");

static void _relayCommandUnlock(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 2) {
        terminalError(ctx, F("UNLOCK <ID>"));
        return;
    }

    size_t id;
    if (!_relayTryParseId(ctx.argv[1], id)) {
        terminalError(ctx, F("Invalid relayID"));
        return;
    }

    const auto status = relayStatus(id);
    if (relayStatusTarget(id) != status) {
        terminalError(ctx, F("Relay change in-progress"));
        return;
    }

    _relays[id].lock = RelayLock::None;

    relayStatus(id, status);

    terminalOK(ctx);
}

static constexpr ::terminal::Command RelayCommands[] PROGMEM {
    {RelayCommand, _relayCommand},
    {PulseCommand, _relayCommandPulse},
    {LockCommand, _relayCommandLock},
    {UnlockCommand, _relayCommandUnlock},
};

void _relayCommandsSetup() {
    espurna::terminal::add(RelayCommands);
}

} // namespace

#endif // TERMINAL_SUPPORT

//------------------------------------------------------------------------------

namespace {

void _relayReport(size_t id [[gnu::unused]], bool status [[gnu::unused]]) {
    for (auto& change : _relay_status_change) {
        change(id, status);
    }
#if MQTT_SUPPORT
    _relayMqttReport(id);
#endif
#if WEB_SUPPORT
    _relayScheduleWsReport();
#endif
}

void _relayReport() {
#if WEB_SUPPORT
    _relayWsReport();
#endif
}

/**
 * Walks the relay vector processing only those relays
 * that have to change to the requested mode
 * @bool mode Requested mode
 */
bool _relayProcess(bool mode) {
    const auto relays = _relays.size();
    bool changed { false };

    for (size_t id = 0; id < relays; ++id) {
        // Only process the relays:
        // - target mode in the one requested by the arg
        // - target status is different from the current one
        // - change delay has expired
        const bool target { _relays[id].target_status };

        if ((target != _relays[id].current_status)
            && (target == mode)
            && ((!_relays[id].change_delay.count())
                || (Relay::TimeSource::now() - _relays[id].change_start > _relays[id].change_delay)))
        {
            // delay will be reset back to the correct value via relayStatus
            _relays[id].change_delay = Relay::Delay::zero();
            _relays[id].current_status = target;
            _relays[id].provider->change(target);

            _relayReport(id, target);

            // try to immediately schedule 'normal' state
            _relayProcessActivePulse(_relays[id], id, target);

            // and make sure relay values are persisted in RAM and flash
            _relayScheduleSave(id);
            changed = true;

            DEBUG_MSG_P(PSTR("[RELAY] #%u set to %s\n"), id, target ? "ON" : "OFF");
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
        _relayProcessPulse();
        _relayPrepareUnlock();
    }

    _relayProcessUnlock();

    _relayReport();
    _relaySave();
}

} // namespace

// Dummy relays for virtual light switches (hardware-less), Sonoff Dual, Sonoff RF Bridge and Tuya

void relaySetupDummy(size_t size, bool reconfigure) {
    if (size == _relayDummy) {
        return;
    }

    const size_t new_size = ((_relays.size() - _relayDummy) + size);
    if (new_size > RelaysMax) {
        return;
    }

    _relayDummy = size;
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

RelayProviderBasePtr _relaySetupProvider(size_t index) {
    auto provider = espurna::relay::settings::provider(index);
    auto type = espurna::relay::settings::type(index);

    RelayProviderBasePtr result;

    switch (provider) {
    case RelayProvider::Dummy:
        result = std::make_unique<DummyProvider>();
        break;

    case RelayProvider::Gpio:
        result = _relayGpioProvider(index, type);
        break;

    case RelayProvider::Stm:
#if RELAY_PROVIDER_STM_SUPPORT
        result = std::make_unique<StmProvider>(index);
#endif
        break;

    case RelayProvider::Dual:
#if RELAY_PROVIDER_DUAL_SUPPORT
        result = std::make_unique<DualProvider>(index);
#endif
        break;

    case RelayProvider::LightState:
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        result = lightMakeStateRelayProvider(index);
#endif
        break;

    case RelayProvider::Fan:
#if FAN_SUPPORT
        result = fanMakeRelayProvider(index);
#endif
        break;

    case RelayProvider::Lightfox:
#ifdef FOXEL_LIGHTFOX_DUAL
        result = lightfoxMakeRelayProvider(index);
#endif
        break;

    case RelayProvider::Tuya:
#if TUYA_SUPPORT
        result = tuya::makeRelayProvider(index);
#endif
        break;

    case RelayProvider::None:
        break;
    }

    return result;
}

void _relaySetup() {
    auto relays = _relays.size();
    _relays.reserve(relays + _relayAdhocPins());

    for (size_t id = relays; id < RelaysMax; ++id) {
        auto impl = _relaySetupProvider(id);
        if (!impl) {
            break;
        }
        if (!impl->setup()) {
            break;
        }
        _relays.emplace_back(std::move(impl));
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
    PROGMEM_STRING(Prefix, "relay");
    return espurna::settings::query::samePrefix(key, Prefix);
}

String findIndexedValueFrom(StringView key) {
    return espurna::settings::query::IndexedSetting::findValueFrom(_relays.size(), IndexedSettings, key);
}

bool checkExact(StringView key) {
    for (const auto& setting : Settings) {
        if (key == setting.key()) {
            return true;
        }
    }

    return false;
}

String findValueFrom(StringView key) {
    return espurna::settings::query::Setting::findValueFrom(Settings, key);
}

void setup() {
    ::settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findIndexedValueFrom
    });

    ::settingsRegisterQueryHandler({
        .check = checkExact,
        .get = findValueFrom
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
        relaySetupWS();
    #endif
    #if API_SUPPORT
        relaySetupAPI();
    #endif
    #if MQTT_SUPPORT
        relaySetupMQTT();
    #endif
    #if TERMINAL_SUPPORT
        _relayCommandsSetup();
    #endif

    // Main callbacks
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
