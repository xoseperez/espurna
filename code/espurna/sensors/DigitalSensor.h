// -----------------------------------------------------------------------------
// Digital Sensor (maps to a digitalRead)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && DIGITAL_SUPPORT

#pragma once

#include "BaseSensor.h"

namespace espurna {
namespace sensor {
namespace driver {
namespace digital {

enum class State {
    Low,
    High,
    Initial,
};

enum Mode {
    Input,
    PullUp,
    PullDown,
};

namespace {
namespace build {

constexpr size_t SensorsMax { 8 };

constexpr unsigned char pin(unsigned char index) {
    return (0 == index) ? (DIGITAL1_PIN) :
        (1 == index) ? (DIGITAL2_PIN) :
        (2 == index) ? (DIGITAL3_PIN) :
        (3 == index) ? (DIGITAL4_PIN) :
        (4 == index) ? (DIGITAL5_PIN) :
        (5 == index) ? (DIGITAL6_PIN) :
        (6 == index) ? (DIGITAL7_PIN) :
        (7 == index) ? (DIGITAL8_PIN) : (GPIO_NONE);
}

constexpr Mode mode_from_value(int value) {
    return (INPUT == value) ? Mode::Input :
        (INPUT_PULLUP == value) ? Mode::PullUp :
        (INPUT_PULLDOWN == value) ? Mode::PullDown : Mode::Input;
}

constexpr Mode mode(unsigned char index) {
    return mode_from_value(
        (0 == index) ? (DIGITAL1_PIN_MODE) :
        (1 == index) ? (DIGITAL2_PIN_MODE) :
        (2 == index) ? (DIGITAL3_PIN_MODE) :
        (3 == index) ? (DIGITAL4_PIN_MODE) :
        (4 == index) ? (DIGITAL5_PIN_MODE) :
        (5 == index) ? (DIGITAL6_PIN_MODE) :
        (6 == index) ? (DIGITAL7_PIN_MODE) :
        (7 == index) ? (DIGITAL8_PIN_MODE) : (INPUT_PULLUP));
}

constexpr State state_from_value(int value) {
    return (HIGH == value) ? State::High :
        (LOW == value) ? State::Low : State::Initial;
}

constexpr State state(unsigned char index) {
    return state_from_value(
        (0 == index) ? (DIGITAL1_DEFAULT_STATE) :
        (1 == index) ? (DIGITAL2_DEFAULT_STATE) :
        (2 == index) ? (DIGITAL3_DEFAULT_STATE) :
        (3 == index) ? (DIGITAL4_DEFAULT_STATE) :
        (4 == index) ? (DIGITAL5_DEFAULT_STATE) :
        (5 == index) ? (DIGITAL6_DEFAULT_STATE) :
        (6 == index) ? (DIGITAL7_DEFAULT_STATE) :
        (7 == index) ? (DIGITAL8_DEFAULT_STATE) : (HIGH));
}

} // namespace build

namespace settings {
namespace options {

using espurna::settings::options::Enumeration;

PROGMEM_STRING(Low, "low");
PROGMEM_STRING(High, "high");
PROGMEM_STRING(Initial, "initial");

static constexpr std::array<Enumeration<digital::State>, 3> State PROGMEM {
    {{digital::State::Low, Low},
     {digital::State::High, High},
     {digital::State::Initial, Initial}}
};

PROGMEM_STRING(Input, "default");
PROGMEM_STRING(PullUp, "pull-up");
PROGMEM_STRING(PullDown, "pull-down");

static constexpr std::array<Enumeration<digital::Mode>, 3> Mode PROGMEM {
    {{digital::Mode::Input, Input},
     {digital::Mode::PullUp, PullUp},
     {digital::Mode::PullDown, PullDown}}
};

} // namespace options
} // namespace settings
} // namespace

} // namespace digital
} // namespace driver
} // namespace sensor

namespace settings {
namespace internal {

using namespace espurna::sensor::driver;

template<>
digital::Mode convert(const String& value) {
    return convert(digital::settings::options::Mode, value, digital::Mode::PullUp);
}

String serialize(digital::Mode value) {
    return serialize(digital::settings::options::Mode, value);
}

template<>
digital::State convert(const String& value) {
    return convert(digital::settings::options::State, value, digital::State::High);
}

String serialize(digital::State value) {
    return serialize(digital::settings::options::State, value);
}

} // namespace internal
} // namespace settings

namespace sensor {
namespace driver {
namespace digital {
namespace {

namespace settings {

STRING_VIEW_INLINE(Prefix, "digital");

namespace keys {

STRING_VIEW_INLINE(Pin, "digitalPin");
STRING_VIEW_INLINE(Mode, "digitalPinMode");
STRING_VIEW_INLINE(State, "digitalDefState");

} // namespace keys

unsigned char pin(size_t index) {
    return getSetting(keys::Pin, build::pin(index));
}

Mode mode(size_t index) {
    return getSetting(keys::Mode, build::mode(index));
}

State state(size_t index) {
    return getSetting(keys::State, build::state(index));
}

namespace query {

#define ID_VALUE(NAME, FUNC)\
String NAME (size_t id) {\
    return espurna::settings::internal::serialize(FUNC(id));\
}

ID_VALUE(pin, settings::pin)
ID_VALUE(mode, settings::mode)
ID_VALUE(state, settings::state)

#undef ID_VALUE

static constexpr espurna::settings::query::IndexedSetting IndexedSettings[] PROGMEM {
    {keys::Pin, pin},
    {keys::Mode, mode},
    {keys::State, state},
};

bool checkSamePrefix(StringView key) {
    return key.startsWith(Prefix);
}

espurna::settings::query::Result findFrom(StringView key) {
    return espurna::settings::query::findFrom(
        build::SensorsMax, IndexedSettings, key);
}

void setup() {
    settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findFrom,
    });
}

} // namespace query
} // namespace settings

struct Config {
    uint8_t pin;
    int pin_mode;
    State default_state;
};

class Sensor : public BaseSensor {
public:
    Sensor() = delete;
    explicit Sensor(Config config) :
        _pin(config.pin),
        _pin_mode(config.pin_mode),
        _default_state(config.default_state)
    {}

