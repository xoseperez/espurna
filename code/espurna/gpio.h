/*

GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <cstdint>

#include "espurna.h"
#include "libs/BasePin.h"

constexpr const size_t GpioPins = 17;

// real hardware pin
class GpioPin final : virtual public BasePin {
    public:
        GpioPin(unsigned char pin);

        void pinMode(int8_t mode);
        void digitalWrite(int8_t val);
        int digitalRead();
};


bool gpioValid(unsigned char gpio);
bool gpioGetLock(unsigned char gpio);
bool gpioReleaseLock(unsigned char gpio);

void gpioSetup();
