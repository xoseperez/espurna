/*

iFan02 MODULE

Copyright (C) 2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Original implementation via RELAY module
Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if IFAN_SUPPORT

#include "api.h"
#include "fan.h"
#include "mqtt.h"
#include "relay.h"
#include "terminal.h"

#include <array>
#include <utility>

// TODO: in case there are more FANs, move externally

namespace espurna {
namespace settings {
namespace options {
namespace {

PROGMEM_STRING(Off, "off");
PROGMEM_STRING(Low, "low");
PROGMEM_STRING(Medium, "medium");
PROGMEM_STRING(High, "high");

static constexpr std::array<espurna::settings::options::Enumeration<FanSpeed>, 4> FanSpeedOptions PROGMEM {
    {{FanSpeed::Off, Off},
     {FanSpeed::Low, Low},
     {FanSpeed::Medium, Medium},
     {FanSpeed::High, High}}
};

} // namespace
} // namespace options

namespace internal {

template <>
FanSpeed convert(const String& value) {
    return convert(options::FanSpeedOptions, value, FanSpeed::Medium);
}

String serialize(FanSpeed speed) {
    return serialize(options::FanSpeedOptions, speed);
}

} // namespace internal
} // namespace settings

namespace ifan02 {
namespace {

FanSpeed payloadToSpeed(const String& value) {
    return espurna::settings::internal::convert<FanSpeed>(value);
}

String speedToPayload(FanSpeed speed) {
    return espurna::settings::internal::serialize(speed);
}

namespace build {

static constexpr auto ControlPin = uint8_t{ 12 };

static constexpr auto SaveDelay = duration::Seconds{ 10 };
static constexpr auto Speed = FanSpeed::Medium;

} // namespace build

namespace settings {
namespace keys {

PROGMEM_STRING(Save, "fanSave");
PROGMEM_STRING(Speed, "fanSpeed");

} // namespace keys

duration::Seconds save() {
    return getSetting(keys::Save, build::SaveDelay);
}

FanSpeed speed() {
    return getSetting(keys::Speed, build::Speed);
}

} // namespace settings

// We expect to write a specific 'mask' via GPIO LOW & HIGH to set the speed
// Sync up with the relay and write it on ON / OFF status events

struct Pin {
    unsigned char init;
    BasePinPtr handle;
};

struct StatePins {
    static constexpr size_t Gpios { 3ul };
    using State = std::array<int8_t, Gpios>;
    using Pins = std::array<Pin, Gpios>;

    StatePins(const StatePins&) = delete;

    StatePins() = default;
    ~StatePins() {
        reset();
    }

    StatePins(StatePins&&) = default;

    bool init();

    bool initialized() const {
        return _initialized;
    }

    void reset();

    State state(FanSpeed);
    State update(FanSpeed);

    State state() const {
        return _state;
    }

    String mask();

private:
    // XXX: while these are hard-coded, we don't really benefit from having these in the hardware cfg
    bool _initialized { false };
    Pins _pins{{
        Pin{5, nullptr},
        Pin{4, nullptr},
        Pin{15, nullptr}}};

    State _state{{LOW, LOW, LOW}};
};

StatePins::State StatePins::state(FanSpeed speed) {
    switch (speed) {
    case FanSpeed::Low:
        _state = {HIGH, LOW, LOW};
        break;
    case FanSpeed::Medium:
        _state = {HIGH, HIGH, LOW};
        break;
    case FanSpeed::High:
        _state = {HIGH, LOW, HIGH};
        break;
    case FanSpeed::Off:
        _state = {LOW, LOW, LOW};
        break;
    }

    return _state;
}

String StatePins::mask() {
    String out("0b000");
    for (size_t index = 2; index != out.length(); ++index) {
        out[index] = (_state[index - 2] == HIGH) ? '1' : '0';
    }

    return out;
}

void StatePins::reset() {
    for (auto& pin : _pins) {
        if (pin.handle) {
            gpioUnlock(pin.handle->pin());
            pin.handle.reset(nullptr);
        }
    }
}

bool StatePins::init() {
    if (_initialized) {
        return true;
    }

    for (auto& pair : _pins) {
        pair.handle = gpioRegister(pair.init);
        if (!pair.handle) {
            DEBUG_MSG_P(PSTR("[IFAN] Could not set up GPIO%hhu\n"), pair.init);
            reset();
            return false;
        }

        pair.handle->pinMode(OUTPUT);
    }

    _initialized = true;
    return true;
}

StatePins::State StatePins::update(FanSpeed speed) {
    const auto out = state(speed);

    for (size_t index = 0; index < _pins.size(); ++index) {
        auto& handle = _pins[index].handle;
        if (!handle) {
            continue;
        }

        handle->digitalWrite(_state[index]);
    }

    return out;
}

struct ControlPin {
    ~ControlPin() {
        reset();
    }

    explicit operator bool() const {
        return static_cast<bool>(_pin);
    }

    ControlPin& operator=(uint8_t pin) {
        reset();

        _pin = gpioRegister(pin);
        if (_pin) {
            _pin->pinMode(OUTPUT);
        }

        return *this;
    }

    ControlPin& operator=(BasePinPtr pin) {
        reset();
        _pin = std::move(pin);
        return *this;
    }

    void reset() {
        if (_pin) {
            gpioUnlock(_pin->pin());
            _pin.reset(nullptr);
        }
    }

    BasePin* operator->() {
        return _pin.get();
    }

    BasePin* operator->() const {
        return _pin.get();
    }

private:
    BasePinPtr _pin;
};

struct Config {
    duration::Seconds save;
    FanSpeed speed;
};

namespace internal {

timer::SystemTimer config_timer;
Config config;

size_t relay_id { RelaysMax };
ControlPin control_pin;
FanSpeed speed { FanSpeed::Off };

StatePins state_pins;

} // namespace internal

bool currentStatus() {
    return internal::speed != FanSpeed::Off;
}

void currentStatus(bool status) {
    internal::speed = status
        ? internal::config.speed
        : FanSpeed::Off;
}

FanSpeed currentSpeed() {
    return internal::speed;
}

String speedToPayload() {
    return speedToPayload(currentSpeed());
}

void save(FanSpeed speed) {
    internal::config.speed = speed;

    if (FanSpeed::Off != speed) {
        internal::config_timer.once(
            internal::config.save,
            [speed]() {
                const auto value = speedToPayload(speed);
                setSetting(settings::keys::Speed, value);
                DEBUG_MSG_P(PSTR("[IFAN] Saved speed \"%s\"\n"), value.c_str());
            });
    }
}

void report(FanSpeed speed [[gnu::unused]]) {
#if MQTT_SUPPORT
    mqttSend(MQTT_TOPIC_SPEED, speedToPayload(speed).c_str());
#endif
}

void pin_update(FanSpeed speed) {
    const bool status = FanSpeed::Off != speed;

    relayStatus(internal::relay_id, status);
    internal::control_pin->digitalWrite(status ? HIGH : LOW);

    internal::state_pins.update(speed);
}

void pin_update() {
    pin_update(internal::speed);
}

FanSpeed update(FanSpeed value) {
    const auto last = internal::speed;
    if (value != last) {
        save(value);
        report(value);
    }

    internal::speed = value;
    pin_update(value);

    return value;
}

FanSpeed update(bool status) {
    currentStatus(status);
    return update(internal::speed);
}

void configure() {
    const auto updated = Config{
        .save = settings::save(),
        .speed = settings::speed()};

    internal::config = updated;
    pin_update(); 
}

// Note that we use API speed endpoint strictly for the setting
// (which also allows to pre-set the speed without turning the relay ON)

FanSpeed updateSpeedFromPayload(StringView payload) {
    return update(payloadToSpeed(payload.toString()));
}

#if MQTT_SUPPORT

void onMqttEvent(unsigned int type, StringView topic, StringView payload) {
    switch (type) {

    case MQTT_CONNECT_EVENT:
        mqttSubscribe(MQTT_TOPIC_SPEED);
        break;

    case MQTT_MESSAGE_EVENT: {
        auto parsed = mqttMagnitude(topic);
        if (parsed.startsWith(MQTT_TOPIC_SPEED)) {
            updateSpeedFromPayload(payload);
        }
        break;
    }

    }
}

#endif // MQTT_SUPPORT

class FanRelayProvider : public RelayProviderBase {
public:
    espurna::StringView id() const override {
        return STRING_VIEW("fan");
    }

    void change(bool status) override {
        ifan02::update(status);
    }

private:
    BasePinPtr _pin;
};

#if TERMINAL_SUPPORT
namespace terminal {

PROGMEM_STRING(Speed, "SPEED");

void speed(::terminal::CommandContext&& ctx) {
    auto value = ifan02::currentSpeed();
    if (ctx.argv.size() == 2) {
        value = updateSpeedFromPayload(ctx.argv[1]);
    }

    ctx.output.printf_P(PSTR("%s %s\n"),
        (value != FanSpeed::Off)
            ? PSTR("speed")
            : PSTR("fan is"),
        speedToPayload(value).c_str());
    terminalOK(ctx);
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Speed, speed},
};

void setup() {
    espurna::terminal::add(Commands);
}

} // namespace terminal
#endif

bool setup() {
    if (internal::control_pin && internal::state_pins.initialized()) {
        return true;
    }

    internal::control_pin = build::ControlPin;
    if (!internal::state_pins.init()) {
        internal::control_pin.reset();
        return false;
    }

    configure();
    espurnaRegisterReload(configure);

#if MQTT_SUPPORT
    mqttRegister(onMqttEvent);
#endif

#if API_SUPPORT
    apiRegister(F(MQTT_TOPIC_SPEED),
        [](ApiRequest& request) {
            request.send(speedToPayload());
            return true;
        },
        [](ApiRequest& request) {
            updateSpeedFromPayload(request.param(F("value")));
            return true;
        }
    );
#endif

#if TERMINAL_SUPPORT
    terminal::setup();
#endif

    return true;
}

RelayProviderBasePtr make_relay_provider(size_t index) {
    RelayProviderBasePtr out;

    if (setup()) {
        out = std::make_unique<FanRelayProvider>(); 
        internal::relay_id = index;
    }

    return out;
}


} // namespace
} // namespace ifan02
} // namespace espurna

RelayProviderBasePtr fanMakeRelayProvider(size_t index) {
    return espurna::ifan02::make_relay_provider(index);
}

void fanStatus(bool value) {
    espurna::ifan02::currentStatus(value);
}

bool fanStatus() {
    return espurna::ifan02::currentStatus();
}

FanSpeed fanSpeed() {
    return espurna::ifan02::currentSpeed();
}

void fanSpeed(FanSpeed speed) {
    espurna::ifan02::update(speed);
}

void fanSetup() {
    espurna::ifan02::setup();
}

#endif // IFAN_SUPPORT
