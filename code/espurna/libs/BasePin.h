/*

Part of BUTTON module

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstdint>

// base interface for generic pin handler. 
class BasePin {
    public:
        BasePin(unsigned char pin) :
            pin(pin)
        {}

        virtual void pinMode(int8_t mode) = 0;
        virtual void digitalWrite(int8_t val) = 0;
        virtual int digitalRead() = 0;

        const unsigned char pin;
};
