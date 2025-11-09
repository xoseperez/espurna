/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

To (re)create the string -> pattern decoder .ipp files, add `re2c` to the $PATH and 'run' the environment:
```
$ pio run -e ... -t espurna/led_parse.re.ipp
```
(see scripts/pio_pre.py and scripts/espurna_utils/build.py for more info)

*/

#include "espurna.h"

#if LED_SUPPORT

#include <algorithm>
#include <cstring>
#include <ctime>
#include <chrono>
#include <forward_list>
#include <vector>

#include "led.h"
#include "led_internal.h"

#include "mqtt.h"
#include "relay.h"
#include "rpc.h"

#if WEB_SUPPORT
#include "ws.h"
#endif

namespace espurna {
namespace led {

using TimeSource = espurna::time::CpuClock;
using TimePoint = TimeSource::time_point;

bool operator==(const Delay& lhs, const Delay& rhs) {
    return lhs.on == rhs.on
        && lhs.off == rhs.off
        && lhs.repeats == rhs.repeats;
}

} // namespace led
} // namespace espurna

#include "led_pattern.ipp"
#include "led_parse.re.ipp"

namespace espurna {
namespace led {
namespace {

// Currently used delay value cycles between 'on' and 'off',
// allow to set the current one and to wait until it expires
struct Cycle {
    Cycle() = default;

    bool run(TimePoint tp, Duration delay) {
        bool out = false;
        if (tp - _last > delay) {
            _last = tp;
            out = true;
        }

        return out;
    }

    void reset(TimePoint tp) {
        _last = tp;
    }

    void reset(TimePoint tp, Duration delay) {
        reset(tp - delay);
    }

    TimePoint last() const {
        return _last;
    }

private:
    TimePoint _last{};
};

struct Led {
    Led() = delete;
    Led(unsigned char pin, bool inverse, LedMode mode);

    unsigned char pin() const {
        return _pin;
    }

    LedMode mode() const {
        return _mode;
    }

    void mode(LedMode mode) {
        _mode = mode;
    }

    bool inverse() const {
        return _inverse;
    }

    Pattern& pattern() {
        return _pattern;
    }

    const Pattern& pattern() const {
        return _pattern;
    }

    void override_pattern(Pattern&& pattern) {
        _pattern = std::move(pattern);
        _pattern_override = _pattern.ok();
    }

    void maybe_pattern(Pattern&& pattern) {
        if (_pattern_override) {
            return;
        }

        if (_pattern != pattern) {
            _pattern = std::move(pattern);
        }
    }

    void stop() {
        _sequence.stop();
    }

    bool status() const;
    bool status(bool);

    bool toggle();

    void run() {
        const auto changed = _sequence.run(
            // which status of the pattern delay value to use
            [&]() {
                return _state;
            },
            // wait until the next on <-> off happens
            [&](bool, Duration delay) {
                return wait(delay);
            });

        if (changed) {
            state(!_state);
        }
    }

private:
    void initial_status();

    bool state() const;
    bool state(bool);

    bool wait(Duration);
    void pattern_status(bool);

    unsigned char _pin{ GPIO_NONE };

    bool _state{};
    bool _inverse{};

    Pattern _pattern;
    bool _pattern_override{ false };

    LedMode _mode{};

