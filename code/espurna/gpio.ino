/*

GPIO MODULE

Copyright (C) 2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

bool _gpio_locked[16] = {false};

bool gpioValid(unsigned char gpio) {
    if (0 <= gpio && gpio <= 5) return true;
    if (12 <= gpio && gpio <= 15) return true;
    return false;
}

bool gpioGetLock(unsigned char gpio) {
    if (gpioValid(gpio)) {
        if (!_gpio_locked[gpio]) {
            _gpio_locked[gpio] = true;
            DEBUG_MSG_P(PSTR("[GPIO] GPIO%d locked\n"), gpio);
            return true;
        }
    }
    DEBUG_MSG_P(PSTR("[GPIO] Failed getting lock for GPIO%d\n"), gpio);
    return false;
}

bool gpioReleaseLock(unsigned char gpio) {
    if (gpioValid(gpio)) {
        _gpio_locked[gpio] = false;
        DEBUG_MSG_P(PSTR("[GPIO] GPIO%d lock released\n"), gpio);
        return true;
    }
    DEBUG_MSG_P(PSTR("[GPIO] Failed releasing lock for GPIO%d\n"), gpio);
    return false;
}
