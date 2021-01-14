/*

iFan02 MODULE

Copyright (C) 2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Original implementation via RELAY module
Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if IFAN_SUPPORT

#include "api.h"
#include "button.h"
#include "mqtt.h"
#include "relay.h"
#include "terminal.h"

#include <array>
#include <utility>

namespace ifan02 {

enum class Speed {
    Off,
    Low,
    Medium,
    High
};

const char* speedToPayload(Speed value) {
    switch (value) {
    case Speed::Off:
        return "off";
    case Speed::Low:
        return "low";
    case Speed::Medium:
        return "medium";
    case Speed::High:
        return "high";
    }

    return "";
}

Speed payloadToSpeed(const char* payload) {
    auto len = strlen(payload);
    if (len == 1) {
        switch (payload[0]) {
        case '0':
            return Speed::Off;
        case '1':
            return Speed::Low;
        case '2':
            return Speed::Medium;
        case '3':
            return Speed::High;
        }
    } else if (len > 1) {
        String cmp(payload);
        if (cmp == "off") {
            return Speed::Off;
        } else if (cmp == "low") {
            return Speed::Low;
        } else if (cmp == "medium") {
            return Speed::Medium;
        } else if (cmp == "high") {
            return Speed::High;
        }
    }

    return Speed::Off;
}

Speed payloadToSpeed(const String& string) {
    return payloadToSpeed(string.c_str());
}

} // namespace ifan02

namespace settings {
namespace internal {

template <>
ifan02::Speed convert(const String& value) {
    return ifan02::payloadToSpeed(value);
}

} // namespace internal
} // namespace settings

namespace ifan02 {

constexpr unsigned long DefaultSaveDelay { 1000ul };

// Remote presses trigger GPIO pushbutton events
// Attach to a specific ID to trigger an action

constexpr unsigned char DefaultLowButtonId { 1u };
constexpr unsigned char DefaultMediumButtonId { 2u };
constexpr unsigned char DefaultHighButtonId { 3u };

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
    Config() = default;
    explicit Config(unsigned long save_, unsigned char buttonLowId_,
            unsigned char buttonMediumId_, unsigned char buttonHighId_, Speed speed_) :
        save(save_),
        buttonLowId(buttonLowId_),
        buttonMediumId(buttonMediumId_),
        buttonHighId(buttonHighId_),
        speed(speed_)
    {}

    unsigned long save { DefaultSaveDelay };
    unsigned char buttonLowId { DefaultLowButtonId };
    unsigned char buttonMediumId { DefaultMediumButtonId };
    unsigned char buttonHighId { DefaultHighButtonId };
    Speed speed { Speed::Off };
    StatePins state_pins;
};

Config readSettings() {
    return Config(
        getSetting("ifanSave", DefaultSaveDelay),
        getSetting("ifanBtnLowId", DefaultLowButtonId),
        getSetting("ifanBtnMediumId", DefaultMediumButtonId),
        getSetting("ifanBtnHighId", DefaultHighButtonId),
        getSetting("ifanSpeed", Speed::Medium)
    );
}

Config config;

void configure() {
    config = readSettings();
}

void report(Speed speed [[gnu::unused]]) {
#if MQTT_SUPPORT
    mqttSend(MQTT_TOPIC_SPEED, speedToPayload(speed));
#endif
}

void save(Speed speed) {
    static Ticker ticker;
    config.speed = speed;
    ticker.once_ms(config.save, []() {
        const char* value = speedToPayload(config.speed);
        setSetting("ifanSpeed", value);
        DEBUG_MSG_P(PSTR("[IFAN] Saved speed setting \"%s\"\n"), value);
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

State stateFromSpeed(Speed speed) {
    switch (speed) {
    case Speed::Low:
        return {HIGH, LOW, LOW};
    case Speed::Medium:
        return {HIGH, HIGH, LOW};
    case Speed::High:
        return {HIGH, LOW, HIGH};
    case Speed::Off:
        break;
    }

    return {LOW, LOW, LOW};
}

const char* maskFromSpeed(Speed speed) {
    switch (speed) {
    case Speed::Low:
        return "0b100";
    case Speed::Medium:
        return "0b110";
    case Speed::High:
        return "0b101";
    case Speed::Off:
        return "0b000";
    }

    return "";
}

// Note that we use API speed endpoint strictly for the setting
// (which also allows to pre-set the speed without turning the relay ON)

using FanSpeedUpdate = std::function<void(Speed)>;

FanSpeedUpdate onSpeedUpdate = [](Speed) {
};

void updateSpeed(Config& config, Speed speed) {
    switch (speed) {
    case Speed::Low:
    case Speed::Medium:
    case Speed::High:
        save(speed);
        report(speed);
        onSpeedUpdate(speed);
        break;
    case Speed::Off:
        break;
    }
}

void updateSpeed(Speed speed) {
    updateSpeed(config, speed);
}

void updateSpeedFromPayload(const char* payload) {
    updateSpeed(payloadToSpeed(payload));
}

void updateSpeedFromPayload(const String& payload) {
    updateSpeedFromPayload(payload.c_str());
}

#if MQTT_SUPPORT

void onMqttEvent(unsigned int type, const char* topic, const char* payload) {
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
    explicit FanProvider(BasePinPtr&& pin, const Config& config, FanSpeedUpdate& callback) :
        _pin(std::move(pin)),
        _config(config)
    {
        callback = [this](Speed speed) {
            change(speed);
        };
    }

    const char* id() const override {
        return "fan";
    }

    void change(Speed speed) {
        _pin->digitalWrite((Speed::Off != speed) ? HIGH : LOW);

        auto state = stateFromSpeed(speed);
        DEBUG_MSG_P(PSTR("[IFAN] State mask: %s\n"), maskFromSpeed(speed));

        for (size_t index = 0; index < _config.state_pins.size(); ++index) {
            auto& pin = _config.state_pins[index].second;
            if (!pin) {
                continue;
            }

            pin->digitalWrite(state[index]);
        }
    }

    void change(bool status) override {
        change(status ? _config.speed : Speed::Off);
    }

private:
    BasePinPtr _pin;
    const Config& _config;
};

void setup() {

    config.state_pins = setupStatePins();
    if (!config.state_pins.size()) {
        return;
    }

    configure();

    espurnaRegisterReload(configure);

    auto relay_pin = gpioRegister(controlPin());
    if (relay_pin) {
        auto provider = std::make_unique<FanProvider>(std::move(relay_pin), config, onSpeedUpdate);
        if (!relayAdd(std::move(provider))) {
            DEBUG_MSG_P(PSTR("[IFAN] Could not add relay provider for GPIO%d\n"), relay_pin->pin());
            gpioUnlock(relay_pin->pin());
        }
    }

#if BUTTON_SUPPORT
    buttonSetCustomAction([](unsigned char id) {
        if (config.buttonLowId == id) {
            updateSpeed(Speed::Low);
        } else if (config.buttonMediumId == id) {
            updateSpeed(Speed::Medium);
        } else if (config.buttonHighId == id) {
            updateSpeed(Speed::High);
        }
    });
#endif

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
    terminalRegisterCommand(F("SPEED"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            updateSpeedFromPayload(ctx.argv[1]);
        }

        ctx.output.println(speedToPayload(config.speed));
        terminalOK(ctx);
    });
#endif

}

} // namespace ifan

void ifanSetup() {
    ifan02::setup();
}

#endif // IFAN_SUPPORT
