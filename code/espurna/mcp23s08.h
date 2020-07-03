/*

MCP23S08 MODULE

Copyright (C) 2020-2020 by Eddi De Pieri <eddi at depieri dot com>
Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2016-2017 Plamen Kovandjiev <p.kovandiev@kmpelectronics.eu> & Dimitar Antonov <d.antonov@kmpelectronics.eu>

*/

#pragma once

#ifndef MCP23S08_H
#define MCP23S08_H

#include "espurna.h"
#include "libs/BasePin.h"

#if MCP23S08_SUPPORT

#include <SPI.h>

constexpr const size_t McpGpioPins = MCP23S08_OPTOIN_COUNT;

// real hardware pin
class McpGpioPin final : virtual public BasePin {
    public:
        McpGpioPin(unsigned char pin);

        void pinMode(int8_t mode);
        void digitalWrite(int8_t val);
        int digitalRead();
};

// Inputs and outputs count.
#define MCP23S08_OPTOIN_COUNT 4

void MCP23S08Setup();
void MCP23S08InitGPIO(); 
void MCP23S08SetDirection(uint8_t pinNumber, uint8_t mode);
uint8_t MCP23S08ReadRegister(uint8_t address);
void MCP23S08WriteRegister(uint8_t address, uint8_t data);
void MCP23S08SetPin(uint8_t pinNumber, bool state);
bool MCP23S08GetPin(uint8_t pinNumber);
void MCP23S08SetRelayState(uint8_t relayNumber, bool state);
bool MCP23S08GetOptoInState(uint8_t optoInNumber);

bool mcpGpioValid(unsigned char gpio);

#endif // MCP23S08_SUPPORT == 1

#endif