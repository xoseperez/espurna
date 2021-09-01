/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

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
#include <forward_list>
#include <vector>

namespace {

struct LedPattern {
    using Delays = std::vector<LedDelay>;

    LedPattern(LedPattern&&) = default;
    LedPattern& operator=(LedPattern&&) = default;

    LedPattern() {
        init();
    }

    explicit LedPattern(const char* input);
    explicit LedPattern(const String& input) :
        LedPattern(input.c_str())
    {}

    explicit LedPattern(Delays&& delays) :
        _delays(std::move(delays))
    {
        init();
    }

    void cycle(unsigned long delay) {
        _last_cycle = ESP.getCycleCount();
        _cycle_delay = delay;
    }

    void init() {
        cycle(_delays.size() ? _delays.back().on() : 0);
    }

    void start() {
        cycle(0);
        _queue = {_delays.rbegin(), _delays.rend()};
    }

    void reset() {
        _queue.clear();
    }

    bool ready() const {
        return _delays.size() > 0;
    }

    bool last() const {
        return _queue.size() == 1;
    }

    bool started() const {
        return _queue.size() > 0;
    }

    const Delays& delays() const {
        return _delays;
    }

    template <typename T>
    void run(T&& callback) {
        if (!_queue.size()) {
            return;
        }

        if (ESP.getCycleCount() - _last_cycle < _cycle_delay) {
            return;
        }

        auto& current = _queue.back();
        if (!callback(current)) {
            _queue.pop_back();
            return;
        }
    }

    void pop() {
        if (_queue.size()) {
            _queue.pop_back();
        }
    }

private:
    Delays _delays;
    Delays _queue;
    unsigned long _last_cycle;
    unsigned long _cycle_delay;
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

    LedPattern& pattern() {
        return _pattern;
    }

    void pattern(LedPattern&& pattern) {
        _pattern = std::move(pattern);
    }

    void start() {
        _pattern.reset();
    }

    bool started() {
        return _pattern.started();
    }

    void stop() {
        _pattern.reset();
    }

    void init();

    bool status();
    bool status(bool new_status);

