/*

Part of the MCP23S08 MODULE

Copyright (C) 2020 by Eddi De Pieri <eddi at depieri dot com>

Adapted from https://github.com/kmpelectronics/Arduino
Copyright (C) 2016 Plamen Kovandjiev <p.kovandiev@kmpelectronics.eu> & Dimitar Antonov <d.antonov@kmpelectronics.eu>

(ref. https://github.com/kmpelectronics/Arduino/blob/master/ProDinoWiFiEsp/src/PRODINoESP8266/src/KMPDinoWiFiESP.cpp)

*/

#pragma once

#include "libs/BasePin.h"
#include "mcp23s08.h"

#include <bitset>

class McpGpioPin final : public BasePin {
public:
    explicit McpGpioPin(unsigned char pin) :
        _pin(pin)
    {}

    void pinMode(int8_t mode) override {
        ::MCP23S08SetDirection(_pin, mode);
    }

    void digitalWrite(int8_t val) override {
        ::MCP23S08SetPin(_pin, val);
    }

    int digitalRead() override {
        return ::MCP23S08GetPin(_pin);
    }

    unsigned char pin() const override {
        return _pin;
    }

    const char* id() const override {
        return "McpGpioPin";
    }

private:
    unsigned char _pin { GPIO_NONE };
};
