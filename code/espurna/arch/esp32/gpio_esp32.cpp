/*

GPIO MODULE FOR ESP32

*/

#include "gpio.h"
#include "settings.h"
#include "rtcmem.h"

#include <Arduino.h>

#include <algorithm>
#include <vector>
#include <map>

#if WEB_SUPPORT
#include "ws.h"
#endif

// --------------------------------------------------------------------------

namespace espurna {
namespace gpio {

namespace origin {

// We want to keep track of who locked which pin. 
static std::vector<Origin> _origins;

void add(const Origin& origin) {
    _origins.push_back(origin);
}

const std::vector<Origin>& all() {
    return _origins;
}

} // namespace origin

namespace {

struct Lock {
    GpioBase* base;
    unsigned char pin;
    SourceLocation location;
};

static std::vector<Lock> _locks;

} // namespace

bool lock(GpioBase& base, unsigned char pin, const SourceLocation& location) {
    for (const auto& lock : _locks) {
        if ((lock.base == &base) && (lock.pin == pin)) {
            return false;
        }
    }

    _locks.push_back(Lock{&base, pin, location});
    base.lock(pin, true);
    return true;
}

void unlock(GpioBase& base, unsigned char pin) {
    auto it = std::remove_if(_locks.begin(), _locks.end(), [&](const Lock& lock) {
        return (lock.base == &base) && (lock.pin == pin);
    });
    if (it != _locks.end()) {
        _locks.erase(it, _locks.end());
        base.lock(pin, false);
    }
}

bool locked(GpioBase& base, unsigned char pin) {
    for (const auto& lock : _locks) {
        if ((lock.base == &base) && (lock.pin == pin)) {
            return true;
        }
    }
    return false;
}

// --------------------------------------------------------------------------

class Hardware final : public GpioBase {
public:
    static constexpr size_t Pins = 40;

    const char* id() const override {
        return "hardware";
    }

    size_t pins() const override {
        return Pins;
    }

    bool valid(unsigned char pin) const override {
        return pin < Pins;
    }

    bool lock(unsigned char pin) const override {
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

    void pinMode(int8_t mode) override {
        ::pinMode(_pin, mode);
    }

    void digitalWrite(int8_t val) override {
        ::digitalWrite(_pin, val);
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
    if (espurna::gpio::lock(base, gpio, source_location)) {
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
    for (const auto& origin : origin::all()) {
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

void hardwareGpioIgnore(unsigned char gpio) {
    Rtcmem->gpio_ignore |= (1ULL << gpio);
}

void gpioLockOrigin(espurna::gpio::Origin origin) {
    espurna::gpio::origin::add(origin);
}

namespace espurna {
namespace settings {
namespace internal {

String serialize_gpio_type(GpioType type) {
    switch (type) {
        case GpioType::Hardware: return "hardware";
        case GpioType::Mcp23s08: return "mcp23s08";
        default: return "none";
    }
}

GpioType convert_gpio_type(const String& value) {
    if (value.equalsIgnoreCase("hardware")) return GpioType::Hardware;
    if (value.equalsIgnoreCase("mcp23s08")) return GpioType::Mcp23s08;
    return GpioType::None;
}

} // namespace internal
} // namespace settings
} // namespace espurna
