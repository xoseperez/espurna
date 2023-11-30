/*

UART MODULE

Copyright (C) 2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if UART_SUPPORT

#include "uart.h"
#include "utils.h"

#if UART_SOFTWARE_SUPPORT
#include <SoftwareSerial.h>
#endif

#include <array>
#include <utility>

namespace espurna {
namespace driver {
namespace uart {
namespace {

enum class Parity {
    None,
    Even,
    Odd,
};

struct Config {
    uint8_t data_bits;
    Parity parity;
    uint8_t stop_bits;
};

} // namespace
} // namespace uart
} // namespace driver

namespace settings {
namespace internal {
namespace {

PROGMEM_STRING(ParityNone, "none");
PROGMEM_STRING(ParityEven, "even");
PROGMEM_STRING(ParityOdd, "odd");

static constexpr std::array<settings::options::Enumeration<driver::uart::Parity>, 3> ParityOptions PROGMEM {
    {{driver::uart::Parity::None, ParityNone},
     {driver::uart::Parity::Even, ParityEven},
     {driver::uart::Parity::Odd, ParityOdd}}
};

} // namespace

template <>
driver::uart::Parity convert(const String& value) {
    return convert(ParityOptions, value, driver::uart::Parity::None);
}

String serialize(driver::uart::Parity value) {
    return espurna::settings::internal::serialize(ParityOptions, value);
}

} // namespace internal
} // namespace settings

namespace driver {
namespace uart {
namespace {

namespace types {

using HardwareConfig = ::SerialConfig;
using HardwareMode = ::SerialMode;

#if UART_SOFTWARE_SUPPORT
#if defined(ARDUINO_ESP8266_RELEASE_2_7_2) \
    || defined(ARDUINO_ESP8266_RELEASE_2_7_3) \
    || defined(ARDUINO_ESP8266_RELEASE_2_7_4) \
    || defined(ARDUINO_ESP8266_RELEASE_3_0_0) \
    || defined(ARDUINO_ESP8266_RELEASE_3_0_1) \
    || defined(ARDUINO_ESP8266_RELEASE_3_1_0) \
    || defined(ARDUINO_ESP8266_RELEASE_3_1_1)
using SoftwareConfig = ::SoftwareSerialConfig;
#elif defined(ARDUINO_ESP8266_RELEASE_3_1_2)
using SoftwareConfig = ::EspSoftwareSerial::Config;
#else
using SoftwareConfig = ::EspSoftwareSerial::Config;
#endif
#endif

} // namespace types

namespace build {

// i.e. uart0, uart1 and a single sw port
// for general use, hardware uart *should* be prefered method
constexpr size_t PortsMax { 3 };

// todo; technically, tx==2 is also possible
// but, we reserve that for the uart1 TX-only interface
constexpr bool uart0_normal(uint8_t tx, uint8_t rx) {
    return ((tx == 1) && (rx == 3))
        || ((tx == GPIO_NONE) && (rx == 3))
        || ((tx == 1) && (rx == GPIO_NONE));
}

constexpr bool uart0_swapped(uint8_t tx, uint8_t rx) {
    return ((tx == 15) && (rx == 13))
        || ((tx == GPIO_NONE) && (rx == 13))
        || ((tx == 15) && (rx == GPIO_NONE));
}

constexpr bool uart1_normal(uint8_t tx, uint8_t rx) {
    return (tx == 2) && (rx == GPIO_NONE);
}

constexpr uint8_t tx(size_t index) {
    return (0 == index) ? (UART1_TX_PIN) :
        (1 == index) ? (UART2_TX_PIN) :
        (2 == index) ? (UART3_TX_PIN) :
            GPIO_NONE;
}

constexpr uint8_t rx(size_t index) {
    return (0 == index) ? (UART1_RX_PIN) :
        (1 == index) ? (UART2_RX_PIN) :
        (2 == index) ? (UART3_RX_PIN) :
            GPIO_NONE;
}

constexpr uint32_t baudrate(size_t index) {
    return (0 == index) ? (UART1_BAUDRATE) :
        (1 == index) ? (UART2_BAUDRATE) :
        (2 == index) ? (UART3_BAUDRATE) :
            0;
}

constexpr uint8_t data_bits(size_t index) {
    return (0 == index) ? (UART1_DATA_BITS) :
        (1 == index) ? (UART2_DATA_BITS) :
        (2 == index) ? (UART3_DATA_BITS)
            : 0;
}

constexpr Parity parity(size_t index) {
    return (0 == index) ? (Parity::UART1_PARITY) :
        (1 == index) ? (Parity::UART2_PARITY) :
        (2 == index) ? (Parity::UART3_PARITY)
            : Parity::None;
}

constexpr uint8_t stop_bits(size_t index) {
    return (0 == index) ? (UART1_STOP_BITS) :
        (1 == index) ? (UART2_STOP_BITS) :
        (2 == index) ? (UART3_STOP_BITS)
            : 0;
}

constexpr bool invert(size_t index) {
    return (0 == index) ? (UART1_INVERT == 1) :
        (1 == index) ? (UART2_INVERT == 1) :
        (2 == index) ? (UART3_INVERT == 1)
            : false;
}

} // namespace build

constexpr int data_bits_from_config(uint8_t bits) {
    return (bits == 5) ? 0 :
        (bits == 6) ? 0b100 :
        (bits == 7) ? 0b1000 :
        (bits == 8) ? 0b1100 :
            data_bits_from_config(8);
}

constexpr int parity_from_config(Parity parity) {
    return (parity == Parity::None) ? 0 :
        (parity == Parity::Even) ? 0b10 :
        (parity == Parity::Odd) ? 0b11 :
            parity_from_config(Parity::None);
}

constexpr int stop_bits_from_config(uint8_t bits) {
    return (bits == 1) ? 0b10000 :
        (bits == 2) ? 0b110000 :
            stop_bits_from_config(1);
}

template <typename T>
constexpr T from_config(Config);

template <>
constexpr types::HardwareConfig from_config(Config config) {
    return static_cast<types::HardwareConfig>(
        data_bits_from_config(config.data_bits)
        | parity_from_config(config.parity)
        | stop_bits_from_config(config.stop_bits));
}

namespace settings {
namespace keys {

PROGMEM_STRING(TxPin, "uartTx");
PROGMEM_STRING(RxPin, "uartRx");

PROGMEM_STRING(Baudrate, "uartBaud");

PROGMEM_STRING(DataBits, "uartDataBits");
PROGMEM_STRING(StopBits, "uartStopBits");
PROGMEM_STRING(Parity, "uartParity");

PROGMEM_STRING(Invert, "uartInv");

} // namespace keys

uint8_t tx(size_t index) {
    return getSetting({keys::TxPin, index}, build::tx(index));
}

uint8_t rx(size_t index) {
    return getSetting({keys::RxPin, index}, build::rx(index));
}

uint32_t baudrate(size_t index) {
    return getSetting({keys::Baudrate, index}, build::baudrate(index));
}

uint8_t data_bits(size_t index) {
    return getSetting({keys::DataBits, index}, build::data_bits(index));
}

uint8_t stop_bits(size_t index) {
    return getSetting({keys::StopBits, index}, build::stop_bits(index));
}

Parity parity(size_t index) {
    return getSetting({keys::Parity, index}, build::parity(index));
}

bool invert(size_t index) {
    return getSetting({keys::Invert, index}, build::invert(index));
}

} // namespace settings

using StreamPtr = std::unique_ptr<Stream>;

struct BasePort {
    Type type;
    bool tx;
    bool rx;
    StreamPtr stream;
};

using BasePortPtr = std::unique_ptr<BasePort>;

namespace internal {

BasePortPtr ports[build::PortsMax];
bool used_hardware_ports[2] = {false, false};

} // namespace internal

BasePortPtr hardware_port(
        uint32_t baudrate, uint8_t tx, uint8_t rx, Config config, bool invert)
{
    const int number =
        build::uart0_normal(tx, rx)
            ? 0 :
        build::uart0_swapped(tx, rx)
            ? 0 :
        build::uart1_normal(tx, rx)
            ? 1
            : -1;

    if ((number < 0) || (internal::used_hardware_ports[number])) {
        return nullptr;
    }

    const int mode =
        (tx == GPIO_NONE)
            ? SERIAL_RX_ONLY :
        (rx == GPIO_NONE)
            ? SERIAL_TX_ONLY :
        ((tx != GPIO_NONE) && (rx != GPIO_NONE))
            ? SERIAL_FULL 
            : -1;
    if (mode < 0) {
        return nullptr;
    }

    internal::used_hardware_ports[number] = true;

    auto* ptr = new HardwareSerial(number);
    ptr->begin(baudrate,
        from_config<types::HardwareConfig>(config),
        static_cast<types::HardwareMode>(mode),
        tx, invert);
    if ((number == 0) && (build::uart0_swapped(tx, rx))) {
        ptr->flush();
        ptr->swap();
    }

    return std::make_unique<BasePort>(
        BasePort{
            .type = (number == 0) ? Type::Uart0 : Type::Uart1,
            .tx = (tx != GPIO_NONE),
            .rx = (rx != GPIO_NONE),
            .stream = StreamPtr(ptr),
        });
}

// based on the values in v6 of the lib. still, return bits instead of the octal notation used there
#if UART_SOFTWARE_SUPPORT
constexpr int software_serial_data_bits_from_config(uint8_t bits) {
    return (bits == 5) ? 0 :
        (bits == 6) ? 0b1 :
        (bits == 7) ? 0b10 :
        (bits == 8) ? 0b11 :
            software_serial_data_bits_from_config(8);
}

// btw, SoftwareSerial also has Mark and Space
// no support on the hardware peripheral though (afaik)
constexpr int software_serial_parity_from_config(Parity parity) {
    return (parity == Parity::None) ? 0 :
        (parity == Parity::Even) ? 0b10000 :
        (parity == Parity::Odd) ? 0b11000 :
            software_serial_parity_from_config(Parity::None);
}

constexpr int software_serial_stop_bits_from_config(uint8_t bits) {
    return (bits == 1) ? 0b0 :
        (bits == 2) ? 0b10000000 :
            software_serial_stop_bits_from_config(1);
}

template <>
constexpr types::SoftwareConfig from_config(Config config) {
    return static_cast<types::SoftwareConfig>(
        software_serial_data_bits_from_config(config.data_bits)
        | software_serial_parity_from_config(config.parity)
        | software_serial_stop_bits_from_config(config.stop_bits));
    
}

BasePortPtr software_serial_port(
        uint32_t baudrate, uint8_t tx, uint8_t rx, Config config, bool invert)
{
    const int8_t tx_pin = (tx == GPIO_NONE) ? -1 : tx;
    const int8_t rx_pin = (rx == GPIO_NONE) ? -1 : rx;

    auto* ptr = new SoftwareSerial(rx_pin, tx_pin, invert);
    ptr->begin(baudrate, from_config<types::SoftwareConfig>(config));

    return std::make_unique<BasePort>(
        BasePort{
            .type = Type::Software,
            .tx = (tx_pin > 0),
            .rx = (rx_pin > 0),
            .stream = StreamPtr(ptr),
        });
}
#endif

BasePortPtr make_port(size_t index) {
    BasePortPtr out;

    const auto tx = settings::tx(index);
    const auto rx = settings::rx(index);
    if ((tx == GPIO_NONE) && (rx == GPIO_NONE)) {
        return out;
    }

    if ((tx != GPIO_NONE) && !gpioLock(tx)) {
        return out;
    }

    if ((rx != GPIO_NONE) && !gpioLock(rx)) {
        gpioUnlock(tx);
        return out;
    }

    const auto config = Config{
        .data_bits = settings::data_bits(index),
        .parity = settings::parity(index),
        .stop_bits = settings::stop_bits(index),
    };

    const auto baudrate = settings::baudrate(index);
    const auto invert = settings::invert(index);

    if (build::uart0_normal(tx, rx)
     || build::uart0_swapped(tx, rx)
     || build::uart1_normal(tx, rx))
    {
        out = hardware_port(baudrate, tx, rx, config, invert);
    }

#if UART_SOFTWARE_SUPPORT
    if (!out) {
        out = software_serial_port(baudrate, tx, rx, config, invert);
    }
#endif
        
    return out;
}

size_t ports() {
    size_t out = 0;
    for (const auto& port : internal::ports) {
        if (!port) {
            break;
        }
        ++out;
    }

    return out;
}

namespace settings {
namespace query {

#define ID_VALUE(NAME)\
String NAME (size_t id) {\
    return espurna::settings::internal::serialize(\
            espurna::driver::uart::settings::NAME(id));\
}

ID_VALUE(tx)
ID_VALUE(rx)
ID_VALUE(baudrate)
ID_VALUE(data_bits)
ID_VALUE(stop_bits)
ID_VALUE(parity)
ID_VALUE(invert)

#undef ID_VALUE

static constexpr espurna::settings::query::IndexedSetting IndexedSettings[] PROGMEM {
     {keys::TxPin, query::tx},
     {keys::RxPin, query::rx},
     {keys::Baudrate, query::baudrate},
     {keys::DataBits, query::data_bits},
     {keys::StopBits, query::stop_bits},
     {keys::Parity, query::parity},
     {keys::Invert, query::invert},
};

bool checkSamePrefix(StringView key) {
    PROGMEM_STRING(Prefix, "uart");
    return espurna::settings::query::samePrefix(key, Prefix);
}

String findIndexedValueFrom(StringView key) {
    return espurna::settings::query::IndexedSetting::findValueFrom(
        ports(), IndexedSettings, key);

}

void setup() {
    settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findIndexedValueFrom,
    });
}

} // namespace query
} // namespace settings

#if TERMINAL_SUPPORT
namespace terminal {
namespace commands {

String port_type(Type type) {
    const char* out = PSTR("UNKNOWN");

    switch (type) {
    case Type::Unknown:
        break;
    case Type::Software:
        out = PSTR("SOFTWARE");
        break;
    case Type::Uart0:
        out = PSTR("UART0");
        break;
    case Type::Uart1:
        out = PSTR("UART1");
        break;
    }

    return out;
}

void uart(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 1) {
        for (size_t index = 0; index < std::size(internal::ports); ++index) {
            const auto& port = internal::ports[index];
            if (!port) {
                break;
            }

            ctx.output.printf_P(
                PSTR("%zu - %s{tx=%c rx=%c}\n"),
                index, port_type(port->type).c_str(),
                port->tx ? 'y' : 'n',
                port->rx ? 'y' : 'n');
        }

    } else if (ctx.argv.size() == 2) {
        const auto result = parseUnsigned(ctx.argv[1], 10);
        if (!result.ok) {
            terminalError(ctx, F("Invalid ID"));
            return;
        }

        if (result.value >= ports()) {
            ctx.output.print(F("(Not active)"));
        }

        settingsDump(ctx, settings::query::IndexedSettings, result.value);
    } else {
        terminalError(ctx, F("UART [<ID>]"));
        return;
    }

    terminalOK(ctx);
}

PROGMEM_STRING(Uart, "UART");

static constexpr ::terminal::Command List[] PROGMEM {
    {Uart, commands::uart},
};

} // namespace commands

void setup() {
    espurna::terminal::add(commands::List);
}

} // namespace terminal
#endif

PortPtr port(size_t index) {
    const auto& ptr = internal::ports[index];
    if ((index < std::size(internal::ports)) && (ptr)) {
        return std::make_unique<Port>(
            Port{
                .type = ptr->type,
                .tx = ptr->tx,
                .rx = ptr->rx,
                .stream = ptr->stream.get(),
            });
    }

    return nullptr;
}

void setup() {
#if TERMINAL_SUPPORT
    terminal::setup();
#endif

    settings::query::setup();

    for (size_t index = 0; index < build::PortsMax; ++index) {
        auto& port = internal::ports[index];

        port = make_port(index);
        if (!port) {
            break;
        }
    }
}

} // namespace uart

} // namespace
} // namespace driver
} // namespace espurna

espurna::driver::uart::PortPtr uartPort(size_t index) {
    return espurna::driver::uart::port(index);
}

void uartSetup() {
    espurna::driver::uart::setup();
}

#endif // UART_SUPPORT
