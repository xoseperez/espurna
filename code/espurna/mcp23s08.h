/*

MCP23S08 MODULE

Copyright (C) 2020 by Eddi De Pieri <eddi at depieri dot com>

Adapted from https://github.com/kmpelectronics/Arduino
Copyright (C) 2016 Plamen Kovandjiev <p.kovandiev@kmpelectronics.eu> & Dimitar Antonov <d.antonov@kmpelectronics.eu>

(ref. https://github.com/kmpelectronics/Arduino/blob/master/ProDinoWiFiEsp/src/PRODINoESP8266/src/KMPDinoWiFiESP.cpp)

*/

#pragma once

#include "espurna.h"

constexpr size_t McpGpioPins = 8;

void MCP23S08Setup();

uint8_t MCP23S08ReadRegister(uint8_t address);
void MCP23S08WriteRegister(uint8_t address, uint8_t data);

void MCP23S08SetDirection(uint8_t pinNumber, uint8_t mode);
void MCP23S08SetPin(uint8_t pinNumber, bool state);
bool MCP23S08GetPin(uint8_t pinNumber);

bool mcpGpioValid(unsigned char gpio);
GpioBase& mcp23s08Gpio();
