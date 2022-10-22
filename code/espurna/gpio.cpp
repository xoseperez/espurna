/*

GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

// --------------------------------------------------------------------------

#include <bitset>
#include <utility>

#include "mcp23s08_pin.h"

#include "terminal.h"
#include "ws.h"

namespace espurna {
namespace peripherals {
namespace {

// TODO: `struct Register { ... }` with additional size params?
// e.g. GPIO IN / OUT register is techically u16, and we don't detect writing past the allowed 'mask'
// something with direct field access may be useful, too; like handling function-select for io mux
// but, *something other than bitfields*, since we can't really guarantee compiler won't optimize reads / writes
// (and since we are not bound by the SDK requirement of writing in C, and could just write C++ code)

inline uint32_t reg_read(uintptr_t) __attribute__((always_inline));
uint32_t reg_read(uintptr_t address) {
    return *reinterpret_cast<volatile uint32_t*>(address);
}

inline void reg_write(uintptr_t, uint32_t) __attribute__((always_inline));
void reg_write(uintptr_t address, uint32_t value) {
    *reinterpret_cast<volatile uint32_t*>(address) = value;
}

template <typename T>
inline void reg_apply(uintptr_t, T&&) __attribute__((always_inline));

template <typename T>
void reg_apply(uintptr_t address, T&& func) {
    static_assert(!std::is_integral<T>::value, "");
    reg_write(address, func(reg_read(address)));
}

// At the time of writing this...
// Current version of Arduino Core uses NONOS, plus a set of "AVR-Style" macros
// - https://github.com/esp8266/Arduino/blob/3.0.2/cores/esp8266/esp8266_peri.h
//
// NONOS does not really have a very good register reference
// - https://github.com/espressif/ESP8266_NONOS_SDK/blob/release/v3.0.5/include/eagle_soc.h
//   (base addresses, register get / set macros, etc.)
// - https://github.com/espressif/ESP8266_NONOS_SDK/blob/release/v3.0.5/driver_lib/include/driver/slc_register.h
// - https://github.com/espressif/ESP8266_NONOS_SDK/blob/release/v3.0.5/driver_lib/include/driver/spi_register.h
// - https://github.com/espressif/ESP8266_NONOS_SDK/blob/release/v3.0.5/driver_lib/include/driver/uart_register.h
// (note that Core still uses v2.2, but it is not available through github)
//
// While the more recent RTOS SDK (based on esp-idf v3.4) has even more *_register.h headers
// Plus, especially helpful are the newly introduced *_struct.h headers, where structs are mapped
// on register memory addresses and specific bits or range of bits are declared through bitfields.
// - https://github.com/espressif/ESP8266_RTOS_SDK/blob/release/v3.4/components/esp8266/include/esp8266/eagle_soc.h
// - https://github.com/espressif/ESP8266_RTOS_SDK/blob/release/v3.4/components/esp8266/include/esp8266/efuse_register.h
// - https://github.com/espressif/ESP8266_RTOS_SDK/blob/release/v3.4/components/esp8266/include/esp8266/pin_mux_register.h
// - https://github.com/espressif/ESP8266_RTOS_SDK/blob/release/v3.4/components/esp8266/include/esp8266/rtc_register.h
// - https://github.com/espressif/ESP8266_RTOS_SDK/blob/release/v3.4/components/esp8266/include/esp8266/slc_register.h
// - https://github.com/espressif/ESP8266_RTOS_SDK/blob/release/v3.4/components/esp8266/include/esp8266/spi_register.h
// - https://github.com/espressif/ESP8266_RTOS_SDK/blob/release/v3.4/components/esp8266/include/esp8266/timer_register.h
// - https://github.com/espressif/ESP8266_RTOS_SDK/blob/release/v3.4/components/esp8266/include/esp8266/uart_register.h
//
// Current ESP32 boards have CMSIS-SVD (.svd) that may be used for code generation
// - https://github.com/espressif/svd/
// But, there does not seem to be anything up-to-date for ESP8266.
//
// Rust-based esp-pacs project provides one
// - https://github.com/esp-rs/esp-pacs/blob/main/esp8266/svd/esp8266.base.svd
// Be aware that it suffers the same problem as RTOS implementation, though,
// sometimes mixing in ESP32 terms that don't really apply to ESP8266
//
// Here we opt into the RTOS-style approach, but have it private to this file.

static constexpr uintptr_t AddressBase { 0x60000000 };

namespace efuse {

static constexpr uintptr_t AddressBase { 0x3ff00050 };

static constexpr uintptr_t Data0 { AddressBase };
static constexpr uintptr_t Data1 { AddressBase + 0x4 };
static constexpr uintptr_t Data2 { AddressBase + 0x8 };
static constexpr uintptr_t Data3 { AddressBase + 0xc };

using Data = std::array<uint32_t, 4>;

inline Data data() {
    return {{
        reg_read(Data0),
        reg_read(Data1),
        reg_read(Data2),
        reg_read(Data3),
    }};
}

} // namespace efuse

namespace rtc {

static constexpr uintptr_t AddressBase { peripherals::AddressBase + 0x700 };

static constexpr uintptr_t PadXpdDcdcConf { AddressBase + 0xA0 };

static constexpr uintptr_t GpioOutput { AddressBase + 0x68 };
static constexpr uintptr_t GpioEnable { AddressBase + 0x74 };
static constexpr uintptr_t GpioInput { AddressBase + 0x8C };
static constexpr uintptr_t GpioConf { AddressBase + 0x90 };

// 2 low bits, 1 high; same as iomux register
static constexpr uint32_t FunctionSelectMask { (1 << 6) | (1 << 1) | 1 };

// no other pin allows pulldown
static constexpr uint32_t Pulldown { (1 << 3) };

inline bool gpio16_get() __attribute__((always_inline));
bool gpio16_get() {
    return reg_read(GpioInput) & 0b1;
}

inline void gpio16_set(bool) __attribute__((always_inline));
void gpio16_set(bool status) {
    reg_apply(
        GpioOutput,
        [=](uint32_t value) {
            return status ? (value | 0b1) : (value & ~0b1);
        });
}

void gpio16_mode(int8_t mode) {
    reg_apply(
        PadXpdDcdcConf,
        [=](uint32_t value) {
            // Like RTOS attempts to preserve existing function-select,
            // first masking with the full mask and then setting the 1st bit
            // (neither RTOS or NONOS really explain it... so, :shrug:)
            value &= ~FunctionSelectMask;
            value |= 0b1;

            // ESP8266 does not have INPUT_PULLDOWN definition, and instead
            // has a GPIO16-specific INPUT_PULLDOWN_16:
            // - https://github.com/esp8266/Arduino/issues/478
            // - https://github.com/esp8266/Arduino/commit/1b3581d55ebf0f8c91e081f9af4cf7433d492ec9
            // Only RTOS SDK includes GPIO definitions, so we include our own unused flag for compatibility
            // (and continue to hope it does not collide with FUNCTION... mask checks)
            value &= ~Pulldown;
            if ((mode == INPUT_PULLDOWN) || (mode == INPUT_PULLDOWN_16)) {
                value |= Pulldown;
            }

            return value;
        });

    // Arduino always erased this, RTOS applies mask. just to be safe
    reg_apply(
        GpioConf,
        [=](uint32_t value) {
            return value & ~0b1;
        });

    // Same as above, 1st bit controls the direction
    reg_apply(
        GpioEnable,
        [=](uint32_t value) {
            value &= ~0b1;
            if (mode == OUTPUT) {
                value |= 0b1;
            }

            return value;
        });
}

} // namespace rtc

namespace pin {

static constexpr uintptr_t AddressBase { peripherals::AddressBase + 0x300 };

static constexpr uintptr_t GpioOutput { AddressBase };
static constexpr uintptr_t GpioOutputSet { AddressBase + 0x4 };
static constexpr uintptr_t GpioOutputClear { AddressBase + 0x8 };

static constexpr uintptr_t GpioEnable { AddressBase + 0xC };
static constexpr uintptr_t GpioEnableSet { AddressBase + 0x10 };
static constexpr uintptr_t GpioEnableClear { AddressBase + 0x14 };

static constexpr uintptr_t GpioInput { AddressBase + 0x18 };

static constexpr uintptr_t GpioInterupt { AddressBase + 0x1C };
static constexpr uintptr_t GpioInteruptSet { AddressBase + 0x20 };
static constexpr uintptr_t GpioInteruptClear { AddressBase + 0x24 };

static constexpr uintptr_t GpioControlBase { peripherals::AddressBase + 0x328 };

static constexpr uintptr_t IoMuxBase { peripherals::AddressBase + 0x800 };

// IO Mux values
// the actual 'function' is described purely through the token name
// we see in pin_mux_register.h, it may not be the same for two
// different pins. plus, gpio16 has a completely different register layout
// (see rtc namespace)
static constexpr uint32_t FunctionLowBit1 { (1 << 4) };
static constexpr uint32_t FunctionLowBit2 { (1 << 5) };
static constexpr uint32_t FunctionHighBit { (1 << 8) };

static constexpr uint32_t PullupMask { (1 << 7) };

static constexpr uint32_t FunctionSelectMask { FunctionHighBit | FunctionLowBit2 | FunctionLowBit1 };
static constexpr uint32_t IoMuxMask { PullupMask | FunctionSelectMask };

// Control values
static constexpr uint32_t Source { 1 };
static constexpr uint32_t Driver { (1 << 2) };

// normal interrupt types
static constexpr uint32_t InterruptRising { (1 << 7) }; // aka positive edge
static constexpr uint32_t InterruptFalling { (1 << 8) }; // aka negative edge
static constexpr uint32_t InterruptChange { (1 << 8) | (1 << 7) }; // aka any edge

// special wake-up interrupts
static constexpr uint32_t InterruptLow { (1 << 9) }; // aka low level trigger
static constexpr uint32_t InterruptHigh { (1 << 9) | (1 << 7) }; // aka high level trigger
static constexpr uint32_t WakeupEnabled { (1 << 10) }; // ^ should set this bit as well when low or high are set

static constexpr uint32_t InterruptMask { (1 << 9) | (1 << 8) | (1 << 7) };
static constexpr uint32_t ControlMask { WakeupEnabled | InterruptMask | Driver | Source };

// ref. PERIPHS_IO_MUX_...
constexpr uintptr_t ioMuxOffset(uint8_t pin) {
    return (pin == 0) ? 0x34 :
        (pin == 1) ? 0x18 :
        (pin == 2) ? 0x38 :
        (pin == 3) ? 0x14 :
        (pin == 4) ? 0x3C :
        (pin == 5) ? 0x40 :
        (pin == 6) ? 0x1C :
        (pin == 7) ? 0x20 :
        (pin == 8) ? 0x24 :
        (pin == 9) ? 0x28 :
        (pin == 10) ? 0x2C :
        (pin == 11) ? 0x30 :
        (pin == 12) ? 0x04 :
        (pin == 13) ? 0x08 :
        (pin == 14) ? 0x0C :
        (pin == 15) ? 0x10 : 0;
}

constexpr uintptr_t ioMuxAddress(uint8_t pin) {
    return IoMuxBase + ioMuxOffset(pin);
}

constexpr uintptr_t controlOffset(uint8_t pin) {
    return (pin & 0xf) * 4;
}

constexpr uintptr_t controlAddress(uint8_t pin) {
    return GpioControlBase + controlOffset(pin);
}

inline void set(uint8_t, bool) __attribute__((always_inline));
void set(uint8_t pin, bool status) {
    reg_write(status ? GpioOutputSet : GpioOutputClear, (1 << pin));
}

inline bool get(uint8_t) __attribute__((always_inline));
bool get(uint8_t pin) {
    return reg_read(GpioInput) & (1 << pin);
}

// ref. function numbers (FUNC_...) used in the header and techical reference
// with the exception of 4 pins, everything sets it to 3 (0b11)
// ──────────────────────────────────────────────────────────────────────────
// #define GPFFS_GPIO(p)
//    (((p) == 0 || (p) == 2 || (p) == 4 || (p) == 5)  ? 0
//     : ((p) == 16)                                   ? 1
//     : 3)
constexpr uint32_t function_select_gpio(uint8_t pin) {
    return ((pin == 0) || (pin == 2) || (pin == 4) || (pin == 5))
        ? 0 : (FunctionLowBit2 | FunctionLowBit1);
}

// NONOS SDK `GPFFS_BUS`, with 'bus' function bits for certain pins (like SPI, UART, I2S, etc.)
// ────────────────────────────────────────────────────────────────────────────────────────────
// #define GPFFS_BUS(p)
//   (((p) == 1 || (p) == 3)                                           ? 0
//    : ((p) == 2 || (p) == 12 || (p) == 13 || (p) == 14 || (p) == 15) ? 2
//    : ((p) == 0)                                                     ? 4
//    : 1)
constexpr uint32_t function_select_arduino_special(uint8_t pin) {
    return ((pin == 1) || (pin == 3))
            ? 0 :
        ((pin == 2) || (pin == 12)
         || (pin == 13) || (pin == 14)
         || (pin == 15))
            ? FunctionLowBit2 :
        (pin == 0)
            ? FunctionHighBit
            : FunctionLowBit1;
}

// Arduino, like SDK, encodes function select in a 3bit mask that is later 'unpacked'
// into a real mask. Simple if-else, since the original involves unnecessary optional shifts.
constexpr uint32_t function_select_arduino_mask(uint8_t mode) {
    return (mode == FUNCTION_1) ? FunctionLowBit1 :
        (mode == FUNCTION_2) ? FunctionLowBit2 :
        (mode == FUNCTION_3) ? (FunctionLowBit2 | FunctionLowBit1) :
        (mode == FUNCTION_4) ? FunctionHighBit : 0;
}

// For some reason, PULLUP was not encoded in the mode directly, so UART
// should be configured by us as well to avoid these exceptional cases
constexpr uint32_t pullup_arduino_mask(uint8_t pin, uint8_t mode) {
    return (((pin == 13) && (mode == FUNCTION_4))
         || ((pin == 3) && (mode == SPECIAL))) ? PullupMask : 0;
}

// Will be exposed as `pinMode(pin, mode)`, must maintain Arduino compatibility with mode selection
void mode(uint8_t pin, uint8_t mode) {
    reg_apply(
        ioMuxAddress(pin),
        [=](uint32_t value) {
            value &= ~PullupMask;
            if (mode == INPUT_PULLUP) {
                value |= PullupMask;
            }

            // b/c Arduino core only has `pinMode` and does not
            // have something like `pinPullup` or `pinFunction`,
            // we are forced into handling these special cases
            value |= pullup_arduino_mask(pin, mode);

            value &= ~FunctionSelectMask;
            if (mode == SPECIAL) {
                value |= function_select_arduino_special(pin);
            } else if (mode & FUNCTION_0) {
                value |= function_select_arduino_mask(mode);
            } else {
                value |= function_select_gpio(pin);
            }

            return value;
        });

    // Notice that Core also implements WAKEUP_PULL{UP,DOWN}, that attempt to wrap around
    // the LOW and HIGH wakeup triggers (respectively). The issue is, pull{up,down} means
    // internal bit setting as well, since only the UP bits works for GPIO0...15
    // Simply ignoring those modes

    switch (mode) {
    case INPUT:
    case INPUT_PULLUP:
    case SPECIAL:
    case FUNCTION_1:
    case FUNCTION_2:
    case FUNCTION_3:
    case FUNCTION_4:
        reg_apply(
            GpioEnableClear,
            [=](uint32_t value) {
                return value | (1 << pin);
            });
        break;
    }

    // notice that we preserve interrupt mask, but it is not yet clear
    // whether it is the right approach.
    reg_apply(
        controlAddress(pin),
        [=](uint32_t value) {
            value &= (WakeupEnabled | InterruptMask);
            if ((mode == INPUT) || (mode == OUTPUT_OPEN_DRAIN)) {
                value |= Driver;
            }

            return value;
        });

    // FUNCTION_0 special case for TX pin. Does not really seem to matter in hardware,
    // but useful for debug code checking whether it's INPUT or OUTPUT
    if ((mode == OUTPUT) || (mode == OUTPUT_OPEN_DRAIN) || (mode == FUNCTION_0)) {
        reg_apply(
            GpioEnableSet,
            [=](uint32_t value) {
                return value | (1 << pin);
            });
    }
}

} // namespace pin

} // namespace
} // namespace peripherals

namespace gpio {
namespace {

namespace settings {
namespace options {

using espurna::settings::options::Enumeration;

PROGMEM_STRING(None, "none");
PROGMEM_STRING(Hardware, "hardware");

#if MCP23S08_SUPPORT
PROGMEM_STRING(Mcp23s08, "mcp23s08");
#endif

static constexpr Enumeration<GpioType> GpioTypeOptions[] PROGMEM {
    {GpioType::Hardware, Hardware},
#if MCP23S08_SUPPORT
    {GpioType::Mcp23s08, Mcp23s08},
#endif
    {GpioType::None, None},
};

} // namespace options
} // namespace settings

// TODO(?)
// will probably stay cached, but... note that both vtable and method are in flash
// may not be safe from within ISR (50/50), and our sensor code still uses Core methods
// possibly, declare and define in header as previous version for forcibly inline?

class Gpio16Pin final : public BasePin {
private:
    static constexpr unsigned char Pin { 16 };

public:
    Gpio16Pin() = default;

    void pinMode(int8_t mode) override {
        peripherals::rtc::gpio16_mode(mode);
    }

    void digitalWrite(int8_t val) override {
        peripherals::rtc::gpio16_set(val == 1);
    }

    int digitalRead() override {
        return peripherals::rtc::gpio16_get();
    }

    unsigned char pin() const override {
        return Pin;
    }

    const char* id() const override {
        return "Gpio16Pin";
    }
};

class GpioPin final : public BasePin {
public:
    GpioPin() = delete;
    explicit GpioPin(unsigned char pin) :
        _pin(pin)
    {}

    void pinMode(int8_t mode) override {
        peripherals::pin::mode(_pin, mode);
    }

    void digitalWrite(int8_t val) override {
        peripherals::pin::set(_pin, (val == 1));
    }

    int digitalRead() override {
        return peripherals::pin::get(_pin);
    }

    unsigned char pin() const override {
        return _pin;
    }

    const char* id() const override {
        return "GpioPin";
    }

private:
    unsigned char _pin;
};

class Hardware : public GpioBase {
public:
    static constexpr size_t Pins { 17 };

    Hardware() {
        // https://github.com/espressif/esptool/blob/f04d34bcab29ace798d2d3800ba87020cccbbfdd/esptool.py#L1060-L1070
        // "One or the other efuse bit is set for ESP8285"
        // https://github.com/espressif/ESP8266_RTOS_SDK/blob/3c055779e9793e5f082afff63a011d6615e73639/components/esp8266/include/esp8266/efuse_register.h#L20-L21
        // "define EFUSE_IS_ESP8285    (1 << 4)"
        const auto data = peripherals::efuse::data();
        _esp8285 = (data[0] & (1 << 4))
                || (data[2] & (1 << 16));
    }

    const char* id() const override {
        return "hardware";
    }

    size_t pins() const override {
        return Pins;
    }

    bool lock(unsigned char index) const override {
        return _mask[index];
    }

    void lock(unsigned char index, bool value) override {
        _mask.set(index, value);
    }

    bool valid(unsigned char index) const override {
        switch (index) {
        case 0 ... 5:
            return true;
        case 9:
        case 10:
            return _esp8285;
        case 12 ... 16:
            return true;
        }

        return false;
    }

    BasePinPtr pin(unsigned char pin) override {
        if (pin == 16) {
            return std::make_unique<Gpio16Pin>();
        }

        return std::make_unique<GpioPin>(pin);
    }

private:
    using Mask = std::bitset<Pins>;

    bool _esp8285 { false };
    Mask _mask;
};

#if WEB_SUPPORT
namespace web {

void onVisible(JsonObject& root) {
    JsonObject& config = root.createNestedObject(F("gpioConfig"));

    constexpr GpioType known_types[] {
        GpioType::Hardware,
#if MCP23S08_SUPPORT
        GpioType::Mcp23s08,
#endif
    };

    JsonArray& types = config.createNestedArray(F("types"));

    for (auto& type : known_types) {
        auto* base = gpioBase(type);
        if (base) {
            JsonArray& entry = types.createNestedArray();
            entry.add(base->id());
            entry.add(static_cast<int>(type));

            JsonArray& pins = config.createNestedArray(base->id());
            for (size_t pin = 0; pin < base->pins(); ++pin) {
                if (base->valid(pin)) {
                    pins.add(pin);
                }
            }
        }
    }
}

void setup() {
    wsRegister()
        .onVisible(onVisible);
}

} // namespace web
#endif

namespace origin {
namespace internal {

std::forward_list<Origin> origins;

} // namespace internal

void add(Origin origin) {
    internal::origins.emplace_front(origin);
}

} // namespace origin

#if TERMINAL_SUPPORT
namespace terminal {

PROGMEM_STRING(GpioLocks, "GPIO.LOCKS");

void gpio_list_origins(::terminal::CommandContext&& ctx) {
    for (const auto& origin : origin::internal::origins) {
        ctx.output.printf_P(PSTR("%c %s GPIO%hhu\t%d:%s:%s\n"),
            origin.lock ? '*' : ' ',
            origin.base, origin.pin,
            origin.location.line,
            origin.location.file,
            origin.location.func);
    }
}

PROGMEM_STRING(Gpio, "GPIO");

void gpio_read_write(::terminal::CommandContext&& ctx) {
    const int pin = (ctx.argv.size() >= 2)
        ? espurna::settings::internal::convert<int>(ctx.argv[1])
        : -1;

    if ((pin >= 0) && !gpioValid(pin)) {
        terminalError(ctx, F("Invalid pin number"));
        return;
    }

    int start = 0;
    int end = gpioPins();

    using namespace peripherals;

    const auto outputs = reg_read(pin::GpioEnable);
    const auto readings = reg_read(pin::GpioInput);

    const auto gpio16output = reg_read(rtc::GpioEnable);
    const auto gpio16reading = reg_read(rtc::GpioInput);

    switch (ctx.argv.size()) {
    case 3:
    {
        // even in INPUT mode, we are still techically allowed to set output register
        // (like it is used in software I2C implementation with INPUT_PULLUP)
        const bool status = espurna::settings::internal::convert<bool>(ctx.argv[2]);
        if (pin == 16) {
            rtc::gpio16_set(status);
        } else {
            pin::set(pin, status);
        }
        break;
    }

    case 2:
        start = pin;
        end = pin + 1;
        // fallthrough!

    case 1:
        for (auto current = start; current < end; ++current) {
            if (gpioValid(current)) {
                const bool reading = (current == 16)
                    ? ((gpio16reading & 0b1) != 0)
                    : ((readings & (1 << current)) != 0);

                const bool output = (current == 16)
                    ? ((gpio16output & 0b1) != 0)
                    : (outputs & (1 << current));

                ctx.output.printf_P(
                    PSTR("%c %6s @ GPIO%02d%s\n"),
                        gpioLocked(current) ? '*' : ' ',
                        output
                            ? "OUTPUT"
                            : "INPUT",
                        current,
                        !output
                            ? reading
                                ? " (HIGH)"
                                : " (LOW)"
                            : ""
                    );
            }
        }

        break;

    }

    terminalOK(ctx);
}

PROGMEM_STRING(RegRead, "REG.READ");

void reg_read(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 2) {
        const auto convert = espurna::settings::internal::convert<uint32_t>;
        const auto address = convert(ctx.argv[1]);

        ctx.output.printf_P(PSTR("0x%08X -> 0x%08X\n"),
            address, peripherals::reg_read(address));
        terminalOK(ctx);
        return;
    }

    terminalError(ctx, F("REG.READ <ADDRESS>"));
}

PROGMEM_STRING(RegWrite, "REG.WRITE");

void reg_write(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 3) {
        const auto convert = espurna::settings::internal::convert<uint32_t>;

        const auto address = convert(ctx.argv[1]);
        const auto value = convert(ctx.argv[2]);

        ctx.output.printf_P(PSTR("0x%08X -> 0x%08X\n"), address, value);
        peripherals::reg_write(address, value);

        terminalOK(ctx);
        return;
    }

    terminalError(ctx, F("REG.WRITE <ADDRESS> <VALUE>"));
}

static constexpr espurna::terminal::Command Commands[] PROGMEM {
    {GpioLocks, gpio_list_origins},
    {Gpio, gpio_read_write},
    {RegRead, reg_read},
    {RegWrite, reg_write},
};

void setup() {
    espurna::terminal::add(Commands);
}

} // namespace terminal
#endif

} // namespace
} // namespace gpio

namespace settings {
namespace internal {
namespace {

using gpio::settings::options::GpioTypeOptions;

} // namespace

template <>
GpioType convert(const String& value) {
    return convert(GpioTypeOptions, value, GpioType::None);
}

String serialize(GpioType value) {
    return serialize(GpioTypeOptions, value);
}

} // namespace internal
} // namespace settings
} // namespace espurna

// --------------------------------------------------------------------------

// Per https://llvm.org/docs/CodingStandards.html#provide-a-virtual-method-anchor-for-classes-in-headers
// > If a class is defined in a header file and has a vtable (either it has virtual methods or it derives from classes with virtual methods),
// > it must always have at least one out-of-line virtual method in the class. Without this, the compiler will copy the vtable and RTTI into
// > every .o file that #includes the header, bloating .o file sizes and increasing link times.
// - http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1263r0.pdf
// > This technique is unfortunate as it relies on detailed knowledge of how common toolchains work, and it may also require creating
// > a dummy virtual function.
//
// `~BasePin() = default;` is technically inlined, so leaving that in the header.
// Thus, ::description() implementation here becomes that anchor for the BasePin.

String BasePin::description() const {
    char buffer[64];
    snprintf_P(buffer, sizeof(buffer),
        PSTR("%s @ GPIO%02hhu"), id(), pin());
    return buffer;
}

GpioBase& hardwareGpio() {
    static espurna::gpio::Hardware gpio;
    return gpio;
}

GpioBase* gpioBase(GpioType type) {
    GpioBase* ptr { nullptr };

    switch (type) {
    case GpioType::Hardware:
        ptr = &hardwareGpio();
        break;
    case GpioType::Mcp23s08:
#if MCP23S08_SUPPORT
        ptr = &mcp23s08Gpio();
#endif
        break;
    case GpioType::None:
        break;
    }

    return ptr;
}

BasePinPtr gpioRegister(GpioBase& base, unsigned char gpio) {
    BasePinPtr result;

    if (gpioLock(base, gpio)) {
        result = base.pin(gpio);
    }

    return result;
}

BasePinPtr gpioRegister(unsigned char gpio) {
    return gpioRegister(hardwareGpio(), gpio);
}

void gpioSetup() {
#if WEB_SUPPORT
    espurna::gpio::web::setup();
#endif
#if TERMINAL_SUPPORT
    espurna::gpio::terminal::setup();
#endif
}

void gpioLockOrigin(espurna::gpio::Origin origin) {
    espurna::gpio::origin::add(origin);
}

extern "C" {

// All of these have __attribute__((weak)) in the Core files. While the original intent
// seems to be for internal use to override things throuhg 'variant' files, we (so far)
// never used anything but the base ESP8266 implementations.
// Which ones get picked up the linker depends on the build system used, and order
// of the files in the resulting build command.

// *Must* be in IRAM by default, ISR almost certainly include these calls
int IRAM_ATTR digitalRead(uint8_t);
int digitalRead(uint8_t pin) {
    using namespace espurna::peripherals;
    return (pin == 16)
        ? rtc::gpio16_get()
        : pin::get(pin);
}

// *Must* be in IRAM by default, ISR almost certainly include these calls
void IRAM_ATTR digitalWrite(uint8_t, uint8_t);
void digitalWrite(uint8_t pin, uint8_t val) {
    using namespace espurna::peripherals;
    if (pin == 16) {
        rtc::gpio16_set(val == 1);
    } else {
        pin::set(pin, val == 1);
    }
}

void pinMode(uint8_t pin, uint8_t mode) {
    using namespace espurna::peripherals;
    if (pin == 16) {
        rtc::gpio16_mode(mode);
    } else {
        pin::mode(pin, mode);
    }
}

// Special override for Core, allows us to skip init for certain pins when needed
void resetPins() {
    const auto& hardware = hardwareGpio();

    for (size_t pin = 0; pin < espurna::gpio::Hardware::Pins; ++pin) {
        if (!hardware.valid(pin)) {
            continue;
        }
#if DEBUG_SERIAL_SUPPORT
        // TODO: actually check which pin is set as TX via function select
        if (pin == 1) {
            continue;
        }
#endif

        pinMode(pin, INPUT);
    }
}

} // extern "C"
