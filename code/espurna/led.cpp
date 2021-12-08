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

namespace led {
namespace {

// Some local-only time & counters implementation:
// - Core conversion is done through macros, implement stronger types
// - force unsigned instead of chrono's 'int64_t', since we want safe overflow
// - bound to 32bits, to seamlessly handle ccount conversion from the 'time source'
// - explicitly check for the maximum number of milliseconds that may be represented with ccount

// TODO: full-width int for repeats instead of 8bit? right now, string parser will *force* [min:max] range,
// but anything else is experiencing overflow mechanics

struct Delay {
    using Source = espurna::time::CpuClock;
    using Duration = Source::duration;
    using TimePoint = Source::time_point;

    static constexpr auto ClockCyclesMax = Duration(Duration::max());
    static constexpr auto MillisecondsMax = std::chrono::duration_cast<espurna::duration::Milliseconds>(ClockCyclesMax);

    using Repeats = unsigned char;
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
} // namespace led

// -----------------------------------------------------------------------------

namespace settings {
namespace internal {

template <>
LedMode convert(const String& value) {
    if (value.length() == 1) {
        switch (*value.c_str()) {
        case '0':
            return LedMode::Manual;
        case '1':
            return LedMode::WiFi;
#if RELAY_SUPPORT
        case '2':
            return LedMode::Follow;
        case '3':
            return LedMode::FollowInverse;
        case '4':
            return LedMode::FindMe;
        case '5':
            return LedMode::FindMeWiFi;
#endif
        case '6':
            return LedMode::On;
        case '7':
            return LedMode::Off;
#if RELAY_SUPPORT
        case '8':
            return LedMode::Relay;
        case '9':
            return LedMode::RelayWiFi;
#endif
        }
    }

    return LedMode::Manual;
}

String serialize(LedMode mode) {
    return String(static_cast<int>(mode), 10);
}

#if 0
String serialize(espurna::duration::ClockCycles value) {
    String out;
    out.reserve(16);

    out += std::chrono::duration_cast<Milliseconds>(value).count();

    return out;
}

[[gnu::unused]]
String serialize(const led::Pattern& pattern) {
    String out;

    for (auto& delay : pattern.delays()) {
        if (out.length()) {
            out += ' ';
        }

        out += serialize(delay.on());
        out += ',';
        out += serialize(delay.off());
        out += ',';
        out += String(delay.repeats(), 10);
    }

    return out;
}
#endif

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
    return getSetting({"ledGpio", id}, build::pin(id));
}

LedMode mode(size_t id) {
    return getSetting({"ledMode", id}, build::mode(id));
}

bool inverse(size_t id) {
    return getSetting({"ledInv", id}, build::inverse(id));
}

size_t relay(size_t id) {
    return getSetting({"ledRelay", id}, build::relay(id));
}

Pattern pattern(size_t id) {
    return Pattern(getSetting({"ledPattern", id}));
}

void migrate(int version) {
    if (version < 5) {
        delSettingPrefix({
            "ledGPIO",
            "ledGpio",
            "ledLogic"
        });
    }
}

} // namespace settings

// For network-based modes, indefinitely cycle ON <-> OFF
// (TODO: template params containing structs like duration need -std=c++2a)

#define LED_STATIC_DELAY(NAME, ON, OFF)\
    static constexpr auto NAME ## MillisecondsOn = espurna::duration::Milliseconds(ON);\
    static constexpr auto NAME ## MillisecondsOff = espurna::duration::Milliseconds(OFF);\
    static_assert(NAME ## MillisecondsOn < Delay::MillisecondsMax, "");\
    static_assert(NAME ## MillisecondsOff < Delay::MillisecondsMax, "");\
    static constexpr Delay NAME = Delay {\
        std::chrono::duration_cast<Delay::Duration>(NAME ## MillisecondsOn),\
        std::chrono::duration_cast<Delay::Duration>(NAME ## MillisecondsOff),\
        Delay::RepeatsMin }

LED_STATIC_DELAY(NetworkConnected, 100, 4900);
LED_STATIC_DELAY(NetworkConnectedInverse, 4900, 100);
LED_STATIC_DELAY(NetworkConfig, 100, 900);
LED_STATIC_DELAY(NetworkConfigInverse, 900, 100);
LED_STATIC_DELAY(NetworkIdle, 500, 500);

namespace internal {

std::vector<Led> leds;
bool update { false };

} // namespace internal

namespace settings {

struct KeyDefault {
    using SerializedFunc = String(*)(size_t);

    KeyDefault() = delete;
    explicit KeyDefault(String key, SerializedFunc func) :
        _key(std::move(key)),
        _func(func)
    {}

    bool match(const String& key, size_t id) const {
        return SettingsKey(_key, id) == key;
    }

    String serialized(size_t id) const {
        return _func(id);
    }

private:
    String _key;
    SerializedFunc _func;
};

#define KEY_DEFAULT_FUNC(X)\
    [](size_t id) {\
        return ::settings::internal::serialize(X(id));\
    }

using KeyDefaults = std::array<KeyDefault, 4>;
KeyDefaults keyDefaults() {
    return {
        KeyDefault{"ledGpio", KEY_DEFAULT_FUNC(pin)},
        KeyDefault{"ledInv", KEY_DEFAULT_FUNC(inverse)},
        KeyDefault{"ledMode", KEY_DEFAULT_FUNC(mode)},
        KeyDefault{"ledRelay", KEY_DEFAULT_FUNC(relay)}};
}

#undef KEY_DEFAULT_FUNC

String findKeyDefault(const KeyDefaults& defaults, const String& key) {
    for (size_t id = 0; id < internal::leds.size(); ++id) {
        for (auto& keyDefault : defaults) {
            if (keyDefault.match(key, id)) {
                return keyDefault.serialized(id);
            }
        }
    }

    return {};
}

String findKeyDefault(const String& key) {
    return findKeyDefault(keyDefaults(), key);
}

} // namespace settings

#if RELAY_SUPPORT
namespace relay {
namespace internal {

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

} // namespace internal

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

} // namespace relay
#endif

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
        case LED_MODE_FINDME_WIFI:
        case LED_MODE_RELAY_WIFI:
        case LED_MODE_FOLLOW:
        case LED_MODE_FOLLOW_INVERSE:
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

    case LED_MODE_MANUAL:
        break;

    case LED_MODE_WIFI:
        if (wifiConnected()) {
            run(led, NetworkConnected);
        } else if (wifiConnectable()) {
            run(led, NetworkConfig);
        } else {
            run(led, NetworkIdle);
        }
        break;

    case LED_MODE_FINDME_WIFI:
#if RELAY_SUPPORT
        if (wifiConnected()) {
            if (relay::status(led)) {
                run(led, NetworkConnected);
            } else {
                run(led, NetworkConnectedInverse);
            }
        } else if (wifiConnectable()) {
            if (relay::status(led)) {
                run(led, NetworkConfig);
            } else {
                run(led, NetworkConfigInverse);
            }
        } else {
            run(led, NetworkIdle);
        }
#endif
        break;

    case LED_MODE_RELAY_WIFI:
#if RELAY_SUPPORT
        if (wifiConnected()) {
            if (relay::status(led)) {
                run(led, NetworkConnected);
            } else {
                run(led, NetworkConnectedInverse);
            }
        } else if (wifiConnectable()) {
            if (relay::status(led)) {
                run(led, NetworkConfig);
            } else {
                run(led, NetworkConfigInverse);
            }
        } else {
            run(led, NetworkIdle);
        }
#endif
        break;

    case LED_MODE_FOLLOW:
#if RELAY_SUPPORT
        if (scheduled()) {
            status(led, relay::status(led));
        }
#endif
        break;

    case LED_MODE_FOLLOW_INVERSE:
#if RELAY_SUPPORT
        if (scheduled()) {
            status(led, !relay::status(led));
        }
#endif
        break;

    case LED_MODE_FINDME:
#if RELAY_SUPPORT
        if (scheduled()) {
            led::status(led, !relay::areAnyOn());
        }
#endif
        break;

    case LED_MODE_RELAY:
#if RELAY_SUPPORT
        if (scheduled()) {
            led::status(led, relay::areAnyOn());
        }
#endif
        break;

    case LED_MODE_ON:
        if (scheduled()) {
            status(led, true);
        }
        break;

    case LED_MODE_OFF:
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

#if MQTT_SUPPORT
namespace mqtt {

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
        if (led.mode() != LED_MODE_MANUAL) {
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

} // namespace mqtt
#endif // MQTT_SUPPORT

#if WEB_SUPPORT
namespace web {

bool onKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "led", 3) == 0);
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, "led");
}

void onConnected(JsonObject& root) {
    if (!count()) {
        return;
    }

    // TODO: something compatible with the settings defaults, to display module config in the terminal as well
    // TODO: add ledPattern strings from settings?
    // TODO: serialize()? although, bool will produce `true` / `false` and not a short number result. and it would be a dynamic string entry

    ::web::ws::EnumerableConfig config{root, F("ledConfig")};
    config(F("leds"), ::led::count(), {
        {F("ledGpio"), [](JsonArray& led, size_t index) {
            led.add(settings::pin(index));
        }},
        {F("ledInv"), [](JsonArray& led, size_t index) {
            led.add(static_cast<int>(settings::inverse(index)));
        }},
        {F("ledMode"), [](JsonArray& led, size_t index) {
            led.add(static_cast<int>(settings::mode(index)));
        }},
#if RELAY_SUPPORT
        {F("ledRelay"), [](JsonArray& led, size_t index) {
            led.add(settings::relay(index));
        }}
#endif
    });
}

} // namespace web
#endif // WEB_SUPPORT

#if TERMINAL_SUPPORT
namespace terminal {

void setup() {
    terminalRegisterCommand(F("LED"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() > 1) {
            size_t id;
            if (!tryParseId(ctx.argv[1].c_str(), ledCount, id)) {
                terminalError(ctx, F("Invalid ledID"));
                return;
            }

            auto& led = internal::leds[id];
            if (ctx.argv.size() > 2) {
                led.mode(LedMode::Manual);
                pattern(led, Pattern(ctx.argv[2]));
            } else {
                led.mode(settings::mode(id));
                led.pattern(settings::pattern(id));
            }

            schedule();
            terminalOK(ctx);

            return;
        }

        terminalError(ctx, F("LED <ID> [<PATTERN>]"));
    });
}

} // namespace terminal
#endif

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

    auto leds = count();
    DEBUG_MSG_P(PSTR("[LED] Number of leds: %u\n"), leds);
    if (leds) {
        ::settingsRegisterDefaults("led", settings::findKeyDefault);
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

bool ledStatus(size_t id, bool status) {
    if (id < led::count()) {
        return led::status(id, status);
    }

    return status;
}

bool ledStatus(size_t id) {
    if (id < led::count()) {
        return led::status(id);
    }

    return false;
}

size_t ledCount() {
    return led::count();
}

void ledLoop() {
    led::loop();
}

void ledSetup() {
    led::setup();
}

#endif // LED_SUPPORT
