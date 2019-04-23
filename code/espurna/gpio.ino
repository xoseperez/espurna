/*

GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

unsigned int _gpio_locked = 0;

bool gpioValid(unsigned char gpio) {
    if (gpio <= 5) return true;
    if (12 <= gpio && gpio <= 15) return true;
    return false;
}

bool gpioGetLock(unsigned char gpio) {
    if (gpioValid(gpio)) {
        unsigned int mask = 1 << gpio;
        if ((_gpio_locked & mask) == 0) {
            _gpio_locked |= mask;
            DEBUG_MSG_P(PSTR("[GPIO] GPIO%u locked\n"), gpio);
            return true;
        }
    }
    DEBUG_MSG_P(PSTR("[GPIO] Failed getting lock for GPIO%u\n"), gpio);
    return false;
}

bool gpioReleaseLock(unsigned char gpio) {
    if (gpioValid(gpio)) {
        unsigned int mask = 1 << gpio;
        _gpio_locked &= ~mask;
        DEBUG_MSG_P(PSTR("[GPIO] GPIO%u lock released\n"), gpio);
        return true;
    }
    DEBUG_MSG_P(PSTR("[GPIO] Failed releasing lock for GPIO%u\n"), gpio);
    return false;
}