    Sequence _sequence;
    Cycle _cycle;
};

Led::Led(unsigned char pin, bool inverse, LedMode mode) :
    _pin(pin),
    _inverse(inverse),
    _mode(mode)
{
    initial_status();
}

void Led::initial_status() {
    pinMode(_pin, OUTPUT);
    status(false);
}

bool Led::status() const {
    return _sequence || _state;
}

bool Led::status(bool next) {
    if (_pattern) {
        pattern_status(next);
    }

    return state(next);
}

bool Led::toggle() {
    return status(!status());
}

bool Led::state(bool next) {
    _state = next;
    digitalWrite(_pin, _inverse ? !next : next);
    return next;
}

bool Led::wait(Duration delay) {
    return _cycle.run(TimeSource::now(), delay);
}

void Led::pattern_status(bool next) {
    if (next) {
        _sequence = _pattern.make_sequence();
        if (_sequence) {
            _cycle.reset(TimeSource::now(), _sequence.on());
        }
    } else {
        _sequence.stop();
    }
}

namespace settings {
namespace keys {

PROGMEM_STRING(Gpio, "ledGpio");
PROGMEM_STRING(Inverse, "ledInv");
PROGMEM_STRING(Mode, "ledMode");
PROGMEM_STRING(Relay, "ledRelay");
PROGMEM_STRING(Pattern, "ledPattern");

} // namespace keys

namespace options {

using espurna::settings::options::Enumeration;

PROGMEM_STRING(Manual, "manual");
PROGMEM_STRING(WiFi, "wifi");
PROGMEM_STRING(On, "on");
PROGMEM_STRING(Off, "off");

#if RELAY_SUPPORT
PROGMEM_STRING(Relay, "relay");
PROGMEM_STRING(RelayInverse, "relay-inverse");

PROGMEM_STRING(FindMe, "findme");
PROGMEM_STRING(FindMeWiFi, "findme-wifi");

PROGMEM_STRING(Relays, "relays");
PROGMEM_STRING(RelaysWiFi, "relays-wifi");
#endif

static constexpr Enumeration<LedMode> LedModeOptions[] PROGMEM {
    {LedMode::Manual, Manual},
    {LedMode::WiFi, WiFi},
#if RELAY_SUPPORT
    {LedMode::Relay, Relay},
    {LedMode::RelayInverse, RelayInverse},
    {LedMode::FindMe, FindMe},
    {LedMode::FindMeWiFi, FindMeWiFi},
#endif
    {LedMode::On, On},
    {LedMode::Off, Off},
#if RELAY_SUPPORT
    {LedMode::Relays, Relays},
    {LedMode::RelaysWiFi, RelaysWiFi},
#endif
};

} // namespace options
} // namespace settings
} // namespace
} // namespace led

// -----------------------------------------------------------------------------

namespace settings {
namespace internal {
namespace {

using led::settings::options::LedModeOptions;

} // namespace

template <>
LedMode convert(const String& value) {
    return convert(LedModeOptions, value, LedMode::Manual);
}

String serialize(LedMode mode) {
    return serialize(LedModeOptions, mode);
}

String serialize(const led::Pattern& pattern) {
    return pattern.toString();
}

} // namespace internal
} // namespace settings

// -----------------------------------------------------------------------------

namespace led {
namespace {

namespace build {

constexpr size_t LedsMax { 8ul };

constexpr size_t preconfiguredLeds() {
    return 0ul
    #if LED1_PIN != GPIO_NONE
        + 1ul
    #endif
    #if LED2_PIN != GPIO_NONE
        + 1ul
    #endif
    #if LED3_PIN != GPIO_NONE
        + 1ul
    #endif
    #if LED4_PIN != GPIO_NONE
        + 1ul
    #endif
    #if LED5_PIN != GPIO_NONE
        + 1ul
    #endif
    #if LED6_PIN != GPIO_NONE
        + 1ul
    #endif
    #if LED7_PIN != GPIO_NONE
        + 1ul
    #endif
    #if LED8_PIN != GPIO_NONE
        + 1ul
    #endif
        ;
}

constexpr unsigned char pin(size_t index) {
    return (
        (index == 0) ? LED1_PIN :
        (index == 1) ? LED2_PIN :
        (index == 2) ? LED3_PIN :
        (index == 3) ? LED4_PIN :
        (index == 4) ? LED5_PIN :
        (index == 5) ? LED6_PIN :
        (index == 6) ? LED7_PIN :
        (index == 7) ? LED8_PIN : GPIO_NONE
    );
}

constexpr LedMode mode(size_t index) {
    return (
        (index == 0) ? LED1_MODE :
        (index == 1) ? LED2_MODE :
        (index == 2) ? LED3_MODE :
        (index == 3) ? LED4_MODE :
        (index == 4) ? LED5_MODE :
        (index == 5) ? LED6_MODE :
        (index == 6) ? LED7_MODE :
        (index == 7) ? LED8_MODE : LedMode::Manual
    );
}

constexpr unsigned char relay(size_t index) {
    return (
        (index == 0) ? (LED1_RELAY - 1) :
        (index == 1) ? (LED2_RELAY - 1) :
        (index == 2) ? (LED3_RELAY - 1) :
        (index == 3) ? (LED4_RELAY - 1) :
        (index == 4) ? (LED5_RELAY - 1) :
        (index == 5) ? (LED6_RELAY - 1) :
        (index == 6) ? (LED7_RELAY - 1) :
        (index == 7) ? (LED8_RELAY - 1) : RELAY_NONE
    );
}

constexpr bool inverse(size_t index) {
    return (
        (index == 0) ? (1 == LED1_PIN_INVERSE) :
        (index == 1) ? (1 == LED2_PIN_INVERSE) :
        (index == 2) ? (1 == LED3_PIN_INVERSE) :
        (index == 3) ? (1 == LED4_PIN_INVERSE) :
        (index == 4) ? (1 == LED5_PIN_INVERSE) :
        (index == 5) ? (1 == LED6_PIN_INVERSE) :
        (index == 6) ? (1 == LED7_PIN_INVERSE) :
        (index == 7) ? (1 == LED8_PIN_INVERSE) : false
    );
}

} // namespace build

namespace settings {

unsigned char pin(size_t id) {
    return getSetting({keys::Gpio, id}, build::pin(id));
}

bool inverse(size_t id) {
    return getSetting({keys::Inverse, id}, build::inverse(id));
}

LedMode mode(size_t id) {
    return getSetting({keys::Mode, id}, build::mode(id));
}

#if RELAY_SUPPORT

size_t relay(size_t id) {
    return getSetting({keys::Relay, id}, build::relay(id));
}

#endif

Pattern pattern(size_t id) {
    return parse(getSetting({keys::Pattern, id}));
}

void migrate(int version) {
    if (version < 5) {
        delSettingPrefix({
            keys::Gpio,
            STRING_VIEW("ledGPIO"),
            STRING_VIEW("ledLogic")
        });
    }
}

} // namespace settings

// For network-based modes, indefinitely cycle ON <-> OFF
#define LED_STATIC_DELAY(NAME, ON, OFF)\
    static_assert(valid_duration(duration::Milliseconds(ON)), "ON should fit ccount");\
    static_assert(valid_duration(duration::Milliseconds(OFF)), "OFF should fit ccount");\
    static constexpr auto NAME PROGMEM = Delay{\
        .on = duration::Milliseconds(ON),\
        .off = duration::Milliseconds(OFF),\
        .repeats = 0 }

static constexpr auto CpuCyclesMax
    = std::chrono::duration_cast<espurna::duration::Milliseconds>(Duration::max());

constexpr bool valid_duration(duration::Milliseconds duration) {
    return duration < CpuCyclesMax;
}

LED_STATIC_DELAY(NetworkConnected, 100, 4900);
LED_STATIC_DELAY(NetworkConnectedInverse, 4900, 100);
LED_STATIC_DELAY(NetworkConfig, 100, 900);
LED_STATIC_DELAY(NetworkConfigInverse, 900, 100);
LED_STATIC_DELAY(NetworkIdle, 500, 500);

Delay network_delay() {
    Delay out;

    if (wifiConnected()) {
        out = NetworkConnected;
    } else if (wifiConnectable()) {
        out = NetworkConfig;
    } else {
        out = NetworkIdle;
    }

    return out;
}

Pattern network_pattern() {
    return Pattern(network_delay());
}

// For a special case when system is unstable
#if SYSTEM_CHECK_ENABLED
LED_STATIC_DELAY(SystemUnstable, 2000, 1000);
#endif

constexpr uint8_t ScheduleManual { 1 << 0 };
constexpr uint8_t ScheduleNetwork { 1 << 1 };
constexpr uint8_t ScheduleRelay { 1 << 2 };

constexpr uint8_t ScheduleAll { std::numeric_limits<uint8_t>::max() };

namespace internal {

std::vector<Led> leds;
uint8_t update;

} // namespace internal

bool add(size_t index) {
    const auto pin = settings::pin(index);
    if (gpioLock(pin)) {
        internal::leds.emplace_back(pin,
                settings::inverse(index), settings::mode(index));
        return true;
    }

    return false;
}

namespace settings {

STRING_VIEW_INLINE(Prefix, "led");

namespace query {
namespace internal {

#define ID_VALUE(NAME)\
String NAME (size_t id) {\
    return espurna::settings::internal::serialize(\
        ::espurna::led::settings::NAME(id));\
}

ID_VALUE(pin)
ID_VALUE(inverse)
ID_VALUE(mode)
ID_VALUE(pattern)

#if RELAY_SUPPORT
ID_VALUE(relay)
#endif

#undef ID_VALUE

} // namespace internal

static constexpr espurna::settings::query::IndexedSetting IndexedSettings[] PROGMEM {
    {keys::Gpio, internal::pin},
    {keys::Inverse, internal::inverse},
    {keys::Mode, internal::mode},
    {keys::Pattern, internal::pattern},
#if RELAY_SUPPORT
    {keys::Relay, internal::relay},
#endif
};

bool checkSamePrefix(StringView key) {
    return key.startsWith(Prefix);
}

espurna::settings::query::Result findFrom(StringView key) {
    return espurna::settings::query::findFrom(
        ::espurna::led::build::LedsMax, IndexedSettings, key);
}

void setup() {
    ::settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findFrom,
    });
}

} // namespace query
} // namespace settings

enum class Status {
    Unknown,
    On,
    Off,
};

#if RELAY_SUPPORT
namespace relay {

struct Link {
    Led& led;
    size_t relayId;
};

namespace internal {

std::forward_list<Link> relays;

bool linked(const Link& link, const Led& led) {
    return &link.led == &led;
}

void unlink(Led& led) {
    relays.remove_if(
        [&](const Link& link) {
            return linked(link, led);
        });
}

Link* find(const Led& led) {
    auto it = std::find_if(
        relays.begin(),
        relays.end(),
        [&](const Link& link) {
            return linked(link, led);
        });

    if (it != relays.end()) {
        return std::addressof(*it);
    }

    return nullptr;
}

void link(Led& led, size_t id) {
    auto link = find(led);

    if (link) {
        link->relayId = id;
        return;
    }

    relays.emplace_front(Link{led, id});
}

} // namespace internal

void unlink(Led& led) {
    internal::unlink(led);
}

void link(Led& led, size_t id) {
    internal::link(led, id);
}

Status mode_status(const Led& led) {
    auto out = Status::Unknown;

    const auto mode = led.mode();
    auto status = false;

    switch (mode) {
    case LedMode::Relay:
    case LedMode::RelayInverse: {
        const auto* link = internal::find(led);
        if (!link || (link->relayId >= RelaysMax)) {
            return out;
        }

        status = relayStatus(link->relayId);
        if (mode == LedMode::RelayInverse) {
            status = !status;
        }
        break;
    }

    case LedMode::FindMe:
    case LedMode::FindMeWiFi:
        status = relayStatus();
        break;

    case LedMode::Relays:
    case LedMode::RelaysWiFi:
        status = !relayStatus();
        break;

    default:
        break;
    }

    if (status) {
        out = Status::On;
    } else {
        out = Status::Off;
    }

    return out;
}

Delay network_delay(Status status) {
    Delay out;

    if (wifiConnected()) {
        if (status == Status::On) {
            out = NetworkConnected;
        } else {
            out = NetworkConnectedInverse;
        }
    } else if (wifiConnectable()) {
        if (status == Status::On) {
            out = NetworkConfig;
        } else {
            out = NetworkConfigInverse;
        }
    } else {
        out = NetworkIdle;
    }

    return out;
}

Pattern findme_pattern(Status status) {
    return Pattern(network_delay(status));
}

void configure(Led& led, LedMode mode, size_t id) {
    switch (mode) {
    case LedMode::Relay:
    case LedMode::RelayInverse:
        link(led, settings::relay(id));
        break;

    default:
        unlink(led);
        break;
    }
}

} // namespace relay
#endif

size_t count() {
    return internal::leds.size();
}

uint8_t current_update() {
    const auto update = internal::update;
    if (update) {
        internal::update = 0;
    }

    return update;
}

void schedule(uint8_t mask) {
    internal::update |= mask;
}

void schedule_all() {
    schedule(ScheduleAll);
}

bool status(Led& led) {
    return led.status();
}

bool status(size_t id) {
    return status(internal::leds[id]);
}

bool status(size_t id, bool value) {
    return internal::leds[id].status(value);
}

void turn_off() {
    for (auto& led : internal::leds) {
        led.status(false);
    }
}

bool payload_mode(Led& led, StringView payload) {
    using espurna::settings::internal::LedModeOptions;

    for (auto& opt : LedModeOptions) {
        if (payload == opt.string()) {
            led.mode(opt.value());
            return true;
        }
    }

    return false;
}

void payload_status(Led& led, StringView payload) {
    led.stop();
    led.mode(LedMode::Manual);
    led.override_pattern(Pattern{});

#if RELAY_SUPPORT
    relay::unlink(led);
#endif

    const auto value = rpcParsePayload(payload);
    switch (value) {
    case PayloadStatus::On:
        led.mode(LedMode::On);
        break;

    case PayloadStatus::Off:
        led.mode(LedMode::Off);
        break;

    case PayloadStatus::Toggle:
        led.status(!led.status());
        break;

    case PayloadStatus::Unknown:
        if (!payload_mode(led, payload)) {
            led.override_pattern(parse(payload));
            led.status(true);
        }
        break;
    }
}

void configure() {
    for (size_t id = 0; id < internal::leds.size(); ++id) {
        auto& led = internal::leds[id];

        led.override_pattern(settings::pattern(id));
        led.mode(settings::mode(id));

#if RELAY_SUPPORT
        relay::configure(led, led.mode(), id);
#endif
    }

    schedule_all();
}

void loop(Led& led, uint8_t update) {
    auto next = Status::Unknown;

    switch (led.mode()) {
    case LedMode::Manual:
        break;

    case LedMode::WiFi:
        if (update & ScheduleNetwork) {
            led.maybe_pattern(network_pattern());
            next = Status::On;
        }
        break;

    case LedMode::FindMeWiFi:
    case LedMode::RelaysWiFi:
#if RELAY_SUPPORT
        if (update & (ScheduleNetwork | ScheduleRelay)) {
            const auto mode_status = relay::mode_status(led);
            led.maybe_pattern(relay::findme_pattern(mode_status));
            next = Status::On;
        }
#endif
        break;

    case LedMode::Relay:
    case LedMode::RelayInverse:
    case LedMode::FindMe:
    case LedMode::Relays:
#if RELAY_SUPPORT
        if (update & ScheduleRelay) {
            next = relay::mode_status(led);
        }
#endif
        break;

    case LedMode::On:
        if (update & ScheduleManual) {
            next = Status::On;
        }
        break;

    case LedMode::Off:
        if (update & ScheduleManual) {
            next = Status::Off;
        }
        break;

    }

    switch (next) {
    case Status::Unknown:
        break;

    case Status::On:
        led.status(true);
        break;

    case Status::Off:
        led.status(false);
        break;
    }

    led.run();
}

void loop() {
    const auto update = current_update();
    for (auto& led : internal::leds) {
        loop(led, update);
    }
}

#if MQTT_SUPPORT
namespace mqtt {

void callback(unsigned int type, StringView topic, StringView payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_LED "/+");
        return;
    }

