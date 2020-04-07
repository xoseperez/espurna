/*

GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "gpio.h"

#include <bitset>

// We need to explicitly call the constructor, because we need to set the const `pin`:
// https://isocpp.org/wiki/faq/multiple-inheritance#virtual-inheritance-ctors
GpioPin::GpioPin(unsigned char pin) :
    BasePin(pin)
{}

inline void GpioPin::pinMode(int8_t mode) {
    ::pinMode(this->pin, mode);
}

inline void GpioPin::digitalWrite(int8_t val) {
    ::digitalWrite(this->pin, val);
}

inline int GpioPin::digitalRead() {
    return ::digitalRead(this->pin);
}

// --------------------------------------------------------------------------

std::bitset<GpioPins> _gpio_locked;
std::bitset<GpioPins> _gpio_available;

bool gpioValid(unsigned char gpio) {
    if (gpio >= GpioPins) return false;

    return _gpio_available.test(gpio);
}

bool gpioGetLock(unsigned char gpio) {
    if (gpioValid(gpio)) {
        if (!_gpio_locked.test(gpio)) {
            _gpio_locked.set(gpio);
            DEBUG_MSG_P(PSTR("[GPIO] GPIO%u locked\n"), gpio);
            return true;
        }
    }
    DEBUG_MSG_P(PSTR("[GPIO] Failed getting lock for GPIO%u\n"), gpio);
    return false;
}

bool gpioReleaseLock(unsigned char gpio) {
    if (gpioValid(gpio)) {
        _gpio_locked.reset(gpio);
        DEBUG_MSG_P(PSTR("[GPIO] GPIO%u lock released\n"), gpio);
        return true;
    }
    DEBUG_MSG_P(PSTR("[GPIO] Failed releasing lock for GPIO%u\n"), gpio);
    return false;
}

void gpioSetup() {

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

    const bool esp8285 = (
        (efuse_blocks[0] & (1 << 4))
        || (efuse_blocks[2] & (1 << 16))
    );

    // TODO: GPIO16 is only for basic I/O, gpioGetLock before attachInterrupt should check for that
    for (unsigned char pin=0; pin < GpioPins; ++pin) {
        if (pin <= 5) _gpio_available.set(pin);
        if (((pin == 9) || (pin == 10)) && (esp8285)) _gpio_available.set(pin);
        if (12 <= pin && pin <= 16) _gpio_available.set(pin);
    }

}
