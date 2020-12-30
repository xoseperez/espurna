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

}
}

namespace {

class GpioHardware : public GpioBase {
public:
    constexpr static size_t Pins { 17ul };
    using Mask = std::bitset<GpioPins>;
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

    const char* id() const {
        return "hardware";
    }

    size_t pins() const {
        return Pins;
    }

    bool lock(unsigned char index) override {
        return _lock[index];
    }

    bool lock(unsigned char index, bool value) override {
        bool current = _lock[index];
        _lock.set(index, value);
        return (value != current);
    }

    bool available(unsigned char index) override {
        switch (index) {
        case 0 ... 5:
        case 9:
        case 10:
            return _esp8285;
        case 12 ... 17:
            return true;
        }

        return false;
    }

private:
    bool _esp8285 { false };
    Mask _lock;
};

std::pair<GpioType, GpioBase*> _gpio_support[] {
    {GpioType::Hardware, new GpioHardware()}
#if MCP23S08_SUPPORT
    ,{GpioType::Mcp23s08, new GpioMcp23s08()}
#endif
};

GpioBase* _gpioFind(GpioType type, unsigned char gpio) {
    for (auto& pair : _gpio_support) {
        if ((pair.first == type)
            && (gpio < pair.second->pins())
            && (pair.second->available(gpio))) {
            return pair.second;
        }
    }

    return nullptr;
}

} // namespace

bool gpioValid(GpioType type, unsigned char gpio) {
    auto* ptr = _gpioFind(type, gpio);
    if (ptr) {
        return ptr->available(gpio);
    }

    return false;
}

bool gpioValid(unsigned char gpio) {
    return gpioValid(GpioType::Hardware, gpio);
}

bool _gpioLock(GpioType type, unsigned char gpio, bool value) {
    auto* ptr = _gpioFind(type, gpio);
    if (ptr) {
        return ptr->lock(gpio, value);
    }

    return false;
}

bool gpioLock(GpioType type, unsigned char gpio) {
    return _gpioLock(type, gpio, true);
}

bool gpioLock(unsigned char gpio) {
    return _gpioLock(GpioType::Hardware, gpio, true);
}

bool gpioUnlock(GpioType type, unsigned char gpio) {
    return _gpioLock(type, gpio, false);
}

bool gpioUnlock(unsigned char gpio) {
    return _gpioLock(GpioType::Hardware, gpio, false);
}

BasePinPtr gpioRegister(GpioType type, unsigned char gpio) {
    BasePinPtr result;

    auto* ptr = _gpioFind(type, gpio);
    if (ptr && ptr->lock(gpio, true)) {
        switch (type) {
        case GpioType::None:
            break;
        case GpioType::Hardware:
            result = std::make_unique<GpioPin>(gpio);
            break;
        case GpioType::Mcp23s08:
#if MCP23S08_SUPPORT
            result = std::make_unique<Mcp23s08Pin>(gpio);
#endif
            break;
        }
    }

    return result;
}

void gpioSetup() {
}