    bool toggle();

private:
    unsigned char _pin;
    bool _inverse;
    LedMode _mode;
    LedPattern _pattern;
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

// Scans input string with format
// '<on1>,<off1>,<repeats1> <on2>,<off2>,<repeats2> ...'
// And returns a list of Delay objects for the pattern

LedPattern::LedPattern(const char* input) {
    char buffer[16];

    const char* d1;
    const char* d2;
    const char* d3;

    const char* p = input;
    const char* marker;

loop:
    const char *yyt1;
    const char *yyt2;
    const char *yyt3;

	{
		char yych;
		yych = (char)*p;
		switch (yych) {
		case '\t':
		case ' ': goto yy4;
		case '0' ... '9':
			yyt1 = p;
			goto yy7;
		default: goto yy2;
		}
yy2:
		++p;
yy3:
		{ goto out; }
yy4:
		yych = (char)*++p;
		switch (yych) {
		case '\t':
		case ' ': goto yy4;
		default: goto yy6;
		}
yy6:
		{ goto loop; }
yy7:
		yych = (char)*(marker = ++p);
		switch (yych) {
		case ',': goto yy8;
		case '0' ... '9': goto yy10;
		default: goto yy3;
		}
yy8:
		yych = (char)*++p;
		switch (yych) {
		case '0' ... '9':
			yyt2 = p;
			goto yy12;
		default: goto yy9;
		}
yy9:
		p = marker;
		goto yy3;
yy10:
		yych = (char)*++p;
		switch (yych) {
		case ',': goto yy8;
		case '0' ... '9': goto yy10;
		default: goto yy9;
		}
yy12:
		yych = (char)*++p;
		switch (yych) {
		case ',': goto yy14;
		case '0' ... '9': goto yy12;
		default: goto yy9;
		}
yy14:
		yych = (char)*++p;
		switch (yych) {
		case '0' ... '9':
			yyt3 = p;
			goto yy15;
		default: goto yy9;
		}
yy15:
		yych = (char)*++p;
		switch (yych) {
		case '0' ... '9': goto yy15;
		default: goto yy17;
		}
yy17:
		d1 = yyt1;
		d2 = yyt2;
		d3 = yyt3;
		{
            unsigned long on;
            unsigned long off;
            unsigned char repeats;

            memcpy(buffer, d1, int(d2 - d1));
            buffer[int(d2 - d1 - 1)] = '\0';
            on = strtoul(buffer, nullptr, 10);

            memcpy(buffer, d2, int(d3 - d2));
            buffer[int(d3 - d2 - 1)] = '\0';
            off = strtoul(buffer, nullptr, 10);

            memcpy(buffer, d3, int(p - d3));
            buffer[int(p - d3)] = '\0';
            repeats = strtoul(buffer, nullptr, 10);

            _delays.emplace_back(on, off, repeats);

            goto loop;
        }
	}

out:
    init();
}

} // namespace

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

[[gnu::unused]]
String serialize(const LedPattern& pattern) {
    String out;

    for (auto& delay : pattern.delays()) {
        if (out.length()) {
            out += ' ';
        }

        out += String(delay.on(), 10);
        out += ',';
        out += String(delay.off(), 10);
        out += ',';
        out += String(delay.repeats(), 10);
    }

    return out;
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
    return getSetting({"ledGpio", id}, build::pin(id));
}

LedMode mode(size_t id) {
    return getSetting({"ledMode", id}, build::mode(id));
}

bool inverse(size_t id) {
    return getSetting({"ledInv", id}, build::inverse(id));
}

#if RELAY_SUPPORT
size_t relay(size_t id) {
    return getSetting({"ledRelay", id}, build::relay(id));
}
#endif

LedPattern pattern(size_t id) {
    return LedPattern(getSetting({"ledPattern", id}));
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

// For network-based modes, cycle ON & OFF (time in milliseconds)
// XXX: internals convert these to clock cycles, delay cannot be longer than 25000 / 50000 ms
constexpr LedDelay NetworkConnected{100, 4900};
constexpr LedDelay NetworkConnectedInverse{4900, 100};
constexpr LedDelay NetworkConfig{100, 900};
constexpr LedDelay NetworkConfigInverse{900, 100};
constexpr LedDelay NetworkIdle{500, 500};

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
        KeyDefault{"ledMode", KEY_DEFAULT_FUNC(mode)},
        KeyDefault{"ledInv", KEY_DEFAULT_FUNC(inverse)},
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

bool isLinked(const Link& link, const Led& led) {
    return &link.led == &led;
}

void unlink(Led& led) {
    relays.remove_if([&](const Link& link) {
        return isLinked(link, led);
    });
}

void link(Led& led, size_t id) {
    auto it = std::find_if(relays.begin(), relays.end(), [&](const Link& link) {
        return isLinked(link, led);
    });

    if (it != relays.end()) {
        (*it).relayId = id;
        return;
    }

    relays.emplace_front(Link{led, id});
}

size_t find(Led& led) {
    auto it = std::find_if(relays.begin(), relays.end(), [&](const Link& link) {
        return isLinked(link, led);
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
    auto& pattern = led.pattern();
    if (pattern.ready()) {
        if (status) {
            if (!pattern.started()) {
                pattern.start();
            }
            result = true;
        } else {
            pattern.reset();
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
void pattern(Led& led, LedPattern&& other) {
    led.pattern(std::move(other));
    status(led, true);
}

void run(Led& led) {
    auto& pattern = led.pattern();
    pattern.run([&](LedDelay& current) {
        const bool status = led.toggle();

        switch (current.mode()) {
        case LedDelayMode::Finite:
            if (status && current.repeat()) {
                if (pattern.last()) {
                    led.status(false);
                    return false;
                }
            }
            break;
        case LedDelayMode::Infinite:
        case LedDelayMode::None:
            break;
        }

        pattern.cycle(status ? current.on() : current.off());
        return true;
    });
}

void run(Led& led, const LedDelay& delays) {
    static auto clock_last = ESP.getCycleCount();
    static auto delay_for = delays.on();

    const auto clock_current = ESP.getCycleCount();
    if (clock_current - clock_last >= delay_for) {
        delay_for = led.toggle() ? delays.on() : delays.off();
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

#if RELAY_SUPPORT

    case LED_MODE_FINDME_WIFI:
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
            break;

    case LED_MODE_RELAY_WIFI:
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
        break;

    case LED_MODE_FOLLOW:
        if (scheduled()) {
            status(led, relay::status(led));
        }
        break;

    case LED_MODE_FOLLOW_INVERSE:
        if (scheduled()) {
            status(led, !relay::status(led));
        }
        break;

    case LED_MODE_FINDME:
        if (scheduled()) {
            led::status(led, !relay::areAnyOn());
        }
        break;

    case LED_MODE_RELAY:
        if (scheduled()) {
            led::status(led, relay::areAnyOn());
        }
        break;

#endif // RELAY_SUPPORT == 1

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

    run(led);
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
            pattern(led, LedPattern(payload));
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
    // TODO: add ledPattern?
    // TODO: serialize()? although, bool will produce `true` / `false` and not a short number result. and it would be a dynamic string entry

    JsonObject& config = root.createNestedObject("ledConfig");

    {
        JsonArray& schema = config.createNestedArray("schema");
        schema.add("ledGpio");
        schema.add("ledMode");
        schema.add("ledInv");
#if RELAY_SUPPORT
        schema.add("ledRelay");
#endif
    }

    JsonArray& leds = config.createNestedArray("leds");

    const size_t Leds { count() };
    for (size_t index = 0; index < Leds; ++index) {
        JsonArray& led = leds.createNestedArray();
        led.add(settings::pin(index));
        led.add(static_cast<int>(settings::inverse(index)));
        led.add(static_cast<int>(settings::mode(index)));
#if RELAY_SUPPORT
        led.add(settings::relay(index));
#endif
    }
}

} // namespace web
#endif // WEB_SUPPORT

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
