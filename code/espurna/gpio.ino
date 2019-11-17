/*

GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <bitset>

constexpr const size_t GPIO_PINS = 16;

std::bitset<GPIO_PINS> _gpio_locked;
std::bitset<GPIO_PINS> _gpio_available;

bool gpioValid(unsigned char gpio) {
    if (gpio >= GPIO_PINS) return false;

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

    for (unsigned char pin=0; pin < GPIO_PINS; ++pin) {
        if (pin <= 5) _gpio_available.set(pin);
        if (((pin == 9) || (pin == 10)) && (esp8285)) _gpio_available.set(pin);
        if (12 <= pin && pin <= 15) _gpio_available.set(pin);
    }

}