    // Only want `led/+/<MQTT_SETTER>`
    // We get the led ID from the `+`
    if (type == MQTT_MESSAGE_EVENT) {
        const auto magnitude = mqttMagnitude(topic);
        if (!magnitude.startsWith(MQTT_TOPIC_LED)) {
            return;
        }

        size_t ledID;
        if (tryParseIdPath(magnitude, ledCount(), ledID)) {
            payload_status(internal::leds[ledID], payload);
        }

        return;
    }
}

} // namespace mqtt
#endif // MQTT_SUPPORT

#if WEB_SUPPORT
namespace web {

bool onKeyCheck(StringView key, const JsonVariant&) {
    return settings::query::checkSamePrefix(key);
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, settings::Prefix);

    espurna::web::ws::EnumerableTypes types{root, STRING_VIEW("ledModes")};
    types(espurna::led::settings::options::LedModeOptions);
}

void onConnected(JsonObject& root) {
    if (count()) {
        espurna::web::ws::EnumerableConfig config{root, STRING_VIEW("ledConfig")};
        config.replacement(
            settings::query::internal::mode,
            [](JsonArray& out, size_t index) {
                out.add(std::to_underlying(settings::mode(index)));
            });
        config(STRING_VIEW("leds"), ::espurna::led::count(), settings::query::IndexedSettings);
    }
}

} // namespace web
#endif // WEB_SUPPORT

