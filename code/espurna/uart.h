/*

UART MODULE

Copyright (C) 2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <memory>

namespace espurna {
namespace driver {
namespace uart {

enum class Type {
    Unknown,
    Software,
    Uart0,
    Uart1,
};

struct Port {
    Type type;
    bool tx;
    bool rx;
    Stream* stream;
};

using PortPtr = std::unique_ptr<Port>;

} // namespace uart
} // namespace driver
} // namespace espurna

espurna::driver::uart::PortPtr uartPort(size_t index);
void uartSetup();

// A driver (e.g. a sensor) that talks to the device on this port takes it for itself.
// Serial log and serial terminal let go of a claimed port; they are notified through
// uartOnClaim() and return what they stopped (PSTR) or nullptr when the port was not theirs.
// Returns false when the port is not set up or is already claimed by another owner.
using UartClaimRelease = const char* (*)(size_t index);
void uartOnClaim(UartClaimRelease);
bool uartClaim(size_t index, const __FlashStringHelper* owner);

// Owner of the port, nullptr when it is free
const __FlashStringHelper* uartClaimed(size_t index);

