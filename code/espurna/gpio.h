/*

GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"
#include "libs/BasePin.h"

#include <memory>

using BasePinPtr = std::unique_ptr<BasePin>;

enum class GpioType : int {
    None,
    Hardware,
    Mcp23s08
};

class GpioBase {
public:
    virtual const char* id() const = 0;
    virtual size_t pins() const = 0;
    virtual bool lock(unsigned char index) = 0;
    virtual bool lock(unsigned char index, bool value) = 0;
    virtual bool available(unsigned char index) = 0;
};

constexpr size_t GpioPins = 18;

bool gpioValid(GpioType type, unsigned char gpio);
bool gpioValid(unsigned char gpio);

bool gpioLock(GpioType type, unsigned char gpio);
bool gpioLock(unsigned char gpio);

bool gpioUnlock(GpioType type, unsigned char gpio);
bool gpioUnlock(unsigned char gpio);

bool gpioLocked(GpioType type, unsigned char gpio);
bool gpioLocked(unsigned char gpio);

BasePinPtr gpioRegister(GpioType type, unsigned char gpio);

void gpioSetup();
