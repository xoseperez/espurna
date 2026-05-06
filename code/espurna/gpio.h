/*

Part of the GPIO module

Copyright (C) 2021-2023 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <memory>
#include <vector>

#include "types.h"
#include "libs/BasePin.h"

// -----------------------------------------------------------------------------

struct GpioBase {
    virtual ~GpioBase() = default;

    virtual const char* id() const = 0;
    virtual size_t pins() const = 0;

    virtual bool lock(unsigned char index) const = 0;
    virtual void lock(unsigned char index, bool value) = 0;

    virtual bool valid(unsigned char index) const = 0;
    virtual BasePinPtr pin(unsigned char index) = 0;
};

// -----------------------------------------------------------------------------

enum class GpioType : int {
    None = 0,
    Hardware = 1,
    Mcp23s08 = 2
};

GpioBase* gpioBase(GpioType type);
GpioBase& hardwareGpio();

void hardwareGpioIgnore(unsigned char gpio);
void gpioSetup();

namespace espurna {
namespace gpio {

enum class Origin : int {
    None,
    Relay,
    Button,
    Led,
    Encoder,
    OneWire,
    Sensor,
    Uart,
    I2c,
    Spi
};

void lockOrigin(Origin);

} // namespace gpio

BasePinPtr gpioRegister(GpioBase& base, unsigned char gpio,
        SourceLocation source_location = make_source_location());
BasePinPtr gpioRegister(unsigned char gpio,
        SourceLocation source_location = make_source_location());

inline bool gpioLock(GpioBase& base, unsigned char gpio, bool value,
        SourceLocation source_location = make_source_location())
{
    if (base.valid(gpio)) {
        base.lock(gpio, value);
        return true;
    }
    return false;
}

inline bool gpioLock(GpioBase& base, unsigned char gpio,
        SourceLocation source_location = make_source_location())
{
    return gpioLock(base, gpio, true, source_location);
}

inline bool gpioLock(BasePinPtr& pin, bool value,
        SourceLocation source_location = make_source_location())
{
    if (pin) {
        return gpioLock(hardwareGpio(), pin->pin(), value, source_location);
    }
    return false;
}

inline bool gpioLock(unsigned char gpio,
        SourceLocation source_location = make_source_location())
{
    return gpioLock(hardwareGpio(), gpio, true, source_location);
}

inline bool gpioUnlock(unsigned char gpio,
        SourceLocation source_location = make_source_location())
{
    return gpioLock(hardwareGpio(), gpio, false, source_location);
}

#if defined(ESP8266)
inline bool gpioUnlock(unsigned char gpio,
        SourceLocation location) {
    return gpioUnlock(hardwareGpio(), gpio, location);
}
#elif defined(ESP32)
bool gpioUnlock(unsigned char gpio,
        SourceLocation location);
#endif

} // namespace espurna

inline bool gpioLocked(const GpioBase& base, unsigned char gpio) {
    if (base.valid(gpio)) {
        return base.lock(gpio);
    }
    return false;
}

inline bool gpioLocked(unsigned char gpio) {
    return gpioLocked(hardwareGpio(), gpio);
}

void gpioLockOrigin(espurna::gpio::Origin origin);

// -----------------------------------------------------------------------------
// Settings conversion for GpioType
// -----------------------------------------------------------------------------

namespace espurna {
namespace settings {
namespace internal {

template <typename T> String serialize(T value);
template <typename T> T convert(const String& value);

template <> inline String serialize(GpioType type) {
    if (type == GpioType::Hardware) return "hardware";
    if (type == GpioType::Mcp23s08) return "mcp23s08";
    return "none";
}

template <> inline GpioType convert(const String& value) {
    if (value.equalsIgnoreCase("hardware")) return GpioType::Hardware;
    if (value.equalsIgnoreCase("mcp23s08")) return GpioType::Mcp23s08;
    return GpioType::None;
}

} // namespace internal
} // namespace settings
} // namespace espurna
