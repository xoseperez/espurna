/*

PWM MODULE

Copyright (C) 2019-2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if PWM_SUPPORT

#include "pwm.h"

#if PWM_PROVIDER == PWM_PROVIDER_GENERIC
#include "libs/esp8266_pwm.h"
#endif

#if defined(ESP8266) and (PWM_PROVIDER == PWM_PROVIDER_ARDUINO)
extern "C" bool stopWaveform(uint8_t pin);
#endif

namespace espurna {
namespace driver {
namespace pwm {
namespace build {

constexpr uint32_t frequency() {
    return PWM_FREQUENCY;
}

constexpr uint32_t resolution() {
    return PWM_RESOLUTION;
}

constexpr float limit() {
    return PWM_DUTY_LIMIT;
}

} // namespace build

namespace settings {
namespace keys {

alignas(4) static constexpr char Frequency[] PROGMEM = "pwmFreq";
alignas(4) static constexpr char Resolution[] PROGMEM = "pwmRes";
alignas(4) static constexpr char Limit[] PROGMEM = "pwmLimit";

} // namespace keys

namespace {

uint32_t frequency() {
    return getSetting(keys::Frequency, build::frequency());
}

[[gnu::unused]] uint32_t resolution() {
    return getSetting(keys::Resolution, build::resolution());
}

[[gnu::unused]] float limit() {
    return getSetting(keys::Limit, build::limit());
}

} // namespace
} // namespace settings

namespace {
namespace internal {

uint32_t duty_limit;

} // namespace internal

// TODO: notice that both LEDC and new-pwm API techically support installing multiple pins
// on the same channel, allowing simultaneous pin changes.
// TODO: fade in / out could be implemented here?
// TODO: only consumer right now is lights module, via hw pin <-> channel assignment
// move pin initialization here and expose 'channel-id'? (partially done)
// TODO: in case of new-pwm, this requires some tweaks to init function that replace
// direct pin configuration with a generic pin mask packed into u16

// Use common Arduino Core functions, should not depend on the SDK used.
// - ESP8266 PWM implementation is either PWM or Phase locked, and is selected at build time.
//   Default is PWM locked, enable Phase locked one by adding `enablePhaseLockedWaveform();`
//   somewhere in the code (e.g. in `setup()` right in this file)
//   Core declares both implementations as a standalone function /w `__attribute__((weak))`
//   Linker will prioritize one over the other when symbol from a specific `.cpp.o` is used.
//   (...such round-about way was chosed to support Arduino IDE builds, which do not commonly
// support an easy way of setting global build flags for Core sources...)
// - ESP32 version uses LEDC driver in 10bit mode, and does not support **any settings**.
//   (at least at the time of writing this)
#if PWM_PROVIDER == PWM_PROVIDER_ARDUINO
namespace arduino {

struct Channel {
    uint8_t pin;
    uint32_t duty;
};

namespace scale {

constexpr uint32_t max(uint32_t resolution) {
    return (1 << resolution) - 1;
}

constexpr uint32_t duty(float value, uint32_t resolution) {
    return (std::clamp(value, 0.f, 100.f) / 100.f) * max(resolution);
}

} // namespace scale

namespace internal {

uint32_t resolution { build::resolution() };
std::vector<Channel> channels;

} // namespace internal

PwmRange range() {
    return PwmRange{
        .min = 0,
        .max = scale::max(internal::resolution),
    };
}

// Our internal PINs <-> CHANNELs mapping
size_t channels() {
    return internal::channels.size();
}

// Scaling is controlled internally, just update driver values
void setup() {
    const auto resolution = settings::resolution();
    internal::resolution = resolution;

    const auto max_duty = scale::max(resolution);
    ::analogWriteRange(max_duty);

    const auto frequency = settings::frequency();
    ::analogWriteFreq(frequency);

    DEBUG_MSG_P(PSTR("[PWM] Arduino - Frequency %u (Hz), resolution %u (bits)\n"),
        frequency, resolution);

    const auto limit = settings::limit();
    driver::pwm::internal::duty_limit =
        (limit < 100.f)
            ? (static_cast<float>(max_duty) / 100.f) * limit
            : max_duty;

    if (max_duty != driver::pwm::internal::duty_limit) {
        DEBUG_MSG_P(PSTR("[PWM] Duty limit %u (%s%%)\n"),
            driver::pwm::internal::duty_limit, String(limit, 3).c_str());
    }
}

void update() {
    // Arduino Core updates pins immediately, forcing delayed update
    // b/c of a weird dependency on ::digitalWrite implementation, explicitly stop PWM
    for (auto channel : internal::channels) {
        ::analogWrite(channel.pin, channel.duty);
        if (!channel.duty) {
            stopWaveform(channel.pin);
        }
    }
}

bool init(const uint8_t* begin, const uint8_t* end) {
    if (!internal::channels.size()) {
        for (auto it = begin; it != end; ++it) {
            const auto pin = *it;
            if (gpioLocked(pin)) {
                internal::channels.clear();
                break;
            }

            internal::channels.push_back(Channel{
                .pin = pin,
                .duty = 0,
            });
        }

        for (auto channel : internal::channels) {
            gpioLock(channel.pin);
        }

        return internal::channels.size();
    }

    return false;
}

void duty(size_t channel, uint32_t value) {
    internal::channels[channel].duty =
        std::min(driver::pwm::internal::duty_limit, value);
}

void duty(size_t channel, float value) {
    duty(channel, scale::duty(value, internal::resolution));
}

} // namespace arduino

// Currently, new-pwm - a drop-in replacement for the version provided in the Espressif SDK
// API is the same as the SDK one, so it is possible (in theory) to seamlessly replace one with the other
// But, one would need to fix period <-> frequency scaling, b/c of (NONOS) SDK weird limits
#elif PWM_PROVIDER == PWM_PROVIDER_GENERIC
namespace generic {
namespace pin {

// TODO: peripherals header should expose our register accessor funcs?
// (or just move these to the lib itself?)

static constexpr std::array<uint32_t, 16> addr PROGMEM {
    PERIPHS_IO_MUX_GPIO0_U,    // GPIO0
    PERIPHS_IO_MUX_U0TXD_U,    // GPIO1
    PERIPHS_IO_MUX_GPIO2_U,    // GPIO2
    PERIPHS_IO_MUX_U0RXD_U,    // GPIO3
    PERIPHS_IO_MUX_GPIO4_U,    // GPIO4
    PERIPHS_IO_MUX_GPIO5_U,    // GPIO5
    PERIPHS_IO_MUX_SD_CLK_U,   // GPIO6  **UNUSED**
    PERIPHS_IO_MUX_SD_DATA0_U, // GPIO7  **UNUSED**
    PERIPHS_IO_MUX_SD_DATA1_U, // GPIO8  **UNUSED**
    PERIPHS_IO_MUX_SD_DATA2_U, // GPIO9
    PERIPHS_IO_MUX_SD_DATA3_U, // GPIO10
    PERIPHS_IO_MUX_SD_CMD_U,   // GPIO11 **UNUSED**
    PERIPHS_IO_MUX_MTDI_U,     // GPIO12 **UNUSED**
    PERIPHS_IO_MUX_MTCK_U,     // GPIO13
    PERIPHS_IO_MUX_MTMS_U,     // GPIO14
    PERIPHS_IO_MUX_MTDO_U,     // GPIO15
};

static constexpr std::array<uint32_t, 16> func PROGMEM {
    FUNC_GPIO0,
    FUNC_GPIO1,
    FUNC_GPIO2,
    FUNC_GPIO3,
    FUNC_GPIO4,
    FUNC_GPIO5,
    FUNC_GPIO6,
    FUNC_GPIO7,
    FUNC_GPIO8,
    FUNC_GPIO9,
    FUNC_GPIO10,
    FUNC_GPIO11,
    FUNC_GPIO12,
    FUNC_GPIO13,
    FUNC_GPIO14,
    FUNC_GPIO15,
};

bool valid(unsigned char pin) {
    return !gpioLocked(pin)
        && (pin < addr.size())
        && (pin < func.size());
}

} // namespace pin

// > By default there is one small difference to the SDK. The code uses a unit of 200ns for both period and duty.
// > E.g. for 10% duty cycle at 1kHz you need to specify a period value of 5000 and a duty cycle value of 500,
// > a duty cycle of 5000 or above switches the channel to full on.
namespace scale {

constexpr uint32_t Step { 200 }; // nanoseconds

constexpr uint32_t period(uint32_t frequency) {
    return std::nano::den / frequency / Step;
}

constexpr uint32_t duty(float value, uint32_t period) {
    return (std::clamp(value, 0.f, 100.f) / 100.f) * period;
}

} // namespace scale

namespace internal {

size_t channels { 0 };
uint32_t period { scale::period(build::frequency()) };

} // namespace internal

constexpr size_t ChannelsMax { 8 };

using Channels = std::vector<pwm_pin_info>;
using Duty = std::array<uint32_t, ChannelsMax>;

PwmRange range() {
    return PwmRange{
        .min = 0,
        .max = internal::period,
    };
}

size_t channels() {
    return internal::channels;
}

Duty initial_duty() {
    Duty out = {0};
    return out;
}

pwm_pin_info from_pin(uint8_t pin) {
    return pwm_pin_info{
        .addr = pin::addr[pin],
        .func = pin::func[pin],
        .pin = pin,
    };
}

Channels prepare(const uint8_t* begin, const uint8_t* end) {
    Channels out;

    for (auto it = begin; it != end; ++it) {
        const auto pin = *it;
        if (!pin::valid(pin)) {
            out.clear();
            break;
        }

        out.push_back(from_pin(*it));
    }

    if (out.size() > ChannelsMax) {
        out.clear();
    }

    return out;
}

void update() {
    ::pwm_start();
}

void setup() {
    const auto frequency = settings::frequency();
    internal::period = scale::period(frequency);
    DEBUG_MSG_P(PSTR("[PWM] Generic - Frequency %u (Hz), period %u (ns)\n"), frequency, internal::period);

    const auto limit = settings::limit();
    driver::pwm::internal::duty_limit =
        (limit < 100.f)
            ? (static_cast<float>(internal::period) / 100.f) * limit
            : internal::period;

    if (internal::period != driver::pwm::internal::duty_limit) {
        DEBUG_MSG_P(PSTR("[PWM] Duty limit %u (%s%%)\n"),
            driver::pwm::internal::duty_limit, String(limit, 3).c_str());
    }
}

bool init(const uint8_t* begin, const uint8_t* end) {
    if (!internal::channels) {
        auto channels = prepare(begin, end);
        if (!channels.size()) {
            return false;
        }

        for (auto channel : channels) {
            gpioLock(channel.pin);
        }

        auto duty = initial_duty();
        ::pwm_init(internal::period, duty.data(),
            channels.size(), channels.data());
        update();

        internal::channels = channels.size();

        return true;
    }

    return false;
}

void duty(uint32_t channel, uint32_t value) {
    ::pwm_set_duty(
        std::min(driver::pwm::internal::duty_limit, value), channel);
}

void duty(uint32_t channel, float value) {
    duty(channel, scale::duty(value, internal::period));
}

} // namespace generic
#endif

} // namespace

#if PWM_PROVIDER == PWM_PROVIDER_ARDUINO
using namespace arduino;
#elif PWM_PROVIDER == PWM_PROVIDER_GENERIC
using namespace generic;
#endif

} // namespace pwm
} // namespace driver
} // namespace espurna

PwmRange pwmRange() {
    return espurna::driver::pwm::range();
}

size_t pwmChannels() {
    return espurna::driver::pwm::channels();
}

void pwmDuty(size_t channel, uint32_t duty) {
    espurna::driver::pwm::duty(channel, duty);
}

void pwmDuty(size_t channel, float duty) {
    espurna::driver::pwm::duty(channel, duty);
}

bool pwmInitPins(const uint8_t* begin, const uint8_t* end) {
    return espurna::driver::pwm::init(begin, end);
}

void pwmUpdate() {
    espurna::driver::pwm::update();
}

void pwmSetup() {
    espurna::driver::pwm::setup();
}

#endif
