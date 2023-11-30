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
