/*

GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "settings.h"
#include "libs/BasePin.h"

#include <cstddef>

enum class GpioType : int {
    None,
    Hardware,
    Mcp23s08
};

namespace espurna {
namespace gpio {

struct Origin {
    const char* base;
    uint8_t pin;
    bool lock;
    SourceLocation location;
};

struct Mode {
    int8_t value;
};

Mode pin_mode(uint8_t);

} // namespace gpio

namespace settings {
namespace internal {

String serialize(GpioType);

} // namespace internal
} // namespace settings
} // namespace espurna

class GpioBase {
public:
    virtual const char* id() const = 0;
    virtual size_t pins() const = 0;
    virtual bool lock(unsigned char index) const = 0;
    virtual void lock(unsigned char index, bool value) = 0;
    virtual bool valid(unsigned char index) const = 0;
    virtual BasePinPtr pin(unsigned char index) = 0;
};

GpioBase* gpioBase(GpioType);

GpioBase& hardwareGpio();
void hardwareGpioIgnore(unsigned char gpio);

void gpioLockOrigin(espurna::gpio::Origin);

void gpioSetup();

inline size_t gpioPins(const GpioBase& base) {
    return base.pins();
}

inline size_t gpioPins() {
    return gpioPins(hardwareGpio());
}

inline bool gpioValid(const GpioBase& base, unsigned char gpio) {
    return base.valid(gpio);
}

inline bool gpioValid(unsigned char gpio) {
    return gpioValid(hardwareGpio(), gpio);
}

inline bool gpioLock(GpioBase& base, unsigned char pin, bool value,
        espurna::SourceLocation source_location = espurna::make_source_location())
{
    if (base.valid(pin)) {
        gpioLockOrigin(espurna::gpio::Origin{
            .base = base.id(),
            .pin = pin,
            .lock = value,
            .location = trim_source_location(source_location)
        });

        bool old = base.lock(pin);
        base.lock(pin, value);
        return (value != old);
    }

    return false;
}

inline bool gpioLock(GpioBase& base, unsigned char gpio,
        espurna::SourceLocation source_location = espurna::make_source_location())
{
    return gpioLock(base, gpio, true, source_location);
}

inline bool gpioLock(unsigned char gpio,
        espurna::SourceLocation source_location = espurna::make_source_location())
{
    return gpioLock(hardwareGpio(), gpio, source_location);
}

inline bool gpioUnlock(GpioBase& base, unsigned char gpio,
        espurna::SourceLocation source_location = espurna::make_source_location())
{
    return gpioLock(base, gpio, false, source_location);
}

inline bool gpioUnlock(unsigned char gpio,
        espurna::SourceLocation source_location = espurna::make_source_location())
{
    return gpioUnlock(hardwareGpio(), gpio, source_location);
}

inline bool gpioLocked(const GpioBase& base, unsigned char gpio) {
    if (base.valid(gpio)) {
        return base.lock(gpio);
    }
    return false;
}

inline bool gpioLocked(unsigned char gpio) {
    return gpioLocked(hardwareGpio(), gpio);
}

BasePinPtr gpioRegister(GpioBase& base, unsigned char gpio,
        espurna::SourceLocation source_location = espurna::make_source_location());
BasePinPtr gpioRegister(unsigned char gpio,
        espurna::SourceLocation source_location = espurna::make_source_location());
