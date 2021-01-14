/*

Part of the GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "gpio.h"

#include <cstdint>

class GpioPin final : public BasePin {
public:
    explicit GpioPin(unsigned char pin) :
        _pin(pin)
    {}

    // ESP8266 does not have INPUT_PULLDOWN definition, and instead
    // has a GPIO16-specific INPUT_PULLDOWN_16:
    // - https://github.com/esp8266/Arduino/issues/478
    // - https://github.com/esp8266/Arduino/commit/1b3581d55ebf0f8c91e081f9af4cf7433d492ec9
    void pinMode(int8_t mode) override {
#ifdef ESP8266
        if ((INPUT_PULLDOWN == mode) && (_pin == 16)) {
            mode = INPUT_PULLDOWN_16;
        }
#endif
        ::pinMode(_pin, mode);
    }

    void digitalWrite(int8_t val) override {
        ::digitalWrite(_pin, val);
    }

    int digitalRead() override {
        return ::digitalRead(_pin);
    }

    unsigned char pin() const override {
        return _pin;
    }

    const char* id() const override {
        return "GpioPin";
    }

private:
    unsigned char _pin { GPIO_NONE };
};

