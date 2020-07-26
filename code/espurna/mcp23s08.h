/*

MCP23S08 MODULE

Copyright (C) 2020 by Eddi De Pieri <eddi at depieri dot com>

Adapted from https://github.com/kmpelectronics/Arduino
Copyright (C) 2016 Plamen Kovandjiev <p.kovandiev@kmpelectronics.eu> & Dimitar Antonov <d.antonov@kmpelectronics.eu>

(ref. https://github.com/kmpelectronics/Arduino/blob/master/ProDinoWiFiEsp/src/PRODINoESP8266/src/KMPDinoWiFiESP.cpp)

*/

#pragma once

#ifndef MCP23S08_H
#define MCP23S08_H

#include "espurna.h"
#include "libs/BasePin.h"

#if MCP23S08_SUPPORT

constexpr size_t McpGpioPins = 8;

// real hardware pin
class McpGpioPin final : public BasePin {
    public:
        explicit McpGpioPin(unsigned char pin);

        void pinMode(int8_t mode);
        void digitalWrite(int8_t val);
        int digitalRead();
};

void MCP23S08Setup();

uint8_t MCP23S08ReadRegister(uint8_t address);
void MCP23S08WriteRegister(uint8_t address, uint8_t data);

void MCP23S08SetDirection(uint8_t pinNumber, uint8_t mode);
void MCP23S08SetPin(uint8_t pinNumber, bool state);
bool MCP23S08GetPin(uint8_t pinNumber);

bool mcpGpioValid(unsigned char gpio);

#endif // MCP23S08_SUPPORT == 1

#endif
