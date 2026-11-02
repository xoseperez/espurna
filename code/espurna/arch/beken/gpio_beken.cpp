/*

GPIO MODULE FOR BEKEN (LibreTiny)

*/

#if defined(LIBRETINY)

#include "gpio.h"
#include "settings.h"
#include "rtcmem.h"

#include <Arduino.h>

#include <algorithm>
#include <forward_list>

#if WEB_SUPPORT
#include "ws.h"
#endif

// --------------------------------------------------------------------------

namespace espurna {
namespace gpio {

namespace origin {
namespace internal {

std::forward_list<Origin> origins;

} // namespace internal

void add(Origin origin) {
    internal::origins.remove_if(
        [&](const Origin& other) {
            return other == origin;
        });
    internal::origins.emplace_front(origin);
}

} // namespace origin

class Hardware final : public GpioBase {
public:
    // LibreTiny pin numbers are GPIO numbers (P6 is 6)
    static constexpr size_t Pins = PINS_GPIO_MAX + 1;

    const char* id() const override {
        return "hardware";
    }

    size_t pins() const override {
        return Pins;
    }

    // Only the pins routed out by the module (ref. LibreTiny board variant)
    bool valid(unsigned char pin) const override {
        if (pin >= Pins) return false;
        return pinSupported(pinInfo(pin), PIN_GPIO);
    }

    bool validForOutput(unsigned char pin) const override {
        return valid(pin);
    }

    bool lock(unsigned char pin) const override {
        if (pin >= Pins) return false;
        return _locked[pin];
    }

    void lock(unsigned char pin, bool value) override {
        if (pin < Pins) {
            _locked[pin] = value;
        }
    }

    BasePinPtr pin(unsigned char pin) override;

private:
    bool _locked[Pins] { false };
};

class HardwarePin final : public BasePin {
public:
    explicit HardwarePin(unsigned char pin) : _pin(pin) {}

    const char* id() const override {
        return "hardware";
    }

    unsigned char pin() const override {
        return _pin;
    }

    // ArduinoCore-API has enums instead of plain ints, values are the same
    void pinMode(int8_t mode) override {
        ::pinMode(_pin, static_cast<PinMode>(mode));
    }

    void digitalWrite(int8_t val) override {
        ::digitalWrite(_pin, static_cast<PinStatus>(val));
    }

    int digitalRead() override {
        return ::digitalRead(_pin);
    }

private:
    unsigned char _pin;
};

BasePinPtr Hardware::pin(unsigned char pin) {
    return BasePinPtr(new HardwarePin(pin));
}

} // namespace gpio
} // namespace espurna

// --------------------------------------------------------------------------

String BasePin::description() const {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s @ GPIO%02hhu", id(), pin());
    return buffer;
}

GpioBase& hardwareGpio() {
    static espurna::gpio::Hardware gpio;
    return gpio;
}

GpioBase* gpioBase(GpioType type) {
    GpioBase* ptr { nullptr };

    switch (type) {
    case GpioType::Hardware:
        ptr = &hardwareGpio();
        break;
    case GpioType::Mcp23s08:
#if MCP23S08_SUPPORT
        // To be implemented or ported
        // ptr = &mcp23s08Gpio();
#endif
        break;
    case GpioType::None:
        break;
    }

    return ptr;
}

BasePinPtr gpioRegister(GpioBase& base, unsigned char gpio, espurna::SourceLocation source_location) {
    BasePinPtr result;
    if (gpioLock(base, gpio, source_location)) {
        result = base.pin(gpio);
    }
    return result;
}

BasePinPtr gpioRegister(unsigned char gpio, espurna::SourceLocation source_location) {
    return gpioRegister(hardwareGpio(), gpio, source_location);
}

#if WEB_SUPPORT
namespace espurna {
namespace gpio {
namespace web {

void onVisible(JsonObject& root) {
    JsonObject& config = root.createNestedObject(F("gpioConfig"));

    constexpr GpioType known_types[] {
        GpioType::Hardware,
#if MCP23S08_SUPPORT
        GpioType::Mcp23s08,
#endif
    };

    JsonArray& types = config.createNestedArray(F("types"));

    for (auto& type : known_types) {
        auto* base = gpioBase(type);
        if (base) {
            JsonArray& entry = types.createNestedArray();
            entry.add(base->id());
            entry.add(static_cast<int>(type));

            JsonArray& pins = config.createNestedArray(base->id());
            for (size_t pin = 0; pin < base->pins(); ++pin) {
                if (base->valid(pin)) {
                    pins.add(pin);
                }
            }
        }
    }

    JsonObject& info = root.createNestedObject(F("gpioInfo"));

    JsonArray& locks = info.createNestedArray(F("failed-locks"));
    for (auto& origin : origin::internal::origins) {
        if (!origin.result) {
            JsonArray& entry = locks.createNestedArray();
            entry.add(origin.pin);
            entry.add(origin.location.file);
            entry.add(origin.location.func);
            entry.add(origin.location.line);
        }
    }
}

void setup() {
    wsRegister()
        .onVisible(onVisible);
}

} // namespace web
} // namespace gpio
} // namespace espurna
#endif

void gpioSetup() {
#if WEB_SUPPORT
    espurna::gpio::web::setup();
#endif
}

// Nothing survives the reset (ref. rtcmem_beken.cpp)
void hardwareGpioIgnore(unsigned char) {
}

void gpioLockOrigin(espurna::gpio::Origin origin) {
    espurna::gpio::origin::add(std::move(origin));
}

#endif // LIBRETINY
