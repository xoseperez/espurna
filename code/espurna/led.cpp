/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

To (re)create the string -> pattern decoder .ipp files, add `re2c` to the $PATH and 'run' the environment:
```
$ pio run -e ... -t espurna/led_pattern.re.ipp
```
(see scripts/pio_pre.py and scripts/espurna_utils/build.py for more info)

*/

#include "espurna.h"

#if LED_SUPPORT

#include "led.h"
#include "mqtt.h"
#include "relay.h"
#include "rpc.h"
#include "ws.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <chrono>
#include <forward_list>
#include <vector>

namespace espurna {
namespace led {
namespace {

// Some local-only time & counters implementation:
// - Core conversion is done through macros, implement stronger types
// - force unsigned instead of chrono's 'int64_t', since we want safe overflow
// - bound to 32bits, to seamlessly handle ccount conversion from the 'time source'
// - explicitly check for the maximum number of milliseconds that may be represented with ccount

// TODO: full-width int for repeats instead of 8bit? right now, string parser will *force* [min:max] range,
// but anything else is experiencing overflow mechanics

struct alignas(8) Delay {
    using Source = espurna::time::CpuClock;
    using Duration = Source::duration;
    using TimePoint = Source::time_point;

    static constexpr auto ClockCyclesMax = Duration(Duration::max());
    static constexpr auto MillisecondsMax = std::chrono::duration_cast<espurna::duration::Milliseconds>(ClockCyclesMax);

    using Repeats = size_t;
    static constexpr Repeats RepeatsMin { std::numeric_limits<Repeats>::min() };
    static constexpr Repeats RepeatsMax { std::numeric_limits<Repeats>::max() };

    enum class Mode {
        Finite,
        Infinite,
        None
    };

    Delay() = delete;

    constexpr Delay(Duration on, Duration off, Repeats repeats) :
        _mode(repeats ? Mode::Finite : Mode::Infinite),
        _on(on),
        _off(off),
        _repeats(repeats)
    {}

    constexpr Mode mode() const {
        return _mode;
    }

    constexpr Duration on() const {
        return _on;
    }

    constexpr Duration off() const {
        return _off;
    }

    constexpr Repeats repeats() const {
        return _repeats;
    }

private:
    Mode _mode;
    Duration _on;
    Duration _off;
    Repeats _repeats;
};

constexpr espurna::duration::ClockCycles Delay::ClockCyclesMax;
constexpr espurna::duration::Milliseconds Delay::MillisecondsMax;

struct Pattern {
    using Delays = std::vector<Delay>;

    Pattern() = default;
    Pattern(Pattern&&) = default;
    Pattern& operator=(Pattern&&) = default;

    Pattern(const char* begin, const char* end);

    explicit Pattern(const String& input) :
        Pattern(input.begin(), input.end())
    {}

    explicit Pattern(Delays&& delays) :
        _delays(std::move(delays)),
        _sequence(_delays),
        _cycle(_delays)
    {}

    explicit operator bool() const {
        return _delays.size() > 0;
    }

    void start() {
        if (!_sequence) {
            _cycle = Delay::Duration::min();
            _sequence = _delays;
        }
    }

    void stop() {
        _cycle = Delay::Duration::min();
        std::move(_sequence) = _delays;
    }

    bool started() const {
        return static_cast<bool>(_sequence);
    }

    const Delays& delays() const {
        return _delays;
    }

    template <typename Status, typename Last>
    void run(Status&& status, Last&& last) {
        if (!_sequence) {
            return;
        }

        if (!_cycle) {
            return;
        }

        const auto currentStatus = status();
        _cycle = currentStatus
            ? _sequence.on() : _sequence.off();

        switch (_sequence.mode()) {
        case Delay::Mode::Finite:
            if (currentStatus && !_sequence.repeat()) {
                if (!_sequence.next()) {
                    last();
                }
            }

            break;
        case Delay::Mode::Infinite:
        case Delay::Mode::None:
            break;
        }
    }

private:
    // Sequence of pending 'delays', by default it's in the order they are specified in the underlying vector.
    // Notice that there are no checks that '_current' is dereferencable, it's up to the consumer to check via 'operator bool()' first
    // TODO: is it actually valid to have 'Sequence() = default', and not actually reference any particular object?
    struct Sequence {
        Sequence() = delete;

        Sequence(const Sequence&) = default;
        Sequence(Sequence&&) = default;

        explicit Sequence(const Delays& delays) :
            _current(delays.cbegin()),
            _end(delays.cend()),
            _repeats((_current != _end) ? (*_current).repeats() : 0)
        {}

