/*

GPIO MODULE FOR ESP32

*/

#include "gpio.h"
#include "settings.h"

#include <Arduino.h>

#include <algorithm>
#include <vector>

// --- BasePin implementation ---
String BasePin::description() const {
    return String("GPIO") + String(pin());
}

namespace {

// Concrete implementation of BasePin for Hardware pins
class HardwarePin final : public BasePin {
public:
    explicit HardwarePin(unsigned char pin) : _pin(pin) {}
    
    const char* id() const override { return "hardware"; }
    unsigned char pin() const override { return _pin; }
    
    void pinMode(int8_t mode) override {
        if (_pin != GPIO_NONE) ::pinMode(_pin, mode);
    }
    
    void digitalWrite(int8_t val) override {
        if (_pin != GPIO_NONE) ::digitalWrite(_pin, val);
    }
    
    int digitalRead() override {
        return (_pin != GPIO_NONE) ? ::digitalRead(_pin) : LOW;
    }

private:
    unsigned char _pin;
};

class HardwareGpio : public GpioBase {
public:
    const char* id() const override { return "hardware"; }
    size_t pins() const override { return 40; }
    
    bool lock(unsigned char index) const override {
        return (index < 40) ? _locked[index] : false;
    }
    
    void lock(unsigned char index, bool value) override {
        if (index < 40) _locked[index] = value;
    }
    
    bool valid(unsigned char index) const override {
        return index < pins();
    }
    
    BasePinPtr pin(unsigned char index) override {
        return std::unique_ptr<BasePin>(new HardwarePin(index));
    }

private:
    bool _locked[40] { false };
};

HardwareGpio _hardware_gpio;

} // namespace

GpioBase* gpioBase(GpioType type) {
    if (type == GpioType::Hardware) return &_hardware_gpio;
    return nullptr;
}

GpioBase& hardwareGpio() {
    return _hardware_gpio;
}

void hardwareGpioIgnore(unsigned char) {}

void gpioSetup() {}

void gpioLockOrigin(espurna::gpio::Origin) {}

BasePinPtr gpioRegister(GpioBase& base, unsigned char gpio, espurna::SourceLocation source_location) {
    base.lock(gpio, true);
    return base.pin(gpio);
}

BasePinPtr gpioRegister(unsigned char gpio, espurna::SourceLocation source_location) {
    return gpioRegister(hardwareGpio(), gpio, source_location);
}

namespace espurna {
namespace settings {
namespace internal {

String serialize_gpio_type(GpioType type) {
    if (type == GpioType::Hardware) return "hardware";
    if (type == GpioType::Mcp23s08) return "mcp23s08";
    return "none";
}

GpioType convert_gpio_type(const String& value) {
    if (value.equalsIgnoreCase("hardware")) return (GpioType)1;
    if (value.equalsIgnoreCase("mcp23s08")) return (GpioType)2;
    return (GpioType)0;
}

} // namespace internal
} // namespace settings
} // namespace espurna
