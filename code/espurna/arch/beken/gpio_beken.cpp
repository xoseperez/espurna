/*

GPIO MODULE FOR BEKEN (LibreTiny)

*/

#if defined(LIBRETINY)

#include "gpio.h"
#include "settings.h"
#include "rtcmem.h"
#include "terminal.h"

#include <Arduino.h>

#include <algorithm>
#include <forward_list>

#if WEB_SUPPORT
#include "ws.h"
#endif

// --------------------------------------------------------------------------

namespace espurna {
namespace gpio {

namespace origin {
namespace internal {

std::forward_list<Origin> origins;

} // namespace internal

void add(Origin origin) {
    internal::origins.remove_if(
        [&](const Origin& other) {
            return other == origin;
        });
    internal::origins.emplace_front(origin);
}

} // namespace origin

class Hardware final : public GpioBase {
public:
    // LibreTiny pin numbers are GPIO numbers (P6 is 6)
    static constexpr size_t Pins = PINS_GPIO_MAX + 1;

    const char* id() const override {
        return "hardware";
    }

    size_t pins() const override {
        return Pins;
    }

    // Only the pins routed out by the module (ref. LibreTiny board variant)
    bool valid(unsigned char pin) const override {
        if (pin >= Pins) return false;
        return pinSupported(pinInfo(pin), PIN_GPIO);
    }

    bool validForOutput(unsigned char pin) const override {
        return valid(pin);
    }

    bool lock(unsigned char pin) const override {
        if (pin >= Pins) return false;
        return _locked[pin];
    }

    void lock(unsigned char pin, bool value) override {
        if (pin < Pins) {
            _locked[pin] = value;
        }
    }

    BasePinPtr pin(unsigned char pin) override;

private:
    bool _locked[Pins] { false };
};

class HardwarePin final : public BasePin {
public:
    explicit HardwarePin(unsigned char pin) : _pin(pin) {}

    const char* id() const override {
        return "hardware";
    }

    unsigned char pin() const override {
        return _pin;
    }

    // ArduinoCore-API has enums instead of plain ints, values are the same
    void pinMode(int8_t mode) override {
        ::pinMode(_pin, static_cast<PinMode>(mode));
    }

    void digitalWrite(int8_t val) override {
        ::digitalWrite(_pin, static_cast<PinStatus>(val));
    }

    int digitalRead() override {
        return ::digitalRead(_pin);
    }

private:
    unsigned char _pin;
};

BasePinPtr Hardware::pin(unsigned char pin) {
    return BasePinPtr(new HardwarePin(pin));
}

} // namespace gpio
} // namespace espurna

// --------------------------------------------------------------------------

String BasePin::description() const {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s @ GPIO%02hhu", id(), pin());
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
        // To be implemented or ported
        // ptr = &mcp23s08Gpio();
#endif
        break;
    case GpioType::None:
        break;
    }

    return ptr;
}

BasePinPtr gpioRegister(GpioBase& base, unsigned char gpio, espurna::SourceLocation source_location) {
    BasePinPtr result;
    if (gpioLock(base, gpio, source_location)) {
        result = base.pin(gpio);
    }
    return result;
}

BasePinPtr gpioRegister(unsigned char gpio, espurna::SourceLocation source_location) {
    return gpioRegister(hardwareGpio(), gpio, source_location);
}

#if WEB_SUPPORT
namespace espurna {
namespace gpio {
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

    JsonObject& info = root.createNestedObject(F("gpioInfo"));

    JsonArray& locks = info.createNestedArray(F("failed-locks"));
    for (auto& origin : origin::internal::origins) {
        if (!origin.result) {
            JsonArray& entry = locks.createNestedArray();
            entry.add(origin.pin);
            entry.add(origin.location.file);
            entry.add(origin.location.func);
            entry.add(origin.location.line);
        }
    }
}

void setup() {
    wsRegister()
        .onVisible(onVisible);
}

} // namespace web
} // namespace gpio
} // namespace espurna
#endif

