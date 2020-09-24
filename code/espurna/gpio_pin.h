/*

Part of the GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "gpio.h"

#include <cstdint>

class GpioPin final : public BasePin {
    public:

    explicit GpioPin(unsigned char pin_) :
        BasePin(pin_)
    {}

    void pinMode(int8_t mode) override {
        ::pinMode(this->pin, mode);
    }

    void digitalWrite(int8_t val) override {
        ::digitalWrite(this->pin, val);
    }

    String description() const override {
        static String desc(String(F("GpioPin @ GPIO")) + static_cast<int>(pin));
        return desc;
    }

    int digitalRead() {
        return ::digitalRead(this->pin);
    }
};

