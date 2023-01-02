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
    return convert(options::FanSpeedOptions, value, FanSpeed::Off);
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

static constexpr auto DefaultSaveDelay = duration::Seconds{ 10 };

// We expect to write a specific 'mask' via GPIO LOW & HIGH to set the speed
// Sync up with the relay and write it on ON / OFF status events

constexpr size_t Gpios { 3ul };

using State = std::array<int8_t, Gpios>;

using Pin = std::pair<int, BasePinPtr>;
using StatePins = std::array<Pin, Gpios>;

// XXX: while these are hard-coded, we don't really benefit from having these in the hardware cfg

StatePins statePins() {
    return {
        {{5, nullptr},
        {4, nullptr},
        {15, nullptr}}
    };
}

constexpr int controlPin() {
    return 12;
}

struct Config {
    duration::Seconds save;
    FanSpeed speed;
};

Config readSettings() {
    return Config{
        .save = getSetting("fanSave", DefaultSaveDelay),
        .speed = getSetting("fanSpeed", FanSpeed::Medium)};
}

StatePins state_pins;
Config config {
    .save = DefaultSaveDelay,
    .speed = FanSpeed::Medium,
};

void configure() {
    config = readSettings();
}

void report(FanSpeed speed [[gnu::unused]]) {
#if MQTT_SUPPORT
    mqttSend(MQTT_TOPIC_SPEED, speedToPayload(speed).c_str());
#endif
}

void save(FanSpeed speed) {
    static timer::SystemTimer ticker;
    config.speed = speed;
    ticker.once(config.save, []() {
        const auto value = speedToPayload(config.speed);
        setSetting("fanSpeed", value);
        DEBUG_MSG_P(PSTR("[IFAN] Saved speed setting \"%s\"\n"), value.c_str());
    });
}

void cleanupPins(StatePins& pins) {
    for (auto& pin : pins) {
        if (!pin.second) continue;
        gpioUnlock(pin.second->pin());
        pin.second.reset(nullptr);
    }
}

StatePins setupStatePins() {
    StatePins pins = statePins();

    for (auto& pair : pins) {
        auto ptr = gpioRegister(pair.first);
        if (!ptr) {
            DEBUG_MSG_P(PSTR("[IFAN] Could not set up GPIO%d\n"), pair.first);
            cleanupPins(pins);
            return pins;
        }
        ptr->pinMode(OUTPUT);
        pair.second = std::move(ptr);
    }

    return pins;
}

State stateFromSpeed(FanSpeed speed) {
    switch (speed) {
    case FanSpeed::Low:
        return {HIGH, LOW, LOW};
    case FanSpeed::Medium:
        return {HIGH, HIGH, LOW};
    case FanSpeed::High:
        return {HIGH, LOW, HIGH};
    case FanSpeed::Off:
        break;
    }

    return {LOW, LOW, LOW};
}

const char* maskFromSpeed(FanSpeed speed) {
    switch (speed) {
    case FanSpeed::Low:
        return "0b100";
    case FanSpeed::Medium:
        return "0b110";
    case FanSpeed::High:
        return "0b101";
    case FanSpeed::Off:
        return "0b000";
    }

    return "";
}

// Note that we use API speed endpoint strictly for the setting
// (which also allows to pre-set the speed without turning the relay ON)

using FanSpeedUpdate = std::function<void(FanSpeed)>;

FanSpeedUpdate onFanSpeedUpdate = [](FanSpeed) {
};

void updateSpeed(Config& config, FanSpeed speed) {
    switch (speed) {
    case FanSpeed::Low:
    case FanSpeed::Medium:
    case FanSpeed::High:
        save(speed);
        report(speed);
        onFanSpeedUpdate(speed);
        break;
    case FanSpeed::Off:
        break;
    }
}

void updateSpeed(FanSpeed speed) {
    updateSpeed(config, speed);
}

void updateSpeedFromPayload(StringView payload) {
    updateSpeed(payloadToSpeed(payload.toString()));
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

class FanProvider : public RelayProviderBase {
public:
    FanProvider(BasePinPtr&& pin, const Config& config, const StatePins& pins, FanSpeedUpdate& callback) :
        _pin(std::move(pin)),
        _config(config),
        _pins(pins)
    {
        callback = [this](FanSpeed speed) {
            change(speed);
        };
        _pin->pinMode(OUTPUT);
    }

    const char* id() const override {
        return "fan";
    }

    void change(FanSpeed speed) {
        _pin->digitalWrite((FanSpeed::Off != speed) ? HIGH : LOW);

        auto state = stateFromSpeed(speed);
        DEBUG_MSG_P(PSTR("[IFAN] State mask: %s\n"), maskFromSpeed(speed));

        for (size_t index = 0; index < _pins.size(); ++index) {
            auto& pin = _pins[index].second;
            if (!pin) {
                continue;
            }

            pin->digitalWrite(state[index]);
        }
    }

    void change(bool status) override {
        change(status ? _config.speed : FanSpeed::Off);
    }

private:
    BasePinPtr _pin;
    const Config& _config;
    const StatePins& _pins;
};

#if TERMINAL_SUPPORT
namespace terminal {

PROGMEM_STRING(Speed, "SPEED");

void speed(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 2) {
        updateSpeedFromPayload(ctx.argv[1]);
    }

    ctx.output.printf_P(PSTR("%s %s\n"),
        (config.speed != FanSpeed::Off)
            ? PSTR("speed")
            : PSTR("fan is"),
        speedToPayload(config.speed).c_str());
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

void setup() {
    state_pins = setupStatePins();
    if (!state_pins.size()) {
        return;
    }

    configure();
    espurnaRegisterReload(configure);

    auto relay_pin = gpioRegister(controlPin());
    if (relay_pin) {
        auto provider = std::make_unique<FanProvider>(
            std::move(relay_pin), config, state_pins, onFanSpeedUpdate);

        const auto result = relayAdd(std::move(provider));
        if (result) {
            DEBUG_MSG_P(PSTR("[IFAN] Could not add relay provider for GPIO%d\n"), controlPin());
            gpioUnlock(controlPin());
        }
    }

#if MQTT_SUPPORT
    mqttRegister(onMqttEvent);
#endif

#if API_SUPPORT
    apiRegister(F(MQTT_TOPIC_SPEED),
        [](ApiRequest& request) {
            request.send(speedToPayload(config.speed));
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

}

} // namespace
} // namespace ifan02
} // namespace espurna

FanSpeed fanSpeed() {
    return espurna::ifan02::config.speed;
}

void fanSpeed(FanSpeed speed) {
    espurna::ifan02::updateSpeed(FanSpeed::Low);
}

void fanSetup() {
    espurna::ifan02::setup();
}

#endif // IFAN_SUPPORT
