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
    virtual bool lock(unsigned char index) const = 0;
    virtual bool lock(unsigned char index, bool value) = 0;
    virtual bool valid(unsigned char index) const = 0;
    virtual BasePinPtr pin(unsigned char index) = 0;
};

GpioBase& hardwareGpio();
GpioBase* gpioBase(GpioType);

BasePinPtr gpioRegister(GpioBase& base, unsigned char gpio);
BasePinPtr gpioRegister(unsigned char gpio);

void gpioSetup();

inline bool gpioPins(const GpioBase& base) {
    return base.pins();
}

inline bool gpioPins() {
    return gpioPins(hardwareGpio());
}

inline bool gpioValid(const GpioBase& base, unsigned char gpio) {
    return base.valid(gpio);
}

inline bool gpioValid(unsigned char gpio) {
    return gpioValid(hardwareGpio(), gpio);
}

inline bool gpioLock(GpioBase& base, unsigned char gpio) {
    return base.lock(gpio, true);
}

inline bool gpioLock(unsigned char gpio) {
    return gpioLock(hardwareGpio(), gpio);
}

inline bool gpioUnlock(GpioBase& base, unsigned char gpio) {
    return base.lock(gpio, false);
}

inline bool gpioUnlock(unsigned char gpio) {
    return gpioUnlock(hardwareGpio(), gpio);
}

inline bool gpioLocked(const GpioBase& base, unsigned char gpio) {
    return base.lock(gpio);
}

inline bool gpioLocked(unsigned char gpio) {
    return gpioLocked(hardwareGpio(), gpio);
}