        Sequence& operator=(const Sequence&) = default;
        Sequence& operator=(Sequence&&) = default;

        Sequence& operator=(const Delays& delays) & {
            return (*this = Sequence(delays));
        }

        Sequence& operator=(const Delays& delays) && {
            _current = delays.cend();
            _end = delays.cend();
            _repeats = 0;
            return *this;
        }

        explicit operator bool() const {
            return _current != _end;
        }

        Delay::Repeats repeats() const {
            return _repeats;
        }

        Delay::Mode mode() const {
            return (*_current).mode();
        }

        Delay::Duration on() const {
            return (*_current).on();
        }

        Delay::Duration off() const {
            return (*_current).off();
        }

        bool repeat() {
            if (_repeats) {
                --_repeats;
                return true;
            }

            return false;
        }

        bool next() {
            if (_current != _end) {
                ++_current;
                if (_current != _end) {
                    _repeats = (*_current).repeats();
                    return true;
                }
            }

            return false;
        }

    private:
        Delays::const_iterator _current;
        Delays::const_iterator _end;
        Delay::Repeats _repeats;
    };

    // Currently used delay value cycles between 'on' and 'off',
    // allow to set the current one and to wait until it expires
    struct Cycle {
        explicit Cycle(const Delays& delays) :
            _last(Delay::Source::now()),
            _delay(delays.size() ? delays.front().on() : Delay::Duration::min())
        {}

        Cycle& operator=(const Delays& delays) {
            return (*this = Cycle(delays));
        }

        Cycle& operator=(Delay::Duration duration) {
            _last = Delay::Source::now();
            _delay = duration;
            return *this;
        }

        explicit operator bool() const {
            return (Delay::Source::now() - _last > _delay);
        }

    private:
        Delay::TimePoint _last;
        Delay::Duration _delay;
    };

    Delays _delays;

    Sequence _sequence { _delays };
    Cycle _cycle { _delays };
};

struct Led {
    Led() = delete;
    Led(unsigned char pin, bool inverse, LedMode mode) :
        _pin(pin),
        _inverse(inverse),
        _mode(mode)
    {
        init();
    }

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

    void pattern(Pattern&& pattern) {
        _pattern = std::move(pattern);
    }

    bool started() {
        return _pattern.started();
    }

    void stop() {
        _pattern.stop();
    }

    void init();

    bool status();
    bool status(bool new_status);

    bool toggle();

    void run() {
        _pattern.run(
            // notify the pattern about the 'current' status
            [&]() {
                return toggle();
            },
            // what happens when the pattern ends
            [&]() {
                status(false);
            });
    }

private:
    unsigned char _pin;
    bool _inverse;
    LedMode _mode;
    Pattern _pattern;
};

void Led::init() {
    pinMode(_pin, OUTPUT);
    status(false);
}

bool Led::status() {
    bool result = digitalRead(_pin);
    return _inverse ? !result : result;
}

bool Led::status(bool new_status) {
    digitalWrite(_pin, _inverse ? !new_status : new_status);
    return new_status;
}

bool Led::toggle() {
    return status(!status());
}

#include "led_pattern.re.ipp"

} // namespace

namespace settings {
namespace keys {
namespace {

alignas(4) static constexpr char Gpio[] PROGMEM = "ledGpio";
alignas(4) static constexpr char Inverse[] PROGMEM = "ledInv";
alignas(4) static constexpr char Mode[] PROGMEM = "ledMode";
alignas(4) static constexpr char Relay[] PROGMEM = "ledRelay";
alignas(4) static constexpr char Pattern[] PROGMEM = "ledPattern";

} // namespace
} // namespace keys

namespace options {
namespace {

using ::settings::options::Enumeration;

alignas(4) static constexpr char Manual[] PROGMEM = "manual";
alignas(4) static constexpr char WiFi[] PROGMEM = "wifi";
alignas(4) static constexpr char On[] PROGMEM = "on";
alignas(4) static constexpr char Off[] PROGMEM = "off";

#if RELAY_SUPPORT
alignas(4) static constexpr char Relay[] PROGMEM = "relay";
alignas(4) static constexpr char RelayInverse[] PROGMEM = "relay-inverse";

alignas(4) static constexpr char FindMe[] PROGMEM = "findme";
alignas(4) static constexpr char FindMeWiFi[] PROGMEM = "findme-wifi";

alignas(4) static constexpr char Relays[] PROGMEM = "relays";
alignas(4) static constexpr char RelaysWiFi[] PROGMEM = "relays-wifi";
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

} // namespace
} // namespace options
} // namespace settings
} // namespace led
} // namespace espurna

