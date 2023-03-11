/*

LIGHT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "light.h"

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#include "api.h"
#include "mqtt.h"
#include "relay.h"
#include "rpc.h"
#include "rtcmem.h"
#include "ws.h"

#include <ArduinoJson.h>

#include <array>
#include <cstring>
#include <vector>

#include "libs/fs_math.h"

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
#include <my92xx.h>
#endif

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
#include "pwm.h"
#endif

// -----------------------------------------------------------------------------

namespace espurna {
namespace light {

#if __GNUC__ > 4
static_assert(std::is_trivially_copyable<Rgb>::value, "");
static_assert(std::is_trivially_copyable<Hsv>::value, "");
static_assert(std::is_trivially_copyable<TemperatureRange>::value, "");
#endif

constexpr long Rgb::Min;
constexpr long Rgb::Max;

constexpr long Hsv::HueMin;
constexpr long Hsv::HueMax;

constexpr long Hsv::SaturationMin;
constexpr long Hsv::SaturationMax;

constexpr long Hsv::ValueMin;
constexpr long Hsv::ValueMax;

static_assert(MiredsCold < MiredsWarm, "");
constexpr long MiredsDefault { (MiredsCold + MiredsWarm) / 2L };

namespace {
namespace build {

constexpr float WhiteFactor { LIGHT_WHITE_FACTOR };

constexpr bool color() {
    return 1 == LIGHT_USE_COLOR;
}

constexpr bool white() {
    return 1 == LIGHT_USE_WHITE;
}

constexpr bool cct() {
    return 1 == LIGHT_USE_CCT;
}

constexpr bool rgb() {
    return 1 == LIGHT_USE_RGB;
}

constexpr bool gamma() {
    return 1 == LIGHT_USE_GAMMA;
}

constexpr bool transition() {
    return 1 == LIGHT_USE_TRANSITIONS;
}

constexpr espurna::duration::Milliseconds transitionTime() {
    return espurna::duration::Milliseconds(LIGHT_TRANSITION_TIME);
}

constexpr espurna::duration::Milliseconds transitionStep() {
    return espurna::duration::Milliseconds(LIGHT_TRANSITION_STEP);
}

constexpr bool save() {
    return 1 == LIGHT_SAVE_ENABLED;
}

constexpr espurna::duration::Milliseconds saveDelay() {
    return espurna::duration::Milliseconds(LIGHT_SAVE_DELAY);
}

constexpr espurna::duration::Milliseconds reportDelay() {
    return espurna::duration::Milliseconds(LIGHT_REPORT_DELAY);
}

constexpr unsigned char enablePin() {
    return LIGHT_ENABLE_PIN;
}

constexpr unsigned char channelPin(size_t index) {
    return (
        (index == 0) ? LIGHT_CH1_PIN :
        (index == 1) ? LIGHT_CH2_PIN :
        (index == 2) ? LIGHT_CH3_PIN :
        (index == 3) ? LIGHT_CH4_PIN :
        (index == 4) ? LIGHT_CH5_PIN : GPIO_NONE
    );
}

constexpr bool inverse(size_t index) {
    return (
        (index == 0) ? (1 == LIGHT_CH1_INVERSE) :
        (index == 1) ? (1 == LIGHT_CH2_INVERSE) :
        (index == 2) ? (1 == LIGHT_CH3_INVERSE) :
        (index == 3) ? (1 == LIGHT_CH4_INVERSE) :
        (index == 4) ? (1 == LIGHT_CH5_INVERSE) : false
    );
}

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX

constexpr my92xx_cmd_t my92xxCommand() {
    return MY92XX_COMMAND;
}

constexpr size_t my92xxChannels() {
    return MY92XX_CHANNELS;
}

constexpr my92xx_model_t my92xxModel() {
    return MY92XX_MODEL;
}

constexpr int my92xxChips() {
    return MY92XX_CHIPS;
}

constexpr int my92xxDiPin() {
    return MY92XX_DI_PIN;
}

constexpr int my92xxDckiPin() {
    return MY92XX_DCKI_PIN;
}

#if defined(MY92XX_MAPPING)
namespace my92xx {

constexpr unsigned char mapping[MY92XX_CHANNELS] {
    MY92XX_MAPPING
};

template <typename... T>
struct FailSafe {
    static constexpr bool value { false };
};

constexpr unsigned char channel(T channel) {
    static_assert(FailSafe<T>::value, "MY92XX_CH# flags should be used instead of MY92XX_MAPPING");
    return mapping[channel];
}

} // namespace my92xx

constexpr unsigned char my92xxChannel(size_t channel) {
    return my92xx::channel(channel);
}

#else // !defined(MY92XX_MAPPING)

constexpr unsigned char my92xxChannel(size_t channel) {
    return (channel == 0) ? MY92XX_CH1 :
        (channel == 1) ? MY92XX_CH2 :
        (channel == 2) ? MY92XX_CH3 :
        (channel == 3) ? MY92XX_CH4 :
        (channel == 4) ? MY92XX_CH5 : espurna::light::ChannelsMax;
}

#endif

#endif

} // namespace build

namespace settings {

unsigned char enablePin() {
    return getSetting("ltEnableGPIO", espurna::light::build::enablePin());
}

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
unsigned char channelPin(size_t index) {
    return getSetting({"ltDimmerGPIO", index}, build::channelPin(index));
}
#endif

bool inverse(size_t index) {
    return getSetting({"ltInv", index}, build::inverse(index));
}

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
size_t my92xxChannels() {
    return getSetting("ltMy92xxChannels", build::my92xxChannels());
}

my92xx_model_t my92xxModel() {
    return getSetting("ltMy92xxModel", build::my92xxModel());
}

int my92xxChips() {
    return getSetting("ltMy92xxChips", build::my92xxChips());
}

int my92xxDiPin() {
    return getSetting("ltMy92xxDiGPIO", build::my92xxDiPin());
}

int my92xxDckiPin() {
    return getSetting("ltMy92xxDckiGPIO", build::my92xxDckiPin());
}

unsigned char my92xxChannel(size_t channel) {
    return getSetting({"ltMy92xxCh", channel}, build::my92xxChannel(channel));
}
#endif

// TODO: avoid clamping here in favour of handlers themselves always making sure values are in range?

long value(size_t channel) {
    const long defaultValue { (channel == 0) ? espurna::light::ValueMax : espurna::light::ValueMin };
    return std::clamp(getSetting({"ch", channel}, defaultValue), espurna::light::ValueMin, espurna::light::ValueMax);
}

void value(size_t channel, long input) {
    setSetting({"ch", channel}, input);
}

espurna::light::Mireds mireds() {
    return getSetting(
        "mireds",
        espurna::light::Mireds{
            .value = espurna::light::MiredsDefault
        });
}

long miredsCold() {
    return getSetting("ltColdMired", espurna::light::MiredsCold);
}

long miredsWarm() {
    return getSetting("ltWarmMired", espurna::light::MiredsWarm);
}

void mireds(espurna::light::Mireds mireds) {
    setSetting("mireds", mireds.value);
}

long brightness() {
    return getSetting("brightness", espurna::light::BrightnessMax);
}

void brightness(long input) {
    setSetting("brightness", input);
}

String mqttGroup() {
    return getSetting("mqttGroupColor");
}

bool color() {
    return getSetting("useColor", build::color());
}

void color(bool value) {
    setSetting("useColor", value);
}

bool white() {
    return getSetting("useWhite", build::white());
}

void white(bool value) {
    setSetting("useWhite", value);
}

bool cct() {
    return getSetting("useCCT", build::cct());
}

void cct(bool value) {
    setSetting("useCCT", value);
}

bool rgb() {
    return getSetting("useRGB", build::rgb());
}

bool gamma() {
    return getSetting("useGamma", build::gamma());
}

bool transition() {
    return getSetting("useTransitions", build::transition());
}

void transition(bool value) {
    setSetting("useTransitions", value);
}

espurna::duration::Milliseconds transitionTime() {
    return getSetting("ltTime", build::transitionTime());
}

void transitionTime(espurna::duration::Milliseconds value) {
    setSetting("ltTime", value.count());
}

espurna::duration::Milliseconds transitionStep() {
    return getSetting("ltStep", build::transitionStep());
}

void transitionStep(espurna::duration::Milliseconds value) {
    setSetting("ltStep", value.count());
}

bool save() {
    return getSetting("ltSave", build::save());
}

espurna::duration::Milliseconds saveDelay() {
    return getSetting("ltSaveDelay", build::saveDelay());
}

} // namespace settings
} // namespace
} // namespace light
} // namespace espurna

// -----------------------------------------------------------------------------

#if RELAY_SUPPORT

class LightStateProvider : public RelayProviderBase {
public:
    espurna::StringView id() const override {
        return STRING_VIEW("light-state");
    }

    void change(bool status) override {
        lightState(status);
        lightUpdate();
    }
};

#endif

namespace {

template <typename T>
long _lightChainedValue(long input, const T& process) {
    return process(input);
}

template <typename T, typename... Args>
long _lightChainedValue(long input, const T& process, Args&&... args) {
    return _lightChainedValue(process(input), std::forward<Args>(args)...);
}

} // namespace

struct LightChannel {
    LightChannel() = default;

    LightChannel(bool inverse, bool gamma) :
        inverse(inverse),
        gamma(gamma)
    {}

    LightChannel& operator=(long input) {
        inputValue = std::clamp(input, espurna::light::ValueMin, espurna::light::ValueMax);
        return *this;
    }

    void apply() {
        value = inputValue;
    }

    template <typename T>
    void apply(const T& process) {
        value = std::clamp(process(inputValue), espurna::light::ValueMin, espurna::light::ValueMax);
    }

    template <typename T, typename... Args>
    void apply(const T& process, Args&&... args) {
        value = std::clamp(
            _lightChainedValue(process(inputValue), std::forward<Args>(args)...),
            espurna::light::ValueMin, espurna::light::ValueMax);
    }

    bool inverse { false };                // re-map the value from [ValueMin:ValueMax] to [ValueMax:ValueMin]
    bool gamma { false };                  // apply gamma correction to the target value

    // TODO: remove in favour of global control, since relays are no longer bound to a single channel?
    bool state { true };                   // is the channel ON

    long inputValue { espurna::light::ValueMin };   // raw, without the brightness
    long value { espurna::light::ValueMin };        // normalized, including brightness
    long target { espurna::light::ValueMin };       // resulting value that will be given to the provider

    float current { espurna::light::ValueMin };     // interim between input and target, used by the transition handler
};

using LightChannels = std::vector<LightChannel>;
LightChannels _light_channels;

namespace espurna {
namespace light {
namespace {

struct Pointers {
    using Type = LightChannels::pointer;
    using Data = std::array<Type, 5>;

    Pointers() = default;
    explicit Pointers(LightChannels&);

    Pointers(const Pointers&) = default;
    Pointers& operator=(const Pointers&) = default;

    Pointers(Pointers&&) = default;
    Pointers& operator=(Pointers&&) = default;

    Pointers& operator=(LightChannels& channels) {
        _data.fill(nullptr);
        reset(channels);
        return *this;
    }

    LightChannel* red() const {
        return _data[0];
    }

    LightChannel* green() const {
        return _data[1];
    }

    LightChannel* blue() const {
        return _data[2];
    }

    LightChannel* warm() const {
        return _data[3];
    }

    LightChannel* cold() const {
        return _data[4];
    }

private:
    void reset(LightChannels& channels);

    Data _data{};
};

void Pointers::reset(LightChannels& channels) {
    switch (channels.size()) {
    case 0:
        break;
    case 1:
        _data[3] = &channels[0];
        break;
    case 2:
        _data[3] = &channels[0];
        _data[4] = &channels[1];
        break;
    case 3:
        _data[0] = &channels[0];
        _data[1] = &channels[1];
        _data[2] = &channels[2];
        break;
    case 4:
        _data[0] = &channels[0];
        _data[1] = &channels[1];
        _data[2] = &channels[2];
        _data[3] = &channels[3];
        break;
    case 5:
        _data[0] = &channels[0];
        _data[1] = &channels[1];
        _data[2] = &channels[2];
        _data[3] = &channels[3];
        _data[4] = &channels[4];
        break;
    }
}

Pointers::Pointers(LightChannels& channels) {
    reset(channels);
}

struct Mapping {
    void reset() {
        _pointers = Pointers();
    }

    template <typename T>
    Mapping operator=(T&& other) {
        _pointers = std::forward<T>(other);
        return *this;
    }

    long red() const {
        return get(_pointers.red());
    }

    void red(long value) {
        set(_pointers.red(), value);
    }

    long green() const {
        return get(_pointers.green());
    }

    void green(long value) {
        set(_pointers.green(), value);
    }

    long blue() const {
        return get(_pointers.blue());
    }

    void blue(long value) {
        set(_pointers.blue(), value);
    }

    long cold() const {
        return get(_pointers.cold());
    }

    void cold(long value) {
        set(_pointers.cold(), value);
    }

    long warm() const {
        return get(_pointers.warm());
    }

    void warm(long value) {
        set(_pointers.warm(), value);
    }

    const Pointers& pointers() const {
        return _pointers;
    }

private:
    static long get(LightChannel* ptr) {
        if (ptr) {
            return ptr->target;
        }

        return espurna::light::ValueMin;
    }

    static void set(LightChannel* ptr, long value) {
        if (ptr) {
            *ptr = value;
        }
    }

    Pointers _pointers;
};

} // namespace
} // namespace light

namespace settings {
namespace internal {

template <>
light::Mireds convert(const String& value) {
    return light::Mireds{ .value = convert<long>(value) };
}

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
template <>
my92xx_model_t convert(const String& value) {
    PROGMEM_STRING(MY9291, "9291");
    PROGMEM_STRING(MY9231, "9231");

    using Options = std::array<espurna::settings::options::Enumeration<my92xx_model_t>, 2>;
    static constexpr Options options {
        {{MY92XX_MODEL_MY9291, MY9291},
         {MY92XX_MODEL_MY9231, MY9231}}
    };

    return convert(options, value, espurna::light::build::my92xxModel());
}
#endif

} // namespace internal
} // namespace settings

} // namespace espurna

namespace {

espurna::light::Mapping _light_mapping;

void _lightUpdateMapping(LightChannels& channels) {
    _light_mapping = channels;
}

template <typename T>
struct LightTimerValue {
    using Timer = espurna::timer::SystemTimer;
    using Duration = Timer::Duration;

    LightTimerValue() = delete;
    constexpr LightTimerValue(T defaultValue) :
        _defaultValue(defaultValue)
    {}

    explicit operator bool() const {
        return _value != _defaultValue;
    }

    void wait_set(Duration duration, T value) {
        _timer.once(
            duration,
            [this, value]() {
                _value = value;
            });
    }

    void reset() {
        _value = _defaultValue;
    }

    T get() {
        const auto value = _value;
        reset();
        return value;
    }

private:
    T _defaultValue;
    T _value;
    espurna::timer::SystemTimer _timer;
};

auto _light_save_delay = espurna::light::build::saveDelay();
bool _light_save { espurna::light::build::save() };

LightTimerValue<bool> _light_save_timer(false);

auto _light_report_delay = espurna::light::build::reportDelay();
std::forward_list<LightReportListener> _light_report;
LightTimerValue<int> _light_report_timer(0);

bool _light_has_cold_white = false;
bool _light_has_warm_white = false;
bool _light_has_color = false;

bool _light_use_color = false;
bool _light_use_rgb = false;
bool _light_use_white = false;
bool _light_use_cct = false;
bool _light_use_gamma = false;

bool _light_state = false;

struct LightBrightness {
    LightBrightness() = default;
    explicit LightBrightness(long value) :
        _value(clamp(value))
    {}

    LightBrightness& operator=(long value) {
        this->value(value);
        return *this;
    }

    long value() const {
        return _value;
    }

    void value(long value) {
        _value = clamp(value);
    }

    long percent() const {
        return (_value * 100l) / espurna::light::BrightnessMax;
    }

    void percent(long value) {
        const auto Fixed = std::clamp(value, 0l, 100l);
        const auto Ratio = espurna::light::BrightnessMax * Fixed;
        this->value(Ratio / 100l);
    }

    long operator()(long input) const {
        return (input * _value) / espurna::light::BrightnessMax;
    }

    String toString() const {
        return String(_value, 10);
    }

private:
    long clamp(long value) {
        return std::clamp(value,
            espurna::light::BrightnessMin,
            espurna::light::BrightnessMax);
    }

    long _value { espurna::light::BrightnessMax };
};

LightBrightness _light_brightness;

// Default to the mireds scale, similar to Philips Hue and old Home Assistant.
// * https://developers.meethue.com/documentation/core-concepts
// Note that HA 2022.11+ uses kelvin as the native color temperature unit
// * https://www.home-assistant.io/blog/2022/11/02/release-202211/#color-temperatures-in-kelvin

struct LightTemperature {
    static constexpr long MiredsKelvinScale { 1000000 };

    long cold() const {
        return _cold;
    }

    void cold(long value) {
        _cold = value;
    }

    long warm() const {
        return _warm;
    }

    void warm(long value) {
        _warm = value;
    }

    void range(espurna::light::TemperatureRange range) {
        _cold = range.cold();
        _warm = range.warm();
    }

    float factor() const {
        const auto Mireds = static_cast<float>(_value);
        const auto Cold = static_cast<float>(_cold);
        const auto Warm = static_cast<float>(_warm);
        return (Mireds - Cold) / (Warm - Cold);
    }

    espurna::light::TemperatureRange range() const {
        return {_cold, _warm};
    }

    espurna::light::Mireds mireds() const {
        return {_value};
    }

    void mireds(espurna::light::Mireds mireds) {
        _value = std::clamp(mireds.value, _cold, _warm);
    }

    espurna::light::Kelvin kelvin() const {
        return {MiredsKelvinScale / _value};
    }

    LightTemperature& operator=(espurna::light::Mireds mireds) {
        this->mireds(mireds);
        return *this;
    }

    LightTemperature& operator=(espurna::light::Kelvin kelvin) {
        *this = espurna::light::Mireds{
            .value = MiredsKelvinScale / kelvin.value
        };
        return *this;
    }

private:
    long _value { espurna::light::MiredsDefault };
    long _warm { espurna::light::MiredsWarm };
    long _cold { espurna::light::MiredsCold };
};

LightTemperature _light_temperature;

#if RELAY_SUPPORT
auto _light_state_relay_id = RelaysMax;
#endif

bool _light_state_changed = false;
LightStateListener _light_state_listener = nullptr;

void _lightProcessNoop(LightChannels&) {
}

using LightProcessInputValues = void(*)(LightChannels&);
LightProcessInputValues _light_process_input_values { _lightProcessNoop };

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
std::unique_ptr<my92xx> _my92xx;
#endif

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
std::unique_ptr<LightProvider> _light_provider;
#endif

} // namespace

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

namespace {

void _lightBrightnessPercent(long value) {
    _light_brightness.percent(value);
}

long _lightBrightnessPercent() {
    return _light_brightness.percent();
}

// After the channel value was updated through the API (i.e. through changing the `inputValue`),
// these functions are expected to be called. Which one is chosen is based on the current settings values.

// Basic brightness application; default when all other processing options are disabled

void _lightValuesWithBrightness(LightChannels& channels) {
    const auto Brightness = _light_brightness;
    for (auto& channel : channels) {
        channel.apply(Brightness);
    }
}

// Maintain compatibility with older versions, limit brightness application to the RGB when using 'color mode'.

void _lightValuesWithBrightnessExceptWhite(LightChannels& channels) {
    auto ptr = espurna::light::Pointers(channels);
    const auto Brightness = _light_brightness;

    (*ptr.red()).apply(Brightness);
    (*ptr.green()).apply(Brightness);
    (*ptr.blue()).apply(Brightness);

    if (ptr.warm()) {
        (*ptr.warm()).apply();
    }

    if (ptr.cold()) {
        (*ptr.cold()).apply();
    }
}

// Reset inputValue directly in the expression
// Ignores all previous values, should only be used at the beginning

struct LightResetInput {
    LightResetInput() = delete;
    explicit LightResetInput(long value) :
        _value(value)
    {}

    long operator()(long) const {
        return _value;
    }

    // 0.0 is the 'coldest', 1.0 is the 'warmest'
    static LightResetInput forWarm(float factor) {
        return LightResetInput(factor * espurna::light::ValueMax);
    }

    // opposite value of `forWarm` for the given factor
    static LightResetInput forCold(float factor) {
        return LightResetInput((1.0f - factor) * espurna::light::ValueMax);
    }

private:
    long _value;
};

// With `useCCT`, balance the value between Warm and Cold channels based on the current `mireds`.

struct LightScaledWhite {
    static auto constexpr Default = espurna::light::build::WhiteFactor;

    LightScaledWhite() = default;
    explicit LightScaledWhite(float factor) :
        _factor(factor)
    {}

    static LightScaledWhite with(float factor) {
        return LightScaledWhite{factor * Default};
    }

    long operator()(long input) const {
        return std::lround(static_cast<float>(input) * _factor);
    }

private:
    float _factor { Default };
};

void _lightValuesWithCct(LightChannels& channels) {
    const auto Brightness = _light_brightness;
    const auto CctFactor = _light_temperature.factor();
    const auto White = LightScaledWhite();

    auto ptr = espurna::light::Pointers(channels);
    (*ptr.warm()).apply(
        LightResetInput::forWarm(CctFactor), White, Brightness);
    (*ptr.cold()).apply(
        LightResetInput::forCold(CctFactor), White, Brightness);

    if (ptr.red() && ptr.green() && ptr.blue()) {
        (*ptr.red()).apply(
            LightResetInput{espurna::light::ValueMin});
        (*ptr.green()).apply(
            LightResetInput{espurna::light::ValueMin});
        (*ptr.blue()).apply(
            LightResetInput{espurna::light::ValueMin});
    }
}

// To handle both 4 and 5 channels, allow to 'adjust' internal factor calculation after construction
// When processing the channel values, this is the expected sequence:
// [250,150,0] -> [200,100,0,50] -> [250,125,0,63], factor is 1.25
//
// XXX: before 1.15.0:
// - factor for the example above is 1 b/c of integer division, meaning the sequence is instead:
// [250,150,0] -> [200,100,0,50] -> [200,100,0,50]
// - when modified, white channels(s) `inputValue` is always equal to the output `value`

struct LightRgbWithoutWhite {
    LightRgbWithoutWhite() = delete;
    explicit LightRgbWithoutWhite(espurna::light::Rgb rgb) :
        _common(makeCommon(rgb)),
        _factor(makeFactor(_common)),
        _luminance(makeLuminance(_common))
    {}

    explicit LightRgbWithoutWhite(const LightChannels& channels) :
        LightRgbWithoutWhite{makeRgb(channels)}
    {}

    long operator()(long input) const {
        return std::lround(static_cast<float>(input - _common.inputMin) * _factor);
    }

    template <typename... Args>
    void adjustOutput(Args&&... args) {
        _common.outputMax = std::max({
            _common.outputMax,
            std::forward<Args>(args)...
        });
        _factor = makeFactor(_common);
        _luminance = makeLuminance(_common);
    }

    long inputMin() const {
        return _common.inputMin;
    }

    float factor() const {
        return _factor;
    }

    float luminance() const {
        return _luminance;
    }

private:
    struct Common {
        long inputMin;
        long inputMax;
        long outputMax;
    };

    static float makeLuminance(Common common) {
        const auto raw = (common.inputMin + common.inputMax) / 2;
        return static_cast<float>(raw) / espurna::light::ValueMax;
    }

    static float makeFactor(Common common) {
        const auto inputMax = static_cast<float>(common.inputMax);
        const auto outputMax = static_cast<float>(common.outputMax);
        return (outputMax > 0.0f)
            ? (inputMax / outputMax)
            : 0.0f;
    }

    static espurna::light::Rgb makeRgb(const LightChannels& channels) {
        return {
            channels[0].inputValue,
            channels[1].inputValue,
            channels[2].inputValue,
        };
    }

    static Common makeCommon(espurna::light::Rgb rgb) {
        Common out;
        out.inputMax = std::max({rgb.red(), rgb.green(), rgb.blue()});
        out.inputMin = std::min({rgb.red(), rgb.green(), rgb.blue()});
        out.outputMax = std::max({
            rgb.red() - out.inputMin,
            rgb.green() - out.inputMin,
            rgb.blue() - out.inputMin
        });

        return out;
    }

    Common _common;
    float _factor;
    float _luminance;
};

// When `useWhite` is enabled, warm white channel is 'detached' from the processing and its value depends on the input RGB.
// Common calculation is to subtract 'white value' from the RGB based on the minimum channel value, e.g. [250, 150, 50] becomes [200, 100, 0, 50]
//
// General case when `useCCT` is disabled, but there are 4 channels.
// Keeps 5th channel as-is, without applying the brightness scale or resetting the value to 0

void _lightValuesWithRgbWhite(LightChannels& channels) {
    auto rgb = LightRgbWithoutWhite{channels};
    rgb.adjustOutput(rgb.inputMin());

    const auto Brightness = _light_brightness;

    auto ptr = espurna::light::Pointers(channels);
    (*ptr.red()).apply(rgb, Brightness);
    (*ptr.green()).apply(rgb, Brightness);
    (*ptr.blue()).apply(rgb, Brightness);

    (*ptr.warm()).apply(
        LightResetInput{rgb.inputMin()},
        LightScaledWhite::with(rgb.factor()),
        Brightness);

    if (ptr.cold()) {
        (*ptr.cold()).apply();
    }
}

// Kelvin to RGB approximation algorithm by Tanner Helland
// * https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html
// Original code for RGB lights from AiLight library by Sacha Telgenhof (@
// * https://github.com/stelgenhof/AiLight/blob/develop/lib/AiLight/AiLight.cpp

// Instead of the above, use `mireds` value as a range for warm and cold channels, based on the calculated rgb common values
// Every value is also scaled by `brightness` after applying all of the previous steps
// Notice that we completely ignore inputs and reset them to either kelvin'ized or hardcoded ValueMin or ValueMax

// Heavily depends on the used temperature range; by default (153...500), we stay on the 'warm side'
// of the scale and effectively never enable blue. Setting cold mireds to 100 will use the whole range.

espurna::light::Rgb _lightKelvinRgb(espurna::light::Kelvin kelvin) {
    kelvin.value /= 100;
    const auto red = ((kelvin.value <= 66)
        ? espurna::light::ValueMax
        : std::lround(329.698727446 * fs_pow(static_cast<double>(kelvin.value - 60), -0.1332047592)));
    const auto green = ((kelvin.value <= 66)
        ? std::lround(99.4708025861 * fs_log(kelvin.value) - 161.1195681661)
        : std::lround(288.1221695283 * fs_pow(static_cast<double>(kelvin.value), -0.0755148492)));
    const auto blue = ((kelvin.value >= 66)
        ? espurna::light::ValueMax
        : ((kelvin.value <= 19)
            ? espurna::light::ValueMin
            : std::lround(138.5177312231 * fs_log(static_cast<double>(kelvin.value - 10)) - 305.0447927307)));

    return {red, green, blue};
}

void _lightValuesWithRgbCct(LightChannels& channels) {
    const auto Temperature = _light_temperature;

    const auto Kelvin = Temperature.kelvin();
    const auto RgbFromKelvin = _lightKelvinRgb(Kelvin);

    auto rgb = LightRgbWithoutWhite{RgbFromKelvin};
    rgb.adjustOutput(rgb.inputMin());

    const auto Brightness = _light_brightness;
    auto ptr = espurna::light::Pointers(channels);

    (*ptr.red()).apply(
        LightResetInput{RgbFromKelvin.red()},
        rgb, Brightness);
    (*ptr.green()).apply(
        LightResetInput{RgbFromKelvin.green()},
        rgb, Brightness);
    (*ptr.blue()).apply(
        LightResetInput{RgbFromKelvin.blue()},
        rgb, Brightness);

    const auto White = LightScaledWhite(rgb.factor());
    if (ptr.warm() || ptr.cold()) {
        if (ptr.warm()) {
            (*ptr.warm()).apply(White, Brightness);
        }

        if (ptr.cold()) {
            (*ptr.cold()).apply(White, Brightness);
        }
    }
}

// UI hints about channel distribution

char _lightTag(size_t channels, size_t index) {
    constexpr size_t Columns { 5ul };
    constexpr size_t Rows { 5ul };

    auto row = channels - 1ul;
    if (row < Rows) {
        constexpr char tags[Rows][Columns] = {
            {'W',   0,   0,   0,   0},
            {'W', 'C',   0,   0,   0},
            {'R', 'G', 'B',   0,   0},
            {'R', 'G', 'B', 'W',   0},
            {'R', 'G', 'B', 'W', 'C'},
        };

        return tags[row][index];
    }

    return 0;
}

const char* _lightDesc(size_t channels, size_t index) {
    const char* out = PSTR("UNKNOWN");

    switch (_lightTag(channels, index)) {
    case 'W':
        out = PSTR("WARM WHITE");
        break;
    case 'C':
        out = PSTR("COLD WHITE");
        break;
    case 'R':
        out = PSTR("RED");
        break;
    case 'G':
        out = PSTR("GREEN");
        break;
    case 'B':
        out = PSTR("BLUE");
        break;
    }

    return out;
}

} // namespace

// -----------------------------------------------------------------------------
// Input Values
// -----------------------------------------------------------------------------

namespace {

void _lightFromHexPayload(espurna::StringView payload) {
    const bool JustRgb { (payload.length() == 6) };
    const bool WithBrightness { (payload.length() == 8) };
    if (!JustRgb && !WithBrightness) {
        return;
    }

    uint8_t values[4] {0, 0, 0, 0};
    if (hexDecode(payload.begin(), payload.length(), values, sizeof(values))) {
        _light_mapping.red(values[0]);
        _light_mapping.green(values[1]);
        _light_mapping.blue(values[2]);
        if (WithBrightness) {
            lightBrightness(values[3]);
        }
    }
}

template <typename T>
const char* _lightForEachToken(espurna::StringView payload, char sep, T&& callback) {
    const auto begin = payload.begin();
    const auto end = payload.end();

    auto it = begin;
    for (auto last = it; it != end; ++it) {
        last = it;
        it = std::find(it, payload.end(), ',');
        if (!callback(espurna::StringView(last, it))) {
            break;
        }
        if (it == end) {
            break;
        }
    }

    return it;
}

void _lightFromCommaSeparatedPayload(espurna::StringView payload, decltype(_light_channels.end()) end) {
    auto it = _light_channels.begin();
    if (it == end) {
        return;
    }

    // every channel value is separated by a comma
    _lightForEachToken(payload, ',',
        [&](espurna::StringView token) {
            if (it != end) {
                const auto result = parseUnsigned(token, 10);
                if (result.ok) {
                    (*it) = result.value;
                    ++it;
                    return true;
                }
            }

            return false;
        });

    // fill the rest with zeroes
    while (it != end) {
        (*it) = 0;
        ++it;
    }
}

void _lightFromCommaSeparatedPayload(espurna::StringView payload) {
    _lightFromCommaSeparatedPayload(payload, _light_channels.end());
}

void _lightFromRgbPayload(espurna::StringView payload) {
    if (!_light_has_color) {
        return;
    }

    if (!payload.length() || (payload[0] == '\0')) {
        return;
    }

    // HEX value is always prefixed, like CSS
    // - #AABBCC
    // Extra byte is interpreted like RGB + brightness
    // - #AABBCCDD
    if (payload[0] == '#') {
        _lightFromHexPayload(
            espurna::StringView(payload.begin() + 1, payload.end()));
        return;
    }

    // Otherwise, assume comma-separated decimal values
    _lightFromCommaSeparatedPayload(payload, _light_channels.begin() + 3);
}

espurna::light::Hsv _lightHsvFromPayload(espurna::StringView payload) {
    espurna::light::Hsv::Array values;
    auto it = std::begin(values);

    // HSV string is expected to be "H,S,V", where:
    // - H [0...360]
    // - S [0...100]
    // - V [0...100]
    const auto end = std::end(values);
    const auto parsed = _lightForEachToken(payload, ',',
        [&](espurna::StringView token) {
            if (it != end) {
                const auto result = parseUnsigned(token, 10);
                if (result.ok) {
                    (*it) = result.value;
                    ++it;
                    return true;
                }
            }

            return false;
        });

    // discard partial or uneven payloads
    espurna::light::Hsv out;
    if ((parsed != payload.end()) || (it != end)) {
        return out;
    }

    // values are expected to be 'clamped' either in the
    // following call or in ctor of the helper object
    out = espurna::light::Hsv(values);
    return out;
}

void _lightFromHsvPayload(espurna::StringView payload) {
    if (!_light_has_color || !payload.length()) {
        return;
    }

    lightHsv(_lightHsvFromPayload(payload));
}

template <typename T>
void _lightTemperature(T value) {
    _light_temperature = value;
}

} // namespace

// -----------------------------------------------------------------------------
// Output Values
// -----------------------------------------------------------------------------

namespace {

espurna::light::Rgb _lightToTargetRgb() {
    return {
        _light_mapping.red(),
        _light_mapping.green(),
        _light_mapping.blue()};
}

espurna::light::Rgb _lightToInputRgb() {
    const auto& ptr = _light_mapping.pointers();

    long values[] {0, 0, 0};
    if (ptr.red() && ptr.green() && ptr.blue()) {
        values[0] = ptr.red()->inputValue;
        values[1] = ptr.green()->inputValue;
        values[2] = ptr.blue()->inputValue;
    }

    return {values[0], values[1], values[2]};
}

// instead of falling back to scale, use channels as reference in simple modes

String _lightRgbHexPayload(espurna::light::Rgb rgb) {
    static_assert(espurna::light::Rgb::Min == 0, "");
    static_assert(espurna::light::Rgb::Max == 255, "");

    uint8_t values[] {
        static_cast<uint8_t>(rgb.red()),
        static_cast<uint8_t>(rgb.green()),
        static_cast<uint8_t>(rgb.blue())};

    String out;

    char buffer[8] {0};
    if (hexEncode(values, sizeof(values), buffer, sizeof(buffer))) {
        out.reserve(8);
        out.concat('#');
        out.concat(&buffer[0], sizeof(buffer) - 1);
    }

    return out;
}

String _lightRgbPayload(espurna::light::Rgb rgb) {
    String out;
    out.reserve(12);

    out += rgb.red();
    out += ',';

    out += rgb.green();
    out += ',';

    out += rgb.blue();

    return out;
}

String _lightRgbPayload() {
    return _lightRgbPayload(_lightToInputRgb());
}

// HSV to RGB transformation
//
// INPUT: [0,100,57]
// IS: [145,0,0]
// SHOULD: [255,0,0]

espurna::light::Rgb _lightRgb(espurna::light::Hsv hsv) {
    constexpr auto ValueMin = static_cast<double>(espurna::light::ValueMin);
    double r { ValueMin };
    double g { ValueMin };
    double b { ValueMin };

    static constexpr auto Scale = 100.0;
    auto v = static_cast<double>(hsv.value()) / Scale;

    if (hsv.saturation()) {
        auto h = hsv.hue();
        if (h < 0) {
            h = 0;
        } else if (h >= 360) {
            h = 359;
        }

        auto s = static_cast<double>(hsv.saturation()) / Scale;

        auto c = v * s;

        auto hmod2 = fs_fmod(static_cast<double>(h) / 60.0, 2.0);
        auto x = c * (1.0 - std::abs(hmod2 - 1.0));

        auto m = v - c;

        if ((0 <= h) && (h < 60)) {
            r = c;
            g = x;
        } else if ((60 <= h) && (h < 120)) {
            r = x;
            g = c;
        } else if ((120 <= h) && (h < 180)) {
            g = c;
            b = x;
        } else if ((180 <= h) && (h < 240)) {
            g = x;
            b = c;
        } else if ((240 <= h) && (h < 300)) {
            r = x;
            b = c;
        } else if ((300 <= h) && (h < 360)) {
            r = c;
            b = x;
        }

        constexpr auto ValueMax = static_cast<double>(espurna::light::ValueMax);
        r = (r + m) * ValueMax;
        g = (g + m) * ValueMax;
        b = (b + m) * ValueMax;
    }

    return {
        static_cast<long>(std::nearbyint(r)),
        static_cast<long>(std::nearbyint(g)),
        static_cast<long>(std::nearbyint(b))};
}

espurna::light::Hsv _lightHsv(espurna::light::Rgb rgb) {
    using namespace espurna::light;

    const auto r = static_cast<double>(rgb.red()) / ValueMax;
    const auto g = static_cast<double>(rgb.green()) / ValueMax;
    const auto b = static_cast<double>(rgb.blue()) / ValueMax;

    const auto max = std::max({r, g, b});
    const auto min = std::min({r, g, b});

    auto v = max;

    if (min != max) {
        auto delta = max - min;

        auto s = delta / max;
        auto rc = (max - r) / delta;
        auto gc = (max - g) / delta;
        auto bc = (max - b) / delta;

        double h;
        if (r == max) {
            h = bc - gc;
        } else if (g == max) {
            h = 2.0 + rc - bc;
        } else {
            h = 4.0 + gc - rc;
        }

        h = fs_fmod((h / 6.0), 1.0);
        if (h < 0.0) {
            h = 1.0 + h;
        }

        return Hsv(
            std::lround(h * 360.0),
            std::lround(s * 100.0),
            std::lround(v * 100.0));
    }

    return Hsv(Hsv::HueMin, Hsv::SaturationMin, v);
}

String _lightHsvPayload(espurna::light::Hsv hsv) {
    String out;

    auto values = hsv.asArray();
    for (const auto& value : values) {
        if (out.length()) {
            out += ',';
        }
        out += value;
    }

    return out;
}

String _lightHsvPayload(espurna::light::Rgb rgb) {
    return _lightHsvPayload(_lightHsv(rgb));
}

String _lightHsvPayload() {
    return _lightHsvPayload(_lightToTargetRgb());
}

String _lightGroupPayload() {
    const auto Channels = _light_channels.size();

    String result;
    result.reserve(4 * Channels);

    for (const auto& channel : _light_channels) {
        if (result.length()) {
            result += ',';
        }
        result += String(channel.inputValue);
    }

    return result;
}

// Basic value adjustments. Expression can be:
// +offset, -offset or the new value

long _lightAdjustValue(long value, espurna::StringView operation) {
    const auto dot = std::find(operation.begin(), operation.end(), '.');
    if (dot != operation.end()) {
        operation = espurna::StringView(operation.begin(), dot);
    }
    
    if (operation.length()) {
        switch (operation[0]) {
        case '+':
        case '-':
        {
            const long multiplier = (operation[0] == '-') ? -1 : 1;
            operation = espurna::StringView(
                operation.begin() + 1, operation.end());

            const auto result = parseUnsigned(operation, 10);
            if (result.ok && result.value < std::numeric_limits<long>::max()) {
                return value + (static_cast<long>(result.value) * multiplier);
            }
            break;
        }

        default:
        {
            const auto result = parseUnsigned(operation, 10);
            if (result.ok && result.value < std::numeric_limits<long>::max()) {
                return result.value;
            }
        }

        }
    }

    return value;
}

void _lightAdjustBrightness(espurna::StringView payload) {
    lightBrightness(_lightAdjustValue(_light_brightness.value(), payload));
}

void _lightAdjustChannel(LightChannel& channel, espurna::StringView payload) {
    channel = _lightAdjustValue(channel.inputValue, payload);
}

void _lightAdjustChannel(size_t id, espurna::StringView payload) {
    if (id < _light_channels.size()) {
        _lightAdjustChannel(_light_channels[id], payload);
    }
}

void _lightAdjustKelvin(espurna::StringView payload) {
    const auto kelvin = _light_temperature.kelvin();
    const auto adjusted = _lightAdjustValue(kelvin.value, payload);
    _lightTemperature(espurna::light::Kelvin{
        .value = adjusted,
    });
}

void _lightAdjustMireds(espurna::StringView payload) {
    const auto mireds = _light_temperature.mireds();
    const auto adjusted = _lightAdjustValue(mireds.value, payload);
    _lightTemperature(espurna::light::Mireds{
        .value = adjusted,
    });
}

} // namespace

// -----------------------------------------------------------------------------
// PROVIDER
// -----------------------------------------------------------------------------

namespace {

// Gamma Correction lookup table (8 bit, ~2.2)
// TODO: input value modifier, instead of a transition-only thing?
// TODO: calculate on the fly instead of limiting this to an 8bit value?

static constexpr long LightGammaMin { 0 };
static constexpr long LightGammaMax { 255 };

long _lightGammaValue(size_t index) {
    static const std::array<uint8_t, 256> Gamma PROGMEM {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,
        3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,
        6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  11,  11,  11,
        12,  12,  13,  13,  14,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,
        19,  20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,
        29,  30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,
        41,  42,  43,  43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  53,  54,
        55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  71,
        72,  73,  74,  75,  76,  77,  78,  80,  81,  82,  83,  84,  86,  87,  88,  89,
        91,  92,  93,  94,  96,  97,  98,  100, 101, 102, 104, 105, 106, 108, 109, 110,
        112, 113, 115, 116, 118, 119, 121, 122, 123, 125, 126, 128, 130, 131, 133, 134,
        136, 137, 139, 140, 142, 144, 145, 147, 149, 150, 152, 154, 155, 157, 159, 160,
        162, 164, 166, 167, 169, 171, 173, 175, 176, 178, 180, 182, 184, 186, 187, 189,
        191, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
        223, 225, 227, 229, 231, 233, 235, 238, 240, 242, 244, 246, 248, 251, 253, 255
    };

    if (index < Gamma.size()) {
        return pgm_read_byte(&Gamma[index]);
    }

    return 0;
}

long _lightGammaMap(long value) {
    static_assert(espurna::light::ValueMin >= 0, "");
    static_assert(espurna::light::ValueMax >= 0, "");

    constexpr auto Divisor = (espurna::light::ValueMax - espurna::light::ValueMin);
    if (Divisor != 0l) {
        const long Scaled {
            (value - espurna::light::ValueMin) * (LightGammaMax - LightGammaMin) / Divisor + LightGammaMin };
        return _lightGammaValue(static_cast<size_t>(Scaled));
    }

    return espurna::light::ValueMin;
}

class LightTransitionHandler {
public:
    // internal calculations are done in floats, so hard-limit target & step time to a certain value
    // that can be representend precisely when casting milliseconds times back and forth
    static constexpr espurna::duration::Milliseconds TimeMin { 10 };
    static constexpr espurna::duration::Milliseconds TimeMax { 1ul << 24ul };

    struct Transition {
        float& value;
        long target;
        float step;
        size_t count;
    };

    using Transitions = std::vector<Transition>;

    LightTransitionHandler() = delete;

    LightTransitionHandler(LightChannels& channels, LightTransition transition, bool state) :
        _transition(clamp(transition)),
        _state(state)
    {
        prepare(channels, transition, state);
    }

    template <typename StateFunc, typename ValueFunc, typename UpdateFunc>
    bool run(StateFunc&& state, ValueFunc&& value, UpdateFunc&& update) {
        bool next { false };

        if (!_state_notified && _state) {
            _state_notified = true;
            state(_state);
        }

        for (size_t index = 0; index < _prepared.size(); ++index) {
            auto& transition = _prepared[index];
            if (!transition.count) {
                continue;
            }

            if (--transition.count) {
                transition.value += transition.step;
                next = true;
            } else {
                transition.value = transition.target;
            }

            value(index, transition.value);
        }

        if (!_state_notified && !next && !_state) {
            _state_notified = true;
            state(_state);
        }

        update();

        return next;
    }

    const Transitions& prepared() const {
        return _prepared;
    }

    bool state() const {
        return _state;
    }

    espurna::duration::Milliseconds time() const {
        return _transition.time;
    }

    espurna::duration::Milliseconds step() const {
        return _transition.step;
    }

private:
    void minimalTime() {
        _transition.time = TimeMin;
        _transition.step = TimeMin;
    }

    void prepare(LightChannels& channels, LightTransition transition, bool state) {
        // generate a single transitions list for all the channels that had changed
        // after that, provider loop will run() the list and assign intermediate target value(s)
        bool delayed { false };
        for (auto& channel : channels) {
            if (prepare(channel, transition, state)) {
                delayed = true;
            }
        }

        // target values are already assigned, next provider loop will apply them
        if (!delayed) {
            minimalTime();
        }
    }

    bool prepare(LightChannel& channel, const LightTransition& transition, bool state) {
        long target = (state && channel.state)
            ? channel.value
            : espurna::light::ValueMin;

        channel.target = target;
        if (channel.gamma) {
            target = _lightGammaMap(target);
        }

        if (channel.inverse) {
            target = espurna::light::ValueMax - target;
        }

        const float Diff { static_cast<float>(target) - channel.current };
        if (!isImmediate(transition, Diff)) {
            pushGradual(transition, channel.current, target, Diff);
            return true;
        }

        pushImmediate(channel.current, target, Diff);
        return false;
    }

    void push(float& current, long target, float diff, size_t count) {
        _prepared.push_back(
            Transition{
                .value = current,
                .target = target,
                .step = diff,
                .count = count,
            });
    }

    void pushImmediate(float& current, long target, float diff) {
        push(current, target, diff, 1);
    }

    void pushGradual(const LightTransition& transition, float& current, long target, float diff) {
        const auto TotalTime = static_cast<float>(transition.time.count());
        const auto StepTime = static_cast<float>(transition.step.count());

        constexpr float BaseStep { 1.0f };
        const float Diff { std::abs(diff) };
        const float Every { TotalTime / Diff };

        float step { (diff > 0.0f) ? BaseStep : -BaseStep };
        if (Every < StepTime) {
            step *= (StepTime / Every);
        }

        const float Count { std::floor(Diff / std::abs(step)) };
        push(current, target, step, static_cast<size_t>(Count));
    }

    static bool isImmediate(const LightTransition& transition, float diff) {
        return !transition.time.count()
            || (transition.step >= transition.time)
            || (std::abs(diff) <= std::numeric_limits<float>::epsilon());
    }

    static LightTransition clamp(LightTransition value) {
        LightTransition out;
        out.time = std::min(value.time, TimeMax);
        out.step = std::min(value.step, TimeMax);
        return out;
    }

    Transitions _prepared;
    bool _state_notified { false };

    LightTransition _transition;
    bool _state;
};

constexpr espurna::duration::Milliseconds LightTransitionHandler::TimeMin;
constexpr espurna::duration::Milliseconds LightTransitionHandler::TimeMax;

struct LightUpdate {
    LightTransition transition;
    int report { 0 };
    bool save { false };
};

struct LightUpdateHandler {
    LightUpdateHandler() = default;
    LightUpdateHandler(const LightUpdateHandler&) = delete;
    LightUpdateHandler(LightUpdateHandler&&) = delete;

    LightUpdateHandler& operator=(const LightUpdateHandler&) = delete;
    LightUpdateHandler& operator=(LightUpdateHandler&&) = delete;

    // TODO: (esp8266) there is only a single thread, and explicit context switch via yield()
    //       callback() below is allowed to yield() and possibly reset the values, but we already have a copy
    // TODO: (esp32?) set() and run() need locking, in case there are multiple threads *and* set() may be called outside of the main one

    explicit operator bool() const {
        return _run;
    }

    void set(LightTransition transition, int report, bool save) {
        _update.transition = transition;
        _update.report = report;
        _update.save = save;
        _run = true;
    }

    void cancel() {
        _run = false;
    }

    template <typename T>
    void run(T&& callback) {
        if (_run) {
            _run = false;
            LightUpdate update{_update};
            callback(update.transition, update.report, update.save);
        }
    }

private:
    LightUpdate _update;
    bool _run { false };
};

struct LightSequenceHandler {
    LightSequenceHandler& operator=(LightSequenceCallbacks&& callbacks) {
        _callbacks = std::move(callbacks);
        return *this;
    }

    void run() {
        if (!_callbacks.empty()) {
            auto callback = std::move(_callbacks.front());
            _callbacks.pop_front();
            callback();
        }
    }

    void clear() {
        _callbacks.clear();
    }

private:
    LightSequenceCallbacks _callbacks;
};

struct LightProviderHandler {
    using Timer = espurna::timer::SystemTimer;
    using Duration = Timer::Duration;

    LightProviderHandler() = default;

    explicit operator bool() const {
        return _ready;
    }

    void stop() {
        _ready = false;
        _timer.stop();
    }

    void reset() {
        _ready = false;
    }

    void start(Duration duration) {
        _ready = false;
        _timer.repeat(
            duration,
            [&]() {
                _ready = true;
            });
    }

private:
    Timer _timer;
    bool _ready { false };
};

LightUpdateHandler _light_update;
LightProviderHandler _light_provider_update;

LightSequenceHandler _light_sequence;
std::unique_ptr<LightTransitionHandler> _light_transition;

auto _light_transition_time = espurna::light::build::transitionTime();
auto _light_transition_step = espurna::light::build::transitionStep();
bool _light_use_transitions = false;

static_assert((espurna::light::ValueMax - espurna::light::ValueMin) != 0, "");

template <typename T>
constexpr T _lightValueMap(long value, T min, T max) {
    return (value - espurna::light::ValueMin) * (max - min) / (espurna::light::ValueMax - espurna::light::ValueMin) + min;
}

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

uint32_t _light_pwm_min;
uint32_t _light_pwm_max;

// since we expect 0 duty on OFF state, no need to do anything else from here
void _lightProviderHandleState(bool) {
}

// Automatically scale from our value to the internal one used by the PWM
// Currently, both u32 and float variants are almost the same precision
// Slight difference would be the amount of generated code; one variant
// needs to call float division, the other one is to simply truncate it
// using two external values which are then used in integer divison
// TODO: actually check call speed?
// TODO: any difference between __fixsfsi and lround?
void _lightProviderHandleValue(size_t channel, float value) {
    pwmDuty(channel, _lightValueMap(value, _light_pwm_min, _light_pwm_max));
}

void _lightProviderHandleUpdate() {
    pwmUpdate();
}

#elif LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX

constexpr unsigned int _lightMy92xxValueShift(my92xx_cmd_bit_width_t width) {
    return (width == MY92XX_CMD_BIT_WIDTH_16) ? 16 :
        (width == MY92XX_CMD_BIT_WIDTH_14) ? 14 :
        (width == MY92XX_CMD_BIT_WIDTH_12) ? 12 :
        (width == MY92XX_CMD_BIT_WIDTH_8) ? 8 : 8;
}

constexpr unsigned int _lightMy92xxValueMax(my92xx_cmd_bit_width_t width) {
    return (1 << _lightMy92xxValueShift(width)) - 1;
}

constexpr unsigned int _lightMy92xxValueMax(my92xx_cmd_t command) {
    return _lightMy92xxValueMax(command.bit_width);
}

unsigned char _light_my92xx_channel_map[espurna::light::ChannelsMax] = {};

constexpr unsigned int _my92xx_value_min = 0;
constexpr unsigned int _my92xx_value_max =
        _lightMy92xxValueMax(espurna::light::build::my92xxCommand());

void _lightProviderHandleValue(size_t channel, float value) {
    _my92xx->setChannel(
        _light_my92xx_channel_map[channel],
        _lightValueMap(value, _my92xx_value_min, _my92xx_value_max));
}

void _lightProviderHandleUpdate() {
    _my92xx->update();
}

void _lightProviderHandleState(bool state) {
    _my92xx->setState(state);
}

#elif LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM

void _lightProviderHandleState(bool state) {
    _light_provider->state(state);
}

void _lightProviderHandleValue(size_t channel, float value) {
    _light_provider->channel(channel, value);
}

void _lightProviderHandleUpdate() {
    _light_provider->update();
}

#endif

void _lightProviderUpdate() {
    if (!_light_provider_update) {
        return;
    }

    if (!_light_transition) {
        _light_provider_update.stop();
        return;
    }

    auto next = _light_transition->run(
        _lightProviderHandleState,
        _lightProviderHandleValue,
        _lightProviderHandleUpdate);

    if (!next) {
        _light_transition.reset(nullptr);
        _light_provider_update.stop();
    }

    _light_provider_update.reset();
}

} // namespace

// -----------------------------------------------------------------------------
// PERSISTANCE
// -----------------------------------------------------------------------------

// Layout should match the old union:
//
// union light_rtcmem_t {
//     struct {
//         uint8_t channels[espurna::light::ChannelsMax];
//         uint8_t brightness;
//         uint16_t mired;
//     } __attribute__((packed)) packed;
//     uint64_t value;
// };

using LightValues = std::array<long, espurna::light::ChannelsMax>;

struct LightRtcmem {
    //   1 2 3 4 5 6 7 8
    // [ m m b c c c c c ]
    //         ^ ^ ^ ^ ^   channels
    //       ^ ~ ~ ~ ~ ~   brightness
    //   ^ ^ ~ ~ ~ ~ ~ ~   mireds
    //
    // As seen in the rtcmem dump:
    //   `ddccbbaa 112233ee`
    // Where:
    // - 1122 are mireds
    //   [153...500]
    // - 33 is brightness
    //   [0...255]
    // - aabbccddee are channels (from 0 to 5 respectively)
    //   [0...255]
    //
    // Prefer to use u64 value for {de,se}rialization instead of a struct.

    static_assert(espurna::light::ChannelsMax == 5, "");
    static_assert(espurna::light::ValueMin >= 0, "");
    static_assert(espurna::light::ValueMax <= 255, "");

    LightRtcmem() = default;

    explicit LightRtcmem(uint64_t value) {
        _mireds.value = (value >> (8ull * 6ull)) & 0xffffull;
        _brightness = (value >> (8ull * 5ull)) & 0xffull;

        _values[4] = ((value >> (8ull * 4ull)) & 0xffull);
        _values[3] = ((value >> (8ull * 3ull)) & 0xffull);
        _values[2] = ((value >> (8ull * 2ull)) & 0xffull);
        _values[1] = ((value >> (8ull * 1ull)) & 0xffull);
        _values[0] = ((value & 0xffull));
    }

    LightRtcmem(const LightValues& values, long brightness, espurna::light::Mireds mireds) :
        _values(values),
        _brightness(brightness),
        _mireds(mireds)
    {}

    uint64_t serialize() const {
        return ((static_cast<uint64_t>(_mireds.value) & 0xffffull) << (8ull * 6ull))
            | ((static_cast<uint64_t>(_brightness) & 0xffull) << (8ull * 5ull))
            | (static_cast<uint64_t>(_values[4] & 0xffl) << (8ull * 4ull))
            | (static_cast<uint64_t>(_values[3] & 0xffl) << (8ull * 3ull))
            | (static_cast<uint64_t>(_values[2] & 0xffl) << (8ull * 2ull))
            | (static_cast<uint64_t>(_values[1] & 0xffl) << (8ull * 1ull))
            | (static_cast<uint64_t>(_values[0] & 0xffl));
    }

    static LightValues defaultValues() {
        LightValues out;
        out.fill(espurna::light::ValueMin);
        return out;
    }

    const LightValues& values() const {
        return _values;
    }

    long brightness() const {
        return _brightness;
    }

    espurna::light::Mireds mireds() const {
        return _mireds;
    }

private:
    LightValues _values = defaultValues();
    long _brightness { espurna::light::BrightnessMax };
    espurna::light::Mireds _mireds { espurna::light::MiredsDefault };
};

bool lightSave() {
    return _light_save;
}

void lightSave(bool save) {
    _light_save = save;
}

namespace {

void _lightSaveRtcmem() {
    auto values = LightRtcmem::defaultValues();
    for (size_t channel = 0; channel < _light_channels.size(); ++channel) {
        values[channel] = _light_channels[channel].inputValue;
    }

    LightRtcmem light(values,
        _light_brightness.value(),
        _light_temperature.mireds());
    Rtcmem->light = light.serialize();
}

void _lightRestoreRtcmem() {
    uint64_t value = Rtcmem->light;
    LightRtcmem light(value);

    const auto& values = light.values();
    for (size_t channel = 0; channel < _light_channels.size(); ++channel) {
        _light_channels[channel] = values[channel];
    }

    lightTemperature(light.mireds());
    lightBrightness(light.brightness());
}

void _lightSaveSettings() {
    if (!_light_save) {
        return;
    }

    for (size_t channel = 0; channel < _light_channels.size(); ++channel) {
        espurna::light::settings::value(
            channel, _light_channels[channel].inputValue);
    }

    espurna::light::settings::brightness(_light_brightness.value());
    espurna::light::settings::mireds(_light_temperature.mireds());

    saveSettings();
}

void _lightRestoreSettings() {
    for (size_t channel = 0; channel < _light_channels.size(); ++channel) {
        _light_channels[channel] = espurna::light::settings::value(channel);
    }

    _light_temperature = espurna::light::settings::mireds();
    lightBrightness(espurna::light::settings::brightness());
}

bool _lightParsePayload(espurna::StringView payload) {
    switch (rpcParsePayload(payload)) {
    case PayloadStatus::On:
        lightState(true);
        break;
    case PayloadStatus::Off:
        lightState(false);
        break;
    case PayloadStatus::Toggle:
        lightState(!_light_state);
        break;
    case PayloadStatus::Unknown:
        return false;
    }

    return true;
}

bool _lightTryParseChannel(espurna::StringView value, size_t& id) {
    const auto channels = _light_channels.size();
    if (std::find(value.begin(), value.end(), '/') != value.end()) {
        return tryParseIdPath(value, channels, id);
    }

    return tryParseId(value, channels, id);
}

} // namespace

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

namespace {

bool _lightApiTransition(espurna::StringView payload) {
    const auto result = parseUnsigned(payload, 10);
    if (result.ok) {
        lightTransition(
                espurna::duration::Milliseconds(result.value),
                _light_transition_step);
        return true;
    }

    return false;
}

int _lightMqttReportMask() {
    return espurna::light::Report::Default & ~(mqttForward() ? espurna::light::Report::None : espurna::light::Report::Mqtt);
}

int _lightMqttReportGroupMask() {
    return _lightMqttReportMask() & ~espurna::light::Report::MqttGroup;
}

void _lightUpdateFromMqtt(LightTransition transition) {
    lightUpdate(transition, _lightMqttReportMask(), _light_save);
}

void _lightUpdateFromMqtt() {
    _lightUpdateFromMqtt(lightTransition());
}

void _lightUpdateFromMqttGroup() {
    lightUpdate(lightTransition(), _lightMqttReportGroupMask(), _light_save);
}

#if MQTT_SUPPORT

// TODO: implement per-module heartbeat mask? e.g. to exclude unwanted topics based on preference, not settings

bool _lightMqttHeartbeat(espurna::heartbeat::Mask mask) {
    if (mask & espurna::heartbeat::Report::Light) {
        lightMQTT();
    }

    return mqttConnected();
}

void _lightMqttCallback(unsigned int type, espurna::StringView topic, espurna::StringView payload) {
    String mqtt_group_color = espurna::light::settings::mqttGroup();

    if (type == MQTT_CONNECT_EVENT) {

        mqttSubscribe(MQTT_TOPIC_TRANSITION);

        mqttSubscribe(MQTT_TOPIC_CHANNEL "/+");
        mqttSubscribe(MQTT_TOPIC_BRIGHTNESS);
        mqttSubscribe(MQTT_TOPIC_LIGHT);

        if (_light_has_color) {
            mqttSubscribe(MQTT_TOPIC_COLOR_RGB);
            mqttSubscribe(MQTT_TOPIC_COLOR_HEX);
            mqttSubscribe(MQTT_TOPIC_COLOR_HSV);
        }

        if (_light_has_color || _light_has_cold_white || _light_has_warm_white) {
            mqttSubscribe(MQTT_TOPIC_MIRED);
            mqttSubscribe(MQTT_TOPIC_KELVIN);
        }

        if (mqtt_group_color.length() > 0) {
            mqttSubscribeRaw(mqtt_group_color.c_str());
        }
    }

    if (type == MQTT_MESSAGE_EVENT) {
        // Group color
        if ((mqtt_group_color.length() > 0) && (topic == mqtt_group_color)) {
            _lightFromCommaSeparatedPayload(payload);
            _lightUpdateFromMqttGroup();
            return;
        }

        // Match topic
        auto t = mqttMagnitude(topic);

        // Color temperature in mireds
        if (t.equals(MQTT_TOPIC_MIRED)) {
            _lightAdjustMireds(payload);
            _lightUpdateFromMqtt();
            return;
        }

        // Color temperature in kelvins
        if (t.equals(MQTT_TOPIC_KELVIN)) {
            _lightAdjustKelvin(payload);
            _lightUpdateFromMqtt();
            return;
        }

        // Color
        if (t.equals(MQTT_TOPIC_COLOR_RGB) || t.equals(MQTT_TOPIC_COLOR_HEX)) {
            _lightFromRgbPayload(payload);
            _lightUpdateFromMqtt();
            return;
        }

        if (t.equals(MQTT_TOPIC_COLOR_HSV)) {
            _lightFromHsvPayload(payload);
            _lightUpdateFromMqtt();
            return;
        }

        // Transition setting (persist)
        if (t.equals(MQTT_TOPIC_TRANSITION)) {
            _lightApiTransition(payload);
            return;
        }

        // Brightness
        if (t.equals(MQTT_TOPIC_BRIGHTNESS)) {
            _lightAdjustBrightness(payload);
            _lightUpdateFromMqtt();
            return;
        }

        // Channel
        if (t.startsWith(MQTT_TOPIC_CHANNEL)) {
            size_t id;
            if (_lightTryParseChannel(t, id)) {
                _lightAdjustChannel(id, payload);
                _lightUpdateFromMqtt();
            }
            return;
        }

        // Global
        if (t.equals(MQTT_TOPIC_LIGHT)) {
            _lightParsePayload(payload);
            _lightUpdateFromMqtt();
        }

    }

}

void _lightMqttSetup() {
    mqttHeartbeat(_lightMqttHeartbeat);
    mqttRegister(_lightMqttCallback);
}

} // namespace

void lightMQTT() {
    if (_light_has_color) {
        const auto rgb = _lightToTargetRgb();
        mqttSend(MQTT_TOPIC_COLOR_HEX, _lightRgbHexPayload(rgb).c_str());
        mqttSend(MQTT_TOPIC_COLOR_RGB, _lightRgbPayload(rgb).c_str());
        mqttSend(MQTT_TOPIC_COLOR_HSV, _lightHsvPayload(rgb).c_str());
    }

    if (_light_has_color || _light_has_cold_white) {
        const auto mireds = _light_temperature.mireds();
        mqttSend(MQTT_TOPIC_MIRED, String(mireds.value, 10).c_str());
    }

    for (size_t channel = 0; channel < _light_channels.size(); ++channel) {
        mqttSend(MQTT_TOPIC_CHANNEL, channel, String(_light_channels[channel].target, 10).c_str());
    }

    mqttSend(MQTT_TOPIC_BRIGHTNESS, _light_brightness.toString().c_str());
    mqttSend(MQTT_TOPIC_LIGHT, _light_state ? "1" : "0");
}

void lightMQTTGroup() {
    const auto mqtt_group_color = espurna::light::settings::mqttGroup();
    if (mqtt_group_color.length()) {
        mqttSendRaw(mqtt_group_color.c_str(), _lightGroupPayload().c_str());
    }
}

#endif

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

#if API_SUPPORT

namespace {

template <typename T>
bool _lightApiTryHandle(ApiRequest& request, T&& callback) {
    const auto param = request.wildcard(0);

    size_t id;
    if (!_lightTryParseChannel(param, id)) {
        return false;
    }

    return callback(id);
}

bool _lightApiRgbSetter(ApiRequest& request) {
    lightParseRgb(request.param(F("value")));
    lightUpdate();
    return true;
}

void _lightApiSetup() {
    if (_light_has_color) {
        apiRegister(F(MQTT_TOPIC_COLOR_RGB),
            [](ApiRequest& request) {
                request.send(_lightRgbPayload(_lightToTargetRgb()));
                return true;
            },
            _lightApiRgbSetter
        );

        apiRegister(F(MQTT_TOPIC_COLOR_HEX),
            [](ApiRequest& request) {
                request.send(_lightRgbHexPayload(_lightToTargetRgb()));
                return true;
            },
            _lightApiRgbSetter
        );

        apiRegister(F(MQTT_TOPIC_COLOR_HSV),
            [](ApiRequest& request) {
                request.send(_lightHsvPayload());
                return true;
            },
            [](ApiRequest& request) {
                lightParseHsv(request.param(F("value")));
                lightUpdate();
                return true;
            }
        );
    }

    if (_light_has_color || _light_has_cold_white || _light_has_warm_white) {
        apiRegister(F(MQTT_TOPIC_MIRED),
            [](ApiRequest& request) {
                const auto mireds = _light_temperature.mireds();
                request.send(String(mireds.value, 10));
                return true;
            },
            [](ApiRequest& request) {
                _lightAdjustMireds(request.param(F("value")));
                lightUpdate();
                return true;
            }
        );

        apiRegister(F(MQTT_TOPIC_KELVIN),
            [](ApiRequest& request) {
                const auto kelvin = _light_temperature.kelvin();
                request.send(String(kelvin.value, 10));
                return true;
            },
            [](ApiRequest& request) {
                _lightAdjustKelvin(request.param(F("value")));
                lightUpdate();
                return true;
            }
        );
    }

    apiRegister(F(MQTT_TOPIC_TRANSITION),
        [](ApiRequest& request) {
            request.send(String(lightTransitionTime().count()));
            return true;
        },
        [](ApiRequest& request) {
            return _lightApiTransition(request.param(F("value")));
        }
    );

    apiRegister(F(MQTT_TOPIC_BRIGHTNESS),
        [](ApiRequest& request) {
            request.send(_light_brightness.toString());
            return true;
        },
        [](ApiRequest& request) {
            _lightAdjustBrightness(request.param(F("value")));
            lightUpdate();
            return true;
        }
    );

    apiRegister(F(MQTT_TOPIC_CHANNEL "/+"),
        [](ApiRequest& request) {
            return _lightApiTryHandle(request, [&](size_t id) {
                request.send(String(_light_channels[id].target));
                return true;
            });
        },
        [](ApiRequest& request) {
            return _lightApiTryHandle(request, [&](size_t id) {
                _lightAdjustChannel(id, request.param(F("value")));
                lightUpdate();
                return true;
            });
        }
    );

    apiRegister(F(MQTT_TOPIC_LIGHT),
        [](ApiRequest& request) {
            request.send(lightState() ? "1" : "0");
            return true;
        },
        [](ApiRequest& request) {
            _lightParsePayload(request.param(F("value")));
            lightUpdate();
            return true;
        }
    );
}

} // namespace

#endif // API_SUPPORT

#if WEB_SUPPORT

namespace {

bool _lightWebSocketOnKeyCheck(espurna::StringView key, const JsonVariant&) {
    return key.startsWith(STRING_VIEW("light"))
        || key.startsWith(STRING_VIEW("use"))
        || key.startsWith(STRING_VIEW("lt"));
}

void _lightWebSocketStatus(JsonObject& root) {
    JsonObject& light = root.createNestedObject("light");

    if (_light_use_color) {
        const auto rgb = _lightToInputRgb();
        if (_light_use_rgb) {
            light["rgb"] = _lightRgbHexPayload(rgb);
        } else {
            const auto hsv = _lightHsv(rgb);
            light["hsv"] = _lightHsvPayload(espurna::light::Hsv(
                hsv.hue(), hsv.saturation(), _lightBrightnessPercent()));
        }
    }

    if (_light_use_cct) {
        light["mireds"] = _light_temperature.mireds().value;
    }

    JsonArray& values = light.createNestedArray("values");
    for (auto& channel : _light_channels) {
        values.add(channel.inputValue);
    }

    light["brightness"] = _light_brightness.value();
    light["state"] = _light_state;
}

void _lightWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, PSTR("light"));

    JsonObject& light = root.createNestedObject("light");
#if RELAY_SUPPORT
    light["state_relay_id"] = _light_state_relay_id;
#endif

    JsonArray& channels = light.createNestedArray("channels");

    const auto Channels = _light_channels.size();
    for (size_t index = 0; index < Channels; ++index) {
        channels.add(String(_lightTag(Channels, index)));
    }

    if (_light_use_cct) {
        JsonObject& cct = light.createNestedObject("cct");
        cct["cold"] = _light_temperature.cold();
        cct["warm"] = _light_temperature.warm();
    }
}

void _lightWebSocketOnConnected(JsonObject& root) {
    root["mqttGroupColor"] = espurna::light::settings::mqttGroup();
    root["useWhite"] = _light_use_white;
    root["useCCT"] = _light_use_cct;
    root["useColor"] = _light_use_color;
    root["useGamma"] = _light_use_gamma;
    root["useRGB"] = _light_use_rgb;
    root["useTransitions"] = _light_use_transitions;
    root["ltSave"] = _light_save;
    root["ltSaveDelay"] = _light_save_delay.count();
    root["ltTime"] = _light_transition_time.count();
    root["ltStep"] = _light_transition_step.count();
}

void _lightWebSocketOnAction(uint32_t client_id, const char* action, JsonObject& data) {
    STRING_VIEW_INLINE(Light, "light");
    if (Light != action) {
        return;
    }

    bool update { false };

    STRING_VIEW_INLINE(State, "state");
    if (data.containsKey("state")) {
        lightState(data[State].as<bool>());
        update = true;
    }

    STRING_VIEW_INLINE(Brightness, "brightness");
    if (data.containsKey(Brightness)) {
        lightBrightness(data[Brightness].as<long>());
        update = true;
    }

    STRING_VIEW_INLINE(Rgb, "rgb");
    if (data.containsKey(Rgb)) {
        _lightFromRgbPayload(data[Rgb].as<String>());
        update = true;
    }

    STRING_VIEW_INLINE(Hsv, "hsv");
    if (data.containsKey(Hsv)) {
        lightHsv(_lightHsvFromPayload(data[Hsv].as<String>()));
        update = true;
    }

    STRING_VIEW_INLINE(Mireds, "mireds");
    if (data.containsKey(Mireds)) {
        _lightTemperature(espurna::light::Mireds{
            .value = data[Mireds].as<long>()
        });
        update = true;
    }

    STRING_VIEW_INLINE(Channel, "channel");
    JsonObject& channel = data[Channel];

    if (channel.success()) {
        for (auto& kv : channel) {
            size_t id;
            if (!_lightTryParseChannel(kv.key, id)) {
                break;
            }

            _lightAdjustChannel(id, kv.value.as<String>());
            update = true;
        }
    }

    if (update) {
        lightUpdate();
    }
}

} // namespace

#endif

#if TERMINAL_SUPPORT
namespace {

// TODO: at this point we have 3 different state save / restoration
// routines that do *almost* the same thing
// (key point is, almost)

// Special persistance case were we take a snapshot of the boolean
// state, brightness and of current input and converted values

struct LightValuesState {
    LightValues inputs;
    long brightness;
    bool state;
};

LightValuesState _lightValuesState() {
    LightValuesState out{};

    std::transform(
        _light_channels.begin(), _light_channels.end(),
        out.inputs.begin(),
        [](const LightChannel& channel) {
            return channel.inputValue;
        });

    out.brightness = _light_brightness.value();
    out.state = _light_state;

    return out;
}

void _lightNotificationRestore(const LightValuesState& state) {
    for (size_t index = 0; index < _light_channels.size(); ++index) {
        lightChannel(index, state.inputs[index]);
    }

    lightBrightness(state.brightness);
    lightState(state.state);
}

void _lightNotificationInit(size_t channel) {
    for (size_t channel = 0; channel < _light_channels.size(); ++channel) {
        lightChannel(channel, espurna::light::ValueMin);
    }

    lightChannel(channel, espurna::light::ValueMax);
    lightBrightness(espurna::light::BrightnessMax);
    lightState(true);
}

PROGMEM_STRING(LightCommandNotify, "NOTIFY");

static void _lightCommandNotify(::terminal::CommandContext&& ctx) {
    static constexpr auto NotifyTransition = LightTransition{
        .time = espurna::duration::Seconds(1),
        .step = espurna::duration::Milliseconds(50),
    };

    if ((ctx.argv.size() < 2) || (ctx.argv.size() > 5)) {
        terminalError(ctx, F("NOTIFY <CHANNEL> [<REPEATS>] [<TIME>] [<STEP>]"));
        return;
    }

    size_t channel;
    if (!_lightTryParseChannel(ctx.argv[1], channel)) {
        terminalError(ctx, F("Invalid channel ID"));
        return;
    }

    using Duration = espurna::duration::Milliseconds;
    const auto time_convert = espurna::settings::internal::convert<Duration>;

    constexpr auto DefaultNotification = LightTransition {
        .time = Duration(500),
        .step = Duration(25),
    };

    const auto notification = (ctx.argv.size() >= 4)
        ? LightTransition{
            .time = time_convert(ctx.argv[2]),
            .step = time_convert(ctx.argv[3])}
        : DefaultNotification;

    constexpr size_t DefaultRepeats { 3 };

    const auto repeats_convert = espurna::settings::internal::convert<size_t>;
    const auto repeats = (ctx.argv.size() >= 5)
        ? repeats_convert(ctx.argv[4])
        : DefaultRepeats;

    auto state = std::make_shared<LightValuesState>(_lightValuesState());
    auto restore = [state]() {
        _lightNotificationRestore(*state);
        lightUpdate(NotifyTransition, 0, false);
    };

    auto on = [channel, notification]() {
        lightChannel(channel, espurna::light::ValueMax);
        lightUpdateSequence(notification);
    };

    auto off = [channel, notification]() {
        lightChannel(channel, espurna::light::ValueMin);
        lightUpdateSequence(notification);
    };

    _lightNotificationInit(channel);
    lightUpdate(NotifyTransition);

    LightSequenceCallbacks callbacks;
    callbacks.push_front(restore);
    for (size_t n = 0; n < repeats; ++n) {
        callbacks.push_front(off);
        callbacks.push_front(on);
    }

    lightSequence(std::move(callbacks));
}

PROGMEM_STRING(LightCommand, "LIGHT");

static void _lightCommand(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        if (!_lightParsePayload(ctx.argv[1])) {
            terminalError(ctx, F("Invalid payload"));
            return;
        }
        lightUpdate();
    }

    ctx.output.printf_P(PSTR("%s\n"),
        _light_state ? PSTR("ON") : PSTR("OFF"));
    terminalOK(ctx);
}

PROGMEM_STRING(LightCommandBrightness, "BRIGHTNESS");

static void _lightCommandBrightness(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        _lightAdjustBrightness(ctx.argv[1]);
        lightUpdate();
    }
    ctx.output.printf_P(PSTR("%ld\n"), _light_brightness);
    terminalOK(ctx);
}

PROGMEM_STRING(LightCommandChannel, "CHANNEL");

static void _lightCommandChannel(::terminal::CommandContext&& ctx) {
    const size_t Channels { _light_channels.size() };
    if (!Channels) {
        terminalError(ctx, F("No channels configured"));
        return;
    }

    auto description = [&](size_t channel) {
        ctx.output.printf_P(PSTR("#%zu (%s) input:%ld value:%ld target:%ld current:%s\n"),
                channel,
                _lightDesc(Channels, channel),
                _light_channels[channel].inputValue,
                _light_channels[channel].value,
                _light_channels[channel].target,
                String(_light_channels[channel].current, 2).c_str());
    };

    if (ctx.argv.size() > 2) {
        size_t id;
        if (!_lightTryParseChannel(ctx.argv[1], id)) {
            terminalError(ctx, F("Invalid channel ID"));
            return;
        }

        _lightAdjustChannel(id, ctx.argv[2]);
        lightUpdate();
        description(id);
    } else {
        for (size_t index = 0; index < Channels; ++index) {
            description(index);
        }
    }

    terminalOK(ctx);
}

PROGMEM_STRING(LightCommandRgb, "RGB");

static void _lightCommandColors(const ::terminal::CommandContext& ctx) {
    const auto rgb = _lightToTargetRgb();
    ctx.output.printf_P(PSTR("hsv %s\n"),
        _lightHsvPayload(rgb).c_str());
    ctx.output.printf_P(PSTR("rgb %s\n"),
        _lightRgbPayload(rgb).c_str());

    terminalOK(ctx);
}

static void _lightCommandRgb(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        _lightFromRgbPayload(ctx.argv[1]);
        lightUpdate();
    }

    _lightCommandColors(ctx);
}

PROGMEM_STRING(LightCommandHsv, "HSV");

static void _lightCommandHsv(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        _lightFromHsvPayload(ctx.argv[1]);
        lightUpdate();
    }

    _lightCommandColors(ctx);
}

PROGMEM_STRING(LightCommandKelvin, "KELVIN");

static void _lightCommandKelvin(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        _lightAdjustKelvin(ctx.argv[1]);
        lightUpdate();
    }

    const auto kelvin = _light_temperature.kelvin();
    ctx.output.printf_P(PSTR("kelvin %ld\n"), kelvin.value);
    terminalOK(ctx);
}

PROGMEM_STRING(LightCommandMired, "MIRED");

static void _lightCommandMired(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        _lightAdjustMireds(ctx.argv[1]);
        lightUpdate();
    }

    const auto mireds = _light_temperature.mireds();
    const auto cold = _light_temperature.cold();
    const auto warm = _light_temperature.warm();
    ctx.output.printf_P(PSTR("mireds %ld range %ld...%ld (factor %s%)\n"),
        mireds.value, cold, warm,
        String(_light_temperature.factor(), 1).c_str());
    terminalOK(ctx);
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {LightCommandNotify, _lightCommandNotify},
    {LightCommand, _lightCommand},
    {LightCommandBrightness, _lightCommandBrightness},
    {LightCommandChannel, _lightCommandChannel},
    {LightCommandRgb, _lightCommandRgb},
    {LightCommandHsv, _lightCommandHsv},
    {LightCommandKelvin, _lightCommandKelvin},
    {LightCommandMired, _lightCommandMired},
};

void _lightInitCommands() {
    espurna::terminal::add(Commands);
}

} // namespace
#endif // TERMINAL_SUPPORT

size_t lightChannels() {
    return _light_channels.size();
}

bool lightHasWhite() {
    return _light_has_cold_white || _light_has_warm_white;
}

bool lightHasColdWhite() {
    return _light_has_cold_white;
}

bool lightHasWarmWhite() {
    return _light_has_warm_white;
}

bool lightHasColor() {
    return _light_has_color;
}

bool lightUseCCT() {
    return _light_use_cct;
}

bool lightUseRGB() {
    return _light_use_rgb;
}

// -----------------------------------------------------------------------------

espurna::light::Rgb lightRgb() {
    return _lightToTargetRgb();
}

void lightRgb(espurna::light::Rgb rgb) {
    _light_mapping.red(rgb.red());
    _light_mapping.green(rgb.green());
    _light_mapping.blue(rgb.blue());
}

void lightHs(long hue, long saturation) {
    lightRgb(_lightRgb(
        espurna::light::Hsv(
            hue, saturation,
            espurna::light::Hsv::ValueMax)));
}

void lightHsv(espurna::light::Hsv hsv) {
    lightHs(hsv.hue(), hsv.saturation());
    lightBrightnessPercent(hsv.value());
}

espurna::light::Hsv lightHsv() {
    return _lightHsv(_lightToTargetRgb());
}

// -----------------------------------------------------------------------------

void lightOnReport(LightReportListener func) {
    _light_report.push_front(func);
}

namespace {

void _lightReport(int report) {
#if MQTT_SUPPORT
    if (report & espurna::light::Report::Mqtt) {
        lightMQTT();
    }

    if (report & espurna::light::Report::MqttGroup) {
        lightMQTTGroup();
    }
#endif

#if WEB_SUPPORT
    if (report & espurna::light::Report::Web) {
        wsPost(_lightWebSocketStatus);
    }
#endif

    for (auto& report : _light_report) {
        report();
    }
}

// Called in the loop() when we received lightUpdate(...) values

void _lightUpdateDebug(const LightTransitionHandler& handler) {
    const auto Time = handler.time();
    const auto Step = handler.step();
    if (Time.count() - Step.count()) {
        DEBUG_MSG_P(PSTR("[LIGHT] Scheduled transition for %u (ms) every %u (ms)\n"),
                Time.count(), Step.count());
    }

    for (auto& transition : handler.prepared()) {
        if (transition.count > 1) {
            DEBUG_MSG_P(PSTR("[LIGHT] Transition from %s to %ld (step %s, %u times)\n"),
                    String(transition.value, 2).c_str(), transition.target,
                    String(transition.step, 2).c_str(), transition.count);
        }
    }
}

struct LightValuesObserver {
    LightValuesObserver() = delete;
    explicit LightValuesObserver(const LightChannels& channels) :
        _channels(channels),
        _last_values(values(_channels)),
        _last_size(_channels.size())
    {}

    bool changed() const {
        return (_last_size != _channels.size())
            || (_last_values != values(_channels));
    }

private:
    static LightValues values(const LightChannels& channels) {
        LightValues out{};

        std::transform(
            channels.begin(), channels.end(), out.begin(),
            [](const LightChannel& channel) {
                return channel.value;
            });

        return out;
    }

    const LightChannels& _channels;

    LightValues _last_values{};
    size_t _last_size { 0 };
};

void _lightSequenceCheck() {
    if (!_light_update && !_light_transition) {
        _light_sequence.run();
    }
}

void _lightPostLoop() {
    if (_light_report_timer) {
        _lightReport(_light_report_timer.get());
    }

    if (_light_save_timer) {
        _light_save_timer.reset();
        _lightSaveSettings();
    }
}

void _lightUpdate() {
    if (!_light_update) {
        return;
    }

    LightValuesObserver observer(_light_channels);
    _light_process_input_values(_light_channels);

    if (!_light_state_changed && !observer.changed()) {
        _light_update.cancel();
        return;
    }

    _light_state_changed = false;
    _light_update.run([](LightTransition transition, int report, bool save) {
        // Channel output values will be set by the handler class and the specified provider
        // We either set the values immediately or schedule an ongoing transition
        _light_transition = std::make_unique<LightTransitionHandler>(_light_channels, transition, _light_state);
        _light_provider_update.start(_light_transition->step());
        _lightUpdateDebug(*_light_transition);

        // Send current state to all available 'report' targets
        // (make sure to delay the report, in case lightUpdate is called repeatedly)
        _light_report_timer.wait_set(_light_report_delay, report);

        // Always save to RTCMEM, optionally preserve the state in the settings storage
        _lightSaveRtcmem();
        if (save) {
            _light_save_timer.wait_set(_light_save_delay, true);
        }
    });
}

void _lightUpdate(LightTransition transition, int report, bool save, bool sequence) {
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
    if (!_light_provider) {
        return;
    }
#endif

    if (!_light_channels.size()) {
        return;
    }

    _light_update.set(transition, report, save);
    if (!sequence) {
        _light_sequence.clear();
    }
}

void _lightUpdate(LightTransition transition, int report, bool save) {
    _lightUpdate(transition, report, save, false);
}

void _lightUpdate(LightTransition transition) {
    _lightUpdate(transition, espurna::light::Report::Default, _light_save, false);
}

void _lightUpdate(bool save) {
    _lightUpdate(lightTransition(), espurna::light::Report::Default, save, false);
}

} // namespace

void lightSequence(LightSequenceCallbacks callbacks) {
    _light_sequence = std::move(callbacks);
}

void lightUpdateSequence(LightTransition transition) {
    _lightUpdate(transition, espurna::light::Report::None, false, true);
}

void lightUpdate(LightTransition transition, int report, bool save) {
    _lightUpdate(transition, report, save);
}

void lightUpdate(LightTransition transition) {
    _lightUpdate(transition);
}

void lightUpdate(bool save) {
    _lightUpdate(save);
}

void lightUpdate() {
    _lightUpdate(lightTransition());
}

void lightState(size_t id, bool state) {
    if ((id < _light_channels.size()) && _light_channels[id].state != state) {
        _light_channels[id].state = state;
        _light_state_changed = true;
    }
}

bool lightState(size_t id) {
    if (id < _light_channels.size()) {
        return _light_channels[id].state;
    }

    return false;
}

void lightState(bool state) {
    if (_light_state != state) {
        _light_state = state;
        if (_light_state_listener) {
            _light_state_listener(state);
        }
        _light_state_changed = true;
    }
}

bool lightState() {
    return _light_state;
}

void lightParseHsv(espurna::StringView value) {
    _lightFromHsvPayload(value);
}

void lightParseRgb(espurna::StringView value) {
    _lightFromRgbPayload(value);
}

String lightRgbPayload() {
    return _lightRgbPayload();
}

String lightHsvPayload() {
    return _lightHsvPayload();
}

String lightColor() {
    return _light_use_rgb ? lightRgbPayload() : lightHsvPayload();
}

long lightRed() {
    return _light_mapping.red();
}

void lightRed(long value) {
    _light_mapping.red(value);
}

long lightGreen() {
    return _light_mapping.green();
}

void lightGreen(long value) {
    _light_mapping.green(value);
}

long lightBlue() {
    return _light_mapping.blue();
}

void lightBlue(long value) {
    _light_mapping.blue(value);
}

long lightWarmWhite() {
    return _light_mapping.warm();
}

void lightWarmWhite(long value) {
    _light_mapping.warm(value);
}

long lightColdWhite() {
    return _light_mapping.cold();
}

void lightColdWhite(long value) {
    _light_mapping.cold(value);
}

void lightTemperature(espurna::light::Mireds mireds) {
    _lightTemperature(mireds);
}

void lightMireds(espurna::light::Kelvin kelvin) {
    _lightTemperature(kelvin);
}

espurna::light::TemperatureRange lightMiredsRange() {
    return _light_temperature.range();
}

long lightChannel(size_t id) {
    if (id < _light_channels.size()) {
        return _light_channels[id].inputValue;
    }

    return 0l;
}

void lightChannel(size_t id, long value) {
    if (id < _light_channels.size()) {
        _light_channels[id] = value;
    }
}

void lightChannelStep(size_t id, long steps, long multiplier) {
    lightChannel(id, lightChannel(id) + (steps * multiplier));
}

void lightChannelStep(size_t id, long steps) {
    lightChannelStep(id, steps, espurna::light::ValueStep);
}

long lightBrightness() {
    return _light_brightness.value();
}

void lightBrightnessPercent(long percent) {
    _lightBrightnessPercent(percent);
}

void lightBrightness(long brightness) {
    _light_brightness = std::clamp(brightness, espurna::light::BrightnessMin, espurna::light::BrightnessMax);
}

void lightBrightnessStep(long steps, long multiplier) {
    lightBrightness(_light_brightness.value() + (steps * multiplier));
}

void lightBrightnessStep(long steps) {
    lightBrightnessStep(steps, espurna::light::ValueStep);
}

espurna::duration::Milliseconds lightTransitionTime() {
    return _light_use_transitions
        ? _light_transition_time
        : espurna::duration::Milliseconds(0);
}

espurna::duration::Milliseconds lightTransitionStep() {
    return _light_use_transitions
        ? _light_transition_step
        : espurna::duration::Milliseconds(0);
}

LightTransition lightTransition() {
    return {lightTransitionTime(), lightTransitionStep()};
}

void lightTransition(espurna::duration::Milliseconds time, espurna::duration::Milliseconds step) {
    bool save { false };

    _light_use_transitions = (time.count() > 0) && (step.count() > 0);
    if (_light_use_transitions) {
        save = true;
        _light_transition_time = time;
        _light_transition_step = step;
    }

    espurna::light::settings::transition(_light_use_transitions);
    if (save) {
        espurna::light::settings::transitionTime(_light_transition_time);
        espurna::light::settings::transitionStep(_light_transition_step);
    }

    saveSettings();
}

void lightTransition(LightTransition transition) {
    lightTransition(transition.time, transition.step);
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

namespace {

inline bool _lightUseGamma(size_t channels, size_t index) {
    switch (_lightTag(channels, index)) {
    case 'R':
    case 'G':
    case 'B':
        return true;
    }

    return false;
}

void _lightConfigure() {
    const auto Channels = _light_channels.size();

    const auto has_color = (Channels >= 3);
    _light_has_color = has_color;

    const auto use_color = espurna::light::settings::color();
    _light_use_color = use_color && has_color;
    if (!_light_use_color) {
        espurna::light::settings::color(false);
    }

    _light_use_rgb = espurna::light::settings::rgb();

    const auto has_warm_white = (Channels >= 4) || (Channels >= 1);
    _light_has_warm_white = has_warm_white;

    const auto has_cold_white = (Channels == 5) || (Channels == 2);
    _light_has_cold_white = has_cold_white;

    const auto use_white = espurna::light::settings::white();
    _light_use_white = use_white && (has_cold_white || has_warm_white);
    if (!_light_use_white) {
        espurna::light::settings::white(false);
    }

    const auto use_cct = espurna::light::settings::cct();
    _light_use_cct = use_cct && has_cold_white && has_warm_white;
    if (!_light_use_cct) {
        espurna::light::settings::cct(false);
    }

    _light_temperature.range(
        espurna::light::TemperatureRange{
            espurna::light::settings::miredsCold(),
            espurna::light::settings::miredsWarm()
        });

    _light_use_transitions = espurna::light::settings::transition();
    _light_transition_time = espurna::light::settings::transitionTime();
    _light_transition_step = espurna::light::settings::transitionStep();

    _light_save = espurna::light::settings::save();
    _light_save_delay = espurna::light::settings::saveDelay();

    _light_use_gamma = espurna::light::settings::gamma();
    for (size_t index = 0; index < Channels; ++index) {
#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
        _light_my92xx_channel_map[index] = espurna::light::settings::my92xxChannel(index);
#endif
        _light_channels[index].inverse = espurna::light::settings::inverse(index);
        _light_channels[index].gamma = (_light_has_color && _light_use_gamma) && _lightUseGamma(Channels, index);
    }

    const auto last_process_input_values = _light_process_input_values;
    auto process_input_values =
        (_light_use_color && _light_use_white && _light_use_cct)
            ? _lightValuesWithCct :
        (_light_use_color && _light_use_white)
            ? _lightValuesWithRgbWhite :
        (_light_use_color && _light_use_cct)
            ? _lightValuesWithRgbCct :
        (_light_use_color)
            ? _lightValuesWithBrightnessExceptWhite :
        (_light_use_cct)
            ? _lightValuesWithCct
            : _lightValuesWithBrightness;

    _light_process_input_values = process_input_values;
    if (!_light_update && (last_process_input_values != process_input_values)) {
        lightUpdate(false);
    }
}

void _lightSleepSetup() {
    systemBeforeSleep(
        []() {
            size_t id = 0;
            for (auto& channel : _light_channels) {
                _lightProviderHandleValue(id, 0);
                ++id;

                channel.value = 0;
            }

            _lightProviderHandleState(false);
            _lightProviderHandleUpdate();
            espurna::time::blockingDelay(
                espurna::duration::Milliseconds{ 100 });
        });

    systemAfterSleep(
        []() {
            _lightUpdate(false);
            _light_state_changed = true;
        });
}

void _lightBoot() {
    const size_t Channels { _light_channels.size() };
    if (Channels) {
        DEBUG_MSG_P(PSTR("[LIGHT] Number of channels: %zu\n"), Channels);

        _lightUpdateMapping(_light_channels);
        _lightConfigure();
        if (rtcmemStatus()) {
            _lightRestoreRtcmem();
        } else {
            _lightRestoreSettings();
        }

        _light_state_changed = true;
        lightUpdate(false);
    }
}

} // namespace

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM

// Custom provider is expected to:
// - register a controller class via `lightSetProvider(...)`
// - use `lightAdd()` N times to create N channels that will be handled via the controller
// Once that's done, we 'boot' the provider and disable further calls to the `lightAdd()`

void lightSetProvider(std::unique_ptr<LightProvider>&& ptr) {
    _light_provider = std::move(ptr);
}

bool lightAdd() {
    enum class State {
        None,
        Scheduled,
        Done
    };

    static State state { State::None };
    if (State::Done == state) {
        return false;
    }

    if (_light_channels.size() < espurna::light::ChannelsMax) {
        _light_channels.emplace_back(LightChannel());
        if (State::Scheduled != state) {
            state = State::Scheduled;
            espurnaRegisterOnceUnique([]() {
                _lightBoot();
                state = State::Done;
            });
        }

        return true;
    }

    return false;
}

#else

bool lightAdd() {
    return false;
}

#endif // LIGHT_PROVIDER_CUSTOM

namespace {

void _lightProviderDebug() {
    DEBUG_MSG_P(PSTR("[LIGHT] Provider: "
#if LIGHT_PROVIDER == LIGHT_PROVIDER_NONE
        "NONE"
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
        "DIMMER"
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
        "MY92XX"
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
        "CUSTOM"
#endif
    "\n"));
}

void _lightSettingsMigrate(int version) {
    if (version < 5) {
        delSettingPrefix({
            "chGPIO",
            "chLogic",
            "myChips",
            "myDCKGPIO",
            "myDIGPIO"
        });
        delSetting("lightProvider");
        delSetting("useCSS");

        moveSetting("lightTime", "ltTime");
        moveSetting("lightColdMired", "ltColdMired");
        moveSetting("lightWarmMired", "ltWarmMired");
    }

    if (version < 14) {
        delSetting(F("ltRelay"));
    }
}

} // namespace

// -----------------------------------------------------------------------------

RelayProviderBasePtr lightMakeStateRelayProvider(size_t id) {
#if RELAY_SUPPORT
    if (!_light_state_listener) {
        _light_state_relay_id = id;
        _light_state_listener = [id](bool state) {
            relayStatus(id, state);
        };

        return std::make_unique<LightStateProvider>();
    }
#endif

    return nullptr;
}

void lightSetup() {
    migrateVersion(_lightSettingsMigrate);

    const auto enable_pin = espurna::light::settings::enablePin();
    if (enable_pin != GPIO_NONE) {
        pinMode(enable_pin, OUTPUT);
        digitalWrite(enable_pin, HIGH);
    }

    _light_channels.reserve(espurna::light::ChannelsMax);
    _lightProviderDebug();

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
    {
        // TODO: library API specifies some hard-coded amount of channels, based off of the model and chips
        // we always map channel index 1-to-1, to simplify hw config, but most of the time there are less active channels
        // than the value generated by the lib (ref. `my92xx::getChannels()`)
        auto channels = espurna::light::settings::my92xxChannels();
        if (channels) {
            _my92xx = std::make_unique<my92xx>(
                    espurna::light::settings::my92xxModel(),
                    espurna::light::settings::my92xxChips(),
                    espurna::light::settings::my92xxDiPin(),
                    espurna::light::settings::my92xxDckiPin(),
                    espurna::light::build::my92xxCommand());
            _light_channels.resize(channels);
        }
    }
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
    {
        // Load up until first invalid pin. Allow settings to override, but not remove values
        std::vector<uint8_t> pins;
        pins.reserve(espurna::light::ChannelsMax);

        for (size_t index = 0; index < espurna::light::ChannelsMax; ++index) {
            const auto pin = espurna::light::settings::channelPin(index);
            if (!gpioValid(pin)) {
                break;
            }

            pins.push_back(pin);
        }

        // The rest is handled by the PWM driver, continue *only* when it actually agrees on selected pins
        if (pwmInitPins(pins)) {
            const auto range = pwmRange();
            _light_pwm_min = range.min;
            _light_pwm_max = range.max;
            _light_channels.resize(pins.size());
        }
    }
#endif

    _lightBoot();

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_lightWebSocketOnVisible)
            .onConnected(_lightWebSocketOnConnected)
            .onData(_lightWebSocketStatus)
            .onAction(_lightWebSocketOnAction)
            .onKeyCheck(_lightWebSocketOnKeyCheck);
    #endif

    #if API_SUPPORT
        _lightApiSetup();
    #endif

    #if MQTT_SUPPORT
        _lightMqttSetup();
    #endif

    #if TERMINAL_SUPPORT
        _lightInitCommands();
    #endif

        _lightSleepSetup();

    espurnaRegisterReload(_lightConfigure);
    espurnaRegisterLoop([]() {
        _lightSequenceCheck();
        _lightUpdate();
        _lightProviderUpdate();
        _lightPostLoop();
    });
}

#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
