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
        BasePin(pin)
    {}

    void pinMode(int8_t mode) override {
        ::MCP23S08SetDirection(this->pin, mode);
    }

    void digitalWrite(int8_t val) override {
        ::MCP23S08SetPin(this->pin, val);
    }

    int digitalRead() override {
        return ::MCP23S08GetPin(this->pin);
    }

    String description() const override {
        static String desc(String(F("McpGpioPin @ GPIO")) + static_cast<int>(pin));
        return desc;
    }
};

class GpioMcp23s08 : public GpioBase {
public:
    constexpr static size_t Pins { 8ul };

    using Pin = McpGpioPin;
    using Mask = std::bitset<Pins>;

    const char* id() const {
        return "mcp23s08";
    }

    size_t pins() const {
        return Pins;
    }

    bool lock(unsigned char index) {
        return _lock[index];
    }

    bool lock(unsigned char index, bool value) {
        bool current = _lock[index];
        _lock.set(index, value);
        return (value != current);
    }

    bool available(unsigned char index) {
        return (index < Pins);
    }

private:
    Mask _lock;
};