// -----------------------------------------------------------------------------

namespace settings {
namespace internal {
namespace {

using espurna::led::settings::options::LedModeOptions;

} // namespace

template <>
LedMode convert(const String& value) {
    return convert(LedModeOptions, value, LedMode::Manual);
}

String serialize(LedMode mode) {
    return serialize(LedModeOptions, mode);
}

} // namespace internal
} // namespace settings

// -----------------------------------------------------------------------------

namespace espurna {
namespace led {
namespace build {
namespace {

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

} // namespace
} // namespace build

namespace settings {
namespace {

unsigned char pin(size_t id) {
    return getSetting({keys::Gpio, id}, build::pin(id));
}

bool inverse(size_t id) {
    return getSetting({keys::Inverse, id}, build::inverse(id));
}

LedMode mode(size_t id) {
    return getSetting({keys::Mode, id}, build::mode(id));
}

size_t relay(size_t id) {
    return getSetting({keys::Relay, id}, build::relay(id));
}

Pattern pattern(size_t id) {
    return Pattern(getSetting({keys::Pattern, id}));
}

void migrate(int version) {
    if (version < 5) {
        delSettingPrefix({
            keys::Gpio,
            PSTR("ledGPIO"),
            PSTR("ledLogic")
        });
    }
}

} // namespace
} // namespace settings

// For network-based modes, indefinitely cycle ON <-> OFF
// (TODO: template params containing structs like duration need -std=c++2a)

#define LED_STATIC_DELAY(NAME, ON, OFF)\
    static constexpr auto NAME ## MillisecondsOn PROGMEM = espurna::duration::Milliseconds(ON);\
    static constexpr auto NAME ## MillisecondsOff PROGMEM = espurna::duration::Milliseconds(OFF);\
    static_assert(NAME ## MillisecondsOn < Delay::MillisecondsMax, "");\
    static_assert(NAME ## MillisecondsOff < Delay::MillisecondsMax, "");\
    static constexpr Delay NAME PROGMEM = Delay {\
        std::chrono::duration_cast<Delay::Duration>(NAME ## MillisecondsOn),\
        std::chrono::duration_cast<Delay::Duration>(NAME ## MillisecondsOff),\
        Delay::RepeatsMin }

LED_STATIC_DELAY(NetworkConnected, 100, 4900);
LED_STATIC_DELAY(NetworkConnectedInverse, 4900, 100);
LED_STATIC_DELAY(NetworkConfig, 100, 900);
LED_STATIC_DELAY(NetworkConfigInverse, 900, 100);
LED_STATIC_DELAY(NetworkIdle, 500, 500);

namespace internal {
namespace {

std::vector<Led> leds;
bool update { false };

} // namespace
} // namespace internal

namespace settings {
namespace query {
namespace internal {
namespace {

#define ID_VALUE(NAME)\
String NAME (size_t id) {\
    return ::settings::internal::serialize(::espurna::led::settings::NAME(id));\
}

ID_VALUE(pin)
ID_VALUE(inverse)
ID_VALUE(mode)
ID_VALUE(relay)

#undef ID_VALUE

} // namespace
} // namespace internal

namespace {

static constexpr ::settings::query::IndexedSetting IndexedSettings[] PROGMEM {
    {keys::Gpio, internal::pin},
    {keys::Inverse, internal::inverse},
    {keys::Mode, internal::mode},
#if RELAY_SUPPORT
    {keys::Relay, internal::relay},
#endif
};

bool checkSamePrefix(::settings::StringView key) {
    static constexpr char Prefix[] PROGMEM = "led";
    return ::settings::query::samePrefix(key, Prefix);
}

String findValueFrom(::settings::StringView key) {
    return ::settings::query::IndexedSetting::findValueFrom(
        ::espurna::led::internal::leds.size(), IndexedSettings, key);
}

void setup() {
    ::settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findValueFrom
    });
}

} // namespace
} // namespace query
} // namespace settings

#if RELAY_SUPPORT
namespace relay {
namespace internal {
namespace {

struct Link {
    Led& led;
    size_t relayId;
};

std::forward_list<Link> relays;

bool linked(const Link& link, const Led& led) {
    return &link.led == &led;
}

void unlink(Led& led) {
    relays.remove_if([&](const Link& link) {
        return linked(link, led);
    });
}

void link(Led& led, size_t id) {
    auto it = std::find_if(relays.begin(), relays.end(), [&](const Link& link) {
        return linked(link, led);
    });

    if (it != relays.end()) {
        (*it).relayId = id;
        return;
    }

    relays.emplace_front(Link{led, id});
}

size_t find(Led& led) {
    auto it = std::find_if(relays.begin(), relays.end(), [&](const Link& link) {
        return linked(link, led);
    });

    if (it != relays.end()) {
        return (*it).relayId;
    }

    return RelaysMax;
}

} // namespace
} // namespace internal

namespace {

void unlink(Led& led) {
    internal::unlink(led);
}

void link(Led& led, size_t id) {
    internal::link(led, id);
}

size_t find(Led& led) {
    return internal::find(led);
}

bool status(Led& led) {
    return relayStatus(find(led));
}

bool areAnyOn() {
    bool result { false };
    for (size_t id = 0; id < relayCount(); ++id) {
        if (relayStatus(id)) {
            result = true;
            break;
        }
    }

    return result;
}

} // namespace
} // namespace relay
#endif

namespace {

size_t count() {
    return internal::leds.size();
}

bool scheduled() {
    return internal::update;
}

void schedule() {
    internal::update = true;
}

void cancel() {
    internal::update = false;
}

bool status(Led& led) {
    return led.started() || led.status();
}

bool status(size_t id) {
    return status(internal::leds[id]);
}

bool status(Led& led, bool status) {
    bool result = false;

    // when led has pattern, status depends on whether it's running
    // (notice that sending 'true' status multiple times does not restart the pattern)
    auto& pattern = led.pattern();
    if (pattern) {
        if (status) {
            pattern.start();
            result = true;
        } else {
            pattern.stop();
            led.status(false);
            result = false;
        }
    // if not, simply proxy status directly to the led pin
    } else {
        result = led.status(status);
    }

    return result;
}

bool status(size_t id, bool value) {
    return status(internal::leds[id], value);
}

[[gnu::unused]]
void pattern(Led& led, Pattern&& other) {
    led.pattern(std::move(other));
    status(led, true);
}

void run(Led& led, const Delay& delay) {
    using TimeSource = espurna::time::CpuClock;

    static auto clock_last = TimeSource::now();
    static auto delay_for = delay.on();

    const auto clock_current = TimeSource::now();
    if (clock_current - clock_last >= delay_for) {
        delay_for = led.toggle() ? delay.on() : delay.off();
        clock_last = clock_current;
    }
}

void configure() {
    for (size_t id = 0; id < internal::leds.size(); ++id) {
        auto& led = internal::leds[id];
        led.mode(settings::mode(id));
        led.pattern(settings::pattern(id));
#if RELAY_SUPPORT
        switch (internal::leds[id].mode()) {
        case LedMode::Relay:
        case LedMode::RelayInverse:
            relay::link(led, settings::relay(id));
            break;
        default:
            relay::unlink(led);
            break;
        }
#endif
    }
    schedule();
}

void loop(Led& led) {
    switch (led.mode()) {

    case LedMode::Manual:
        break;

    case LedMode::WiFi:
        if (wifiConnected()) {
            run(led, NetworkConnected);
        } else if (wifiConnectable()) {
            run(led, NetworkConfig);
        } else {
            run(led, NetworkIdle);
        }
        break;

    case LedMode::FindMeWiFi:
#if RELAY_SUPPORT
        if (wifiConnected()) {
            if (relay::areAnyOn()) {
                run(led, NetworkConnected);
            } else {
                run(led, NetworkConnectedInverse);
            }
        } else if (wifiConnectable()) {
            if (relay::areAnyOn()) {
                run(led, NetworkConfig);
            } else {
                run(led, NetworkConfigInverse);
            }
        } else {
            run(led, NetworkIdle);
        }
#endif
        break;

    case LedMode::RelaysWiFi:
#if RELAY_SUPPORT
        if (wifiConnected()) {
            if (!relay::areAnyOn()) {
                run(led, NetworkConnected);
            } else {
                run(led, NetworkConnectedInverse);
            }
        } else if (wifiConnectable()) {
            if (!relay::areAnyOn()) {
                run(led, NetworkConfig);
            } else {
                run(led, NetworkConfigInverse);
            }
        } else {
            run(led, NetworkIdle);
        }
#endif
        break;

    case LedMode::Relay:
#if RELAY_SUPPORT
        if (scheduled()) {
            status(led, relay::status(led));
        }
#endif
        break;

    case LedMode::RelayInverse:
#if RELAY_SUPPORT
        if (scheduled()) {
            status(led, !relay::status(led));
        }
#endif
        break;

    case LedMode::FindMe:
#if RELAY_SUPPORT
        if (scheduled()) {
            led::status(led, !relay::areAnyOn());
        }
#endif
        break;

    case LedMode::Relays:
#if RELAY_SUPPORT
        if (scheduled()) {
            led::status(led, relay::areAnyOn());
        }
#endif
        break;

    case LedMode::On:
        if (scheduled()) {
            status(led, true);
        }
        break;

    case LedMode::Off:
        if (scheduled()) {
            status(led, false);
        }
        break;

    }

    led.run();
}

void loop() {
    for (auto& led : internal::leds) {
        loop(led);
    }
    cancel();
}

} // namespace

#if MQTT_SUPPORT
namespace mqtt {
namespace {

void callback(unsigned int type, const char* topic, char* payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_LED "/+");
        return;
    }

    // Only want `led/+/<MQTT_SETTER>`
    // We get the led ID from the `+`
    if (type == MQTT_MESSAGE_EVENT) {
        const String magnitude = mqttMagnitude(topic);
        if (!magnitude.startsWith(MQTT_TOPIC_LED)) {
            return;
        }

        size_t ledID;
        if (!tryParseId(magnitude.substring(strlen(MQTT_TOPIC_LED) + 1).c_str(), ledCount, ledID)) {
            return;
        }

        auto& led = internal::leds[ledID];
        if (led.mode() != LedMode::Manual) {
            return;
        }

        const auto value = rpcParsePayload(payload);
        switch (value) {
        case PayloadStatus::On:
        case PayloadStatus::Off:
            led::status(led, (value == PayloadStatus::On));
            return;
        case PayloadStatus::Toggle:
            led::status(led, !led::status(led));
            return;
        case PayloadStatus::Unknown:
            pattern(led, Pattern(payload, payload + strlen(payload)));
            break;
        }
    }
}

} // namespace
} // namespace mqtt
#endif // MQTT_SUPPORT

#if WEB_SUPPORT
namespace web {
namespace {

bool onKeyCheck(const char* key, JsonVariant&) {
    return settings::query::checkSamePrefix(key);
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, "led");
}

void onConnected(JsonObject& root) {
    if (count()) {
        ::web::ws::EnumerableConfig config{root, STRING_VIEW("ledConfig")};
        config(STRING_VIEW("leds"), ::espurna::led::count(), settings::query::IndexedSettings);
    }
}

} // namespace
} // namespace web
#endif // WEB_SUPPORT

#if TERMINAL_SUPPORT
namespace terminal {
namespace {

void setup() {
    terminalRegisterCommand(F("LED"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() > 1) {
            size_t id;
            if (!tryParseId(ctx.argv[1].c_str(), ledCount, id)) {
                terminalError(ctx, F("Invalid ledID"));
                return;
            }

            auto& led = internal::leds[id];
            if (ctx.argv.size() == 2) {
                settingsDump(ctx, settings::query::IndexedSettings, id);
            } else if (ctx.argv.size() > 2) {
                led.mode(LedMode::Manual);
                pattern(led, Pattern(ctx.argv[2]));
            }

            schedule();
            terminalOK(ctx);

            return;
        }

        size_t id { 0 };
        for (const auto& led : internal::leds) {
            ctx.output.printf_P(
                PSTR("led%u {Gpio=%hhu Mode=%s}\n"), id++, led.pin(),
                ::settings::internal::serialize(led.mode()).c_str());
        }
    });
}

} // namespace
} // namespace terminal
#endif

namespace {

void setup() {
    migrateVersion(settings::migrate);
    internal::leds.reserve(build::preconfiguredLeds());

    for (size_t index = 0; index < build::LedsMax; ++index) {
        const auto pin = settings::pin(index);
        if (!gpioLock(pin)) {
            break;
        }

        internal::leds.emplace_back(pin,
                settings::inverse(index), settings::mode(index));
    }

    const auto leds = internal::leds.size();
    DEBUG_MSG_P(PSTR("[LED] Number of leds: %u\n"), leds);
    if (leds) {
        led::settings::query::setup();
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
            schedule();
        });
#endif
#if TERMINAL_SUPPORT
        terminal::setup();
#endif

        ::espurnaRegisterLoop(loop);

        ::espurnaRegisterReload(configure);
        configure();
    }
}

} // namespace
} // namespace led
} // namespace led

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

#endif // LED_SUPPORT
