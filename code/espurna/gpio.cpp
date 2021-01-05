/*

GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

// --------------------------------------------------------------------------

#include <bitset>
#include <utility>

#include "gpio_pin.h"
#include "mcp23s08_pin.h"

namespace settings {
namespace internal {

template <>
GpioType convert(const String& value) {
    auto type = static_cast<GpioType>(value.toInt());
    switch (type) {
    case GpioType::Hardware:
    case GpioType::Mcp23s08:
        return type;
    case GpioType::None:
        break;
    }

    return GpioType::None;
}

} // namespace internal
} // namespace settings

namespace {

class GpioHardware : public GpioBase {
public:
    constexpr static size_t Pins { 17ul };

    using Mask = std::bitset<Pins>;
    using Pin = GpioPin;

    GpioHardware() {
        // https://github.com/espressif/esptool/blob/f04d34bcab29ace798d2d3800ba87020cccbbfdd/esptool.py#L1060-L1070
        // "One or the other efuse bit is set for ESP8285"
        // https://github.com/espressif/ESP8266_RTOS_SDK/blob/3c055779e9793e5f082afff63a011d6615e73639/components/esp8266/include/esp8266/efuse_register.h#L20-L21
        // "define EFUSE_IS_ESP8285    (1 << 4)"
        const uint32_t efuse_blocks[4] {
            READ_PERI_REG(0x3ff00050),
            READ_PERI_REG(0x3ff00054),
            READ_PERI_REG(0x3ff00058),
            READ_PERI_REG(0x3ff0005c)
        };

        _esp8285 = (
            (efuse_blocks[0] & (1 << 4))
            || (efuse_blocks[2] & (1 << 16))
        );
    }

    const char* id() const override {
        return "hardware";
    }

    size_t pins() const override {
        return Pins;
    }

    bool lock(unsigned char index) const override {
        return _lock[index];
    }

    void lock(unsigned char index, bool value) override {
        _lock.set(index, value);
    }

    bool valid(unsigned char index) const override {
        switch (index) {
        case 0 ... 5:
            return true;
        case 9:
        case 10:
            return _esp8285;
        case 12 ... 16:
            return true;
        }

        return false;
    }

    BasePinPtr pin(unsigned char index) {
        return std::make_unique<GpioPin>(index);
    }

private:
    bool _esp8285 { false };
    Mask _lock;
};

} // namespace

String BasePin::description() const {
    char buffer[64];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s @ GPIO%02u"), id(), pin());
    return buffer;
}

BasePin::~BasePin() {
}

GpioBase& hardwareGpio() {
    static GpioHardware gpio;
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
        ptr = &mcp23s08Gpio();
#endif
        break;
    case GpioType::None:
        break;
    }

    return ptr;
}

BasePinPtr gpioRegister(GpioBase& base, unsigned char gpio) {
    BasePinPtr result;

    if (gpioLock(base, gpio)) {
        result = std::move(base.pin(gpio));
    }

    return result;
}

BasePinPtr gpioRegister(unsigned char gpio) {
    return gpioRegister(hardwareGpio(), gpio);
}

void gpioSetup() {
}