    // ---------------------------------------------------------------------
    // Sensor API
    // ---------------------------------------------------------------------

    unsigned char id() const override {
        return SENSOR_DIGITAL_ID;
    }

    unsigned char count() const override {
        return 1;
    }

    // Initialization method, must be idempotent
    void begin() override {
        if (!_ready) {
            pinMode(_pin, _pin_mode);

            switch (_default_state) {
            case State::Initial:
                _default = digitalRead(_pin);
                break;
            case State::High:
                _default = HIGH;
                break;
            case State::Low:
                _default = LOW;
                break;
            }
        }

        _ready = true;
    }

    // Descriptive name of the sensor
    String description() const override {
        char buffer[32];
        snprintf_P(buffer, sizeof(buffer),
            PSTR("Digital @ GPIO%hhu"), _pin);
        return String(buffer);
    }

    // Address of the sensor (it could be the GPIO or I2C address)
    String address(unsigned char) const override {
        return String(_pin, 10);
    }

    // Type for slot # index
    unsigned char type(unsigned char index) const override {
        if (index == 0) {
            return MAGNITUDE_DIGITAL;
        }

        return MAGNITUDE_NONE;
    }

    void pre() override {
        _current = digitalRead(_pin);
    }

    // Current value for slot # index
    double value(unsigned char index) override {
        if (index != 0) {
            return 0;
        }

        return (_current != _default) ? 1.0 : 0.0;
    }

private:
    unsigned char _pin;
    uint8_t _pin_mode;
    State _default_state;

    int _current { -1 };
    int _default { LOW };
};

class Init : public sensor::PreInit {
public:
    Init() = default;

    String description() const override {
        return STRING_VIEW("DigitalSensor").toString();
    }

    Result find_sensors() override {
        return _find_sensors();
    }

private:
    Result _with_error(int error) {
        return Result{
            .sensors = make_span(_sensors),
            .error = error,
        };
    }

    Result _find_sensors() {
        std::array<uint8_t, build::SensorsMax> pins;
        pins.fill(GPIO_NONE);

        int err = SENSOR_ERROR_OK;

        size_t index = 0;
        for (; index < build::SensorsMax; ++index) {
            const auto pin = settings::pin(index);
            if (pin == GPIO_NONE) {
                break;
            }

            if (!gpioLock(pin)) {
                err = SENSOR_ERROR_GPIO_USED;
                break;
            }

            pins[index] = pin;
        }

        size_t until = index;
        if (!until) {
            err = SENSOR_ERROR_CONFIG;
        }

        for (size_t index = 0; index < until; ++index) {
            if (err != SENSOR_ERROR_OK) {
                gpioUnlock(pins[index]);
            } else {
                _sensors.push_back(
                    new Sensor(Config{
                        .pin = pins[index],
                        .pin_mode = settings::mode(index),
                        .default_state = settings::state(index),
                    }));
            }
        }

        return _with_error(err);
    }

    std::vector<BaseSensor*> _sensors;
};

inline void load() {
    settings::query::setup();
    sensor::add_preinit(std::make_unique<Init>());
}

} // namespace
} // namespace digital
} // namespace driver
} // namespace sensor
} // namespace espurna

#endif // SENSOR_SUPPORT && DIGITAL_SUPPORT