#if TERMINAL_SUPPORT
namespace terminal {

PROGMEM_STRING(Led, "LED");

void led(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        size_t id;
        if (!tryParseId(ctx.argv[1], ledCount(), id)) {
            terminalError(ctx, F("Invalid ledID"));
            return;
        }

        auto& led = internal::leds[id];
        if (ctx.argv.size() == 2) {
            settingsDump(ctx, settings::query::IndexedSettings, id);
        } else if (ctx.argv.size() > 2) {
            payload_status(led, ctx.argv[2]);
        }

        schedule_all();
        terminalOK(ctx);

        return;
    }

    size_t id { 0 };
    for (const auto& led : internal::leds) {
        ctx.output.printf_P(
                PSTR("led%u {Gpio=%hhu Mode=%s Pattern={%s}}\n"), id++, led.pin(),
                espurna::settings::internal::serialize(led.mode()).c_str(),
                led.pattern().toString().c_str());
    }
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Led, led},
};

void setup() {
    espurna::terminal::add(Commands);
}

} // namespace terminal
#endif

void setup() {
    migrateVersion(settings::migrate);
    internal::leds.reserve(build::preconfiguredLeds());

    for (size_t index = 0; index < build::LedsMax; ++index) {
        if (!add(index)) {
            break;
        }
    }

    const auto leds = internal::leds.size();
    DEBUG_MSG_P(PSTR("[LED] Number of leds: %u\n"), leds);
    if (leds) {
        espurna::led::settings::query::setup();
#if MQTT_SUPPORT
        ::mqttRegister(mqtt::callback);
#endif
#if WEB_SUPPORT
        ::wsRegister()
            .onVisible(web::onVisible)
            .onConnected(web::onConnected)
            .onKeyCheck(web::onKeyCheck);
#endif
#if RELAY_SUPPORT
        ::relayOnStatusChange([](size_t, bool) {
            schedule(ScheduleRelay);
        });
#endif
#if TERMINAL_SUPPORT
        terminal::setup();
#endif
        wifiRegister([](espurna::wifi::Event) {
            schedule(ScheduleNetwork);
        });

        systemBeforeSleep(turn_off);
        systemAfterSleep(schedule_all);

        ::espurnaRegisterLoop(loop);

        ::espurnaRegisterReload(configure);
        configure();
    }
}

void setup_unstable() {
#if SYSTEM_CHECK_ENABLED
    setup();

    for (auto& led : internal::leds) {
        switch (led.mode()) {
        case LedMode::FindMe:
        case LedMode::FindMeWiFi:
        case LedMode::RelaysWiFi:
        case LedMode::WiFi:
            led.mode(LedMode::Manual);
            led.override_pattern(Pattern(SystemUnstable));
            led.status(true);
            break;

        default:
            break;
        }
    }
#endif
}

} // namespace
} // namespace led
} // namespace espurna

bool ledStatus(size_t id, bool status) {
    if (id < espurna::led::count()) {
        return espurna::led::status(id, status);
    }

    return status;
}

bool ledStatus(size_t id) {
    if (id < espurna::led::count()) {
        return espurna::led::status(id);
    }

    return false;
}

size_t ledCount() {
    return espurna::led::count();
}

void ledLoop() {
    espurna::led::loop();
}

void ledSetup() {
    espurna::led::setup();
}

void ledSetupUnstable() {
    espurna::led::setup_unstable();
}

#endif // LED_SUPPORT