#if TERMINAL_SUPPORT
namespace espurna {
namespace gpio {
namespace terminal {
namespace {

// BK72xx GPIO config registers, one word per pin (ref. BDK driver/gpio/gpio.h)
// bit 0 input level, bit 1 output level, bit 2 input enable, bit 3 output disable, bit 6 second function
constexpr uintptr_t RegGpioConfig { 0x00802800 };

volatile uint32_t& reg(uintptr_t address) {
    return *reinterpret_cast<volatile uint32_t*>(address);
}

uint32_t pin_config(unsigned char pin) {
    return reg(RegGpioConfig + (pin * 4));
}

PROGMEM_STRING(GpioLocks, "GPIO.LOCKS");

void gpio_list_origins(::terminal::CommandContext&& ctx) {
    for (const auto& origin : origin::internal::origins) {
        ctx.output.printf_P(PSTR("%c %c %s GPIO%hhu\t%d:%s:%s\n"),
            origin.result ? ' ' : '!',
            origin.lock ? '*' : ' ',
            origin.base, origin.pin,
            origin.location.line,
            origin.location.file,
            origin.location.func);
    }
}

int gpio_pin(const String& value) {
    int pin = espurna::settings::internal::convert<int>(value);
    if ((pin < 0) || !gpioValid(pin)) {
        pin = -1;
    }

    return pin;
}

PROGMEM_STRING(Gpio, "GPIO");

void gpio_read_write(::terminal::CommandContext&& ctx) {
    const auto pin_arg = ctx.argv.size() >= 2;
    const int pin = pin_arg
        ? gpio_pin(ctx.argv[1])
        : -1;

    if (pin_arg && pin < 0) {
        terminalError(ctx, STRING_VIEW("Invalid pin number"));
        return;
    }

    int start = 0;
    int end = gpioPins();

    switch (ctx.argv.size()) {
    case 3:
        ::digitalWrite(pin, espurna::settings::internal::convert<bool>(ctx.argv[2])
            ? HIGH : LOW);
        break;

    case 2:
        start = pin;
        end = pin + 1;
        // fallthrough!

    case 1:
        for (auto current = start; current < end; ++current) {
            if (!gpioValid(current)) {
                continue;
            }

            const auto config = pin_config(current);
            const bool function = config & (1 << 6);
            const bool output = !(config & (1 << 3));

            ctx.output.printf_P(
                PSTR("%c %8s @ GPIO%02d (%s) 0x%02X\n"),
                gpioLocked(current) ? '*' : ' ',
                function ? PSTR("FUNCTION")
                    : output ? PSTR("OUTPUT")
                    : PSTR("INPUT"),
                current,
                (output ? (config & (1 << 1)) : (config & (1 << 0)))
                    ? PSTR("HIGH")
                    : PSTR("LOW"),
                config & 0xff);
        }
        break;
    }

    terminalOK(ctx);
}

PROGMEM_STRING(Mode, "GPIO.MODE");

void gpio_mode(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 3) {
        const auto pin = gpio_pin(ctx.argv[1]);
        if (pin < 0) {
            terminalError(ctx, STRING_VIEW("Invalid pin number"));
            return;
        }

        const auto mode = ctx.argv[2];
        if (mode == F("output")) {
            ::pinMode(pin, OUTPUT);
        } else if (mode == F("input")) {
            ::pinMode(pin, INPUT);
        } else if (mode == F("input-pull-up")) {
            ::pinMode(pin, INPUT_PULLUP);
        } else if (mode == F("input-pull-down")) {
            ::pinMode(pin, INPUT_PULLDOWN);
        } else {
            terminalError(ctx, STRING_VIEW("Invalid mode"));
            return;
        }

        terminalOK(ctx);
        return;
    }

    terminalError(ctx, F("GPIO.MODE <GPIO> <output|input|input-pull-up|input-pull-down>"));
}

PROGMEM_STRING(RegRead, "REG.READ");

void reg_read(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 2) {
        const auto address = espurna::settings::internal::convert<uint32_t>(ctx.argv[1]);
        ctx.output.printf_P(PSTR("0x%08X -> 0x%08X\n"),
            address, reg(address));
        terminalOK(ctx);
        return;
    }

    terminalError(ctx, F("REG.READ <ADDRESS>"));
}

PROGMEM_STRING(RegWrite, "REG.WRITE");

void reg_write(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() == 3) {
        const auto address = espurna::settings::internal::convert<uint32_t>(ctx.argv[1]);
        const auto value = espurna::settings::internal::convert<uint32_t>(ctx.argv[2]);

        ctx.output.printf_P(PSTR("0x%08X -> 0x%08X\n"), address, value);
        reg(address) = value;

        terminalOK(ctx);
        return;
    }

    terminalError(ctx, F("REG.WRITE <ADDRESS> <VALUE>"));
}

static constexpr espurna::terminal::Command Commands[] PROGMEM {
    {GpioLocks, gpio_list_origins},
    {Gpio, gpio_read_write},
    {Mode, gpio_mode},
    {RegRead, reg_read},
    {RegWrite, reg_write},
};

} // namespace

void setup() {
    espurna::terminal::add(Commands);
}

} // namespace terminal
} // namespace gpio
} // namespace espurna
#endif

void gpioSetup() {
#if WEB_SUPPORT
    espurna::gpio::web::setup();
#endif
#if TERMINAL_SUPPORT
    espurna::gpio::terminal::setup();
#endif
}

// Nothing but the system word survives the reset (ref. rtcmem_beken.cpp)
void hardwareGpioIgnore(unsigned char) {
}

void gpioLockOrigin(espurna::gpio::Origin origin) {
    espurna::gpio::origin::add(std::move(origin));
}

#endif // LIBRETINY
