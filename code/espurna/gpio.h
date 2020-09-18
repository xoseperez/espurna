/*

GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"
#include "libs/BasePin.h"

#include <cstdint>

constexpr const size_t GpioPins = 17;

class GpioPin final : virtual public BasePin {
    public:

    // We need to explicitly call the constructor, because we need to set the const `pin`:
    // https://isocpp.org/wiki/faq/multiple-inheritance#virtual-inheritance-ctors
    explicit GpioPin(unsigned char pin_) :
        BasePin(pin_)
    {}

    void pinMode(int8_t mode) override {
        ::pinMode(this->pin, mode);
    }

    void digitalWrite(int8_t val) override {
        ::digitalWrite(this->pin, val);
    }

    int digitalRead() {
        return ::digitalRead(this->pin);
    }
};

bool gpioValid(unsigned char gpio);
bool gpioGetLock(unsigned char gpio);
bool gpioReleaseLock(unsigned char gpio);

void gpioSetup();
