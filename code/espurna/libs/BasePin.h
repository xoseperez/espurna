/*

Part of BUTTON module

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <cstdint>
#include "../config/types.h"

// base interface for generic pin handler. 
struct BasePin {
    explicit BasePin(unsigned char pin) :
        pin(pin)
    {}

    virtual ~BasePin() {
    }

    virtual operator bool() {
        return GPIO_NONE != pin;
    }

    virtual void pinMode(int8_t mode) = 0;
    virtual void digitalWrite(int8_t val) = 0;
    virtual int digitalRead() = 0;

    virtual String description() const {
        static String desc(String(F("BasePin @ GPIO")) + static_cast<int>(pin));
        return desc;
    }

    const unsigned char pin { GPIO_NONE };
};
