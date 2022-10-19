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

#if __GNUC__ > 4
static_assert(std::is_trivially_copyable<espurna::light::Rgb>::value, "");
static_assert(std::is_trivially_copyable<espurna::light::Hsv>::value, "");
static_assert(std::is_trivially_copyable<espurna::light::MiredsRange>::value, "");
#endif

namespace espurna {
namespace light {

// TODO: unless we are building with latest Core versions and -std=c++17, these need to be explicitly bound to at least one object file
#if __cplusplus < 201703L
constexpr long Rgb::Min;
constexpr long Rgb::Max;

constexpr long Hsv::HueMin;
constexpr long Hsv::HueMax;

constexpr long Hsv::SaturationMin;
constexpr long Hsv::SaturationMax;

constexpr long Hsv::ValueMin;
constexpr long Hsv::ValueMax;
#endif

static_assert(MiredsCold < MiredsWarm, "");
constexpr long MiredsDefault { (MiredsCold + MiredsWarm) / 2L };

namespace {
namespace build {

constexpr float WhiteFactor { LIGHT_WHITE_FACTOR };

constexpr bool relay() {
    return 1 == LIGHT_RELAY_ENABLED;
}

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

long mireds() {
    return std::clamp(getSetting("mireds", espurna::light::MiredsDefault), espurna::light::MiredsCold, espurna::light::MiredsWarm);
}

long miredsCold() {
    return std::clamp(getSetting("ltColdMired", espurna::light::MiredsCold), espurna::light::MiredsCold, espurna::light::MiredsWarm);
}

long miredsWarm() {
    return std::clamp(getSetting("ltWarmMired", espurna::light::MiredsWarm), espurna::light::MiredsCold, espurna::light::MiredsWarm);
}

void mireds(long input) {
    setSetting("mireds", input);
}

long brightness() {
    return std::clamp(getSetting("brightness", espurna::light::BrightnessMax), espurna::light::BrightnessMin, espurna::light::BrightnessMax);
}

void brightness(long input) {
    setSetting("brightness", input);
}

String mqttGroup() {
    return getSetting("mqttGroupColor");
}

bool relay() {
    return getSetting("ltRelay", build::relay());
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

// Setup virtual relays contolling the light's state
// TODO: only do per-channel setup optionally

class LightChannelProvider : public RelayProviderBase {
public:
    LightChannelProvider() = delete;
    explicit LightChannelProvider(size_t id) :
        _id(id)
    {}

    const char* id() const override {
        return "light_channel";
    }

    void change(bool status) override {
        lightState(_id, status);
        lightState(true);
        lightUpdate();
    }

private:
    size_t _id { RelaysMax };
};

class LightGlobalProvider : public RelayProviderBase {
public:
    const char* id() const override {
        return "light_global";
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
    Pointers() = default;
    Pointers(const Pointers&) = default;
    Pointers(Pointers&&) = default;

    Pointers& operator=(const Pointers&) = default;
    Pointers& operator=(Pointers&&) = default;

    using Type = LightChannel*;

    Pointers(Type red, Type green, Type blue, Type cold, Type warm) :
        _red(red),
        _green(green),
        _blue(blue),
        _cold(cold),
        _warm(warm)
    {}

    LightChannel* red() const {
        return _red;
    }

    LightChannel* green() const {
        return _green;
    }

    LightChannel* blue() const {
        return _blue;
    }

    LightChannel* cold() const {
        return _cold;
    }

    LightChannel* warm() const {
        return _warm;
    }

private:
    LightChannel* _red { nullptr };
    LightChannel* _green { nullptr };
    LightChannel* _blue { nullptr };
    LightChannel* _cold { nullptr };
    LightChannel* _warm { nullptr };
};

struct Mapping {
    template <typename ...Args>
    void update(Args&&... args) {
        _pointers = Pointers(std::forward<Args>(args)...);
    }

    void reset() {
        _pointers = Pointers();
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

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
namespace settings {
namespace internal {

template <>
my92xx_model_t convert(const String& value) {
    alignas(4) static constexpr char MY9291[] PROGMEM = "9291";
    alignas(4) static constexpr char MY9231[] PROGMEM = "9231";

    using Options = std::array<espurna::settings::options::Enumeration<my92xx_model_t>, 2>;
    static constexpr Options options {
        {{MY92XX_MODEL_MY9291, MY9291},
         {MY92XX_MODEL_MY9231, MY9231}}
    };

    return convert(options, value, espurna::light::build::my92xxModel());
}

} // namespace internal
} // namespace settings
#endif

} // namespace espurna

namespace {

espurna::light::Mapping _light_mapping;

template <typename T>
void _lightUpdateMapping(T& channels) {
    switch (channels.size()) {
    case 0:
        break;
    case 1:
        _light_mapping.update(nullptr, nullptr, nullptr, &channels[0], nullptr);
        break;
    case 2:
        _light_mapping.update(nullptr, nullptr, nullptr, &channels[0], &channels[1]);
        break;
    case 3:
        _light_mapping.update(&_light_channels[0], &channels[1], &channels[2], nullptr, nullptr);
        break;
    case 4:
        _light_mapping.update(&channels[0], &channels[1], &channels[2], &channels[3], nullptr);
        break;
    case 5:
        _light_mapping.update(&channels[0], &channels[1], &channels[2], &channels[3], &channels[4]);
        break;
    }
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
            [&]() {
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

bool _light_has_controls = false;
bool _light_has_cold_white = false;
bool _light_has_warm_white = false;
bool _light_has_color = false;

bool _light_use_color = false;
bool _light_use_rgb = false;
bool _light_use_white = false;
bool _light_use_cct = false;
bool _light_use_gamma = false;

bool _light_state = false;
long _light_brightness = espurna::light::BrightnessMax;

// Default to the Philips Hue value that HA also use.
// https://developers.meethue.com/documentation/core-concepts

// TODO: We only accept this as input, thus setting 'related' channels directly
// will cause the cached mireds value to be used:
// - by brightness function in R G B CW and R G B CW WW as a factor for CW and WW channels
// - by setter in CW and CW WW modes

long _light_cold_mireds = espurna::light::MiredsCold;
long _light_warm_mireds = espurna::light::MiredsWarm;

long _light_cold_kelvin = (1000000L / _light_cold_mireds);
long _light_warm_kelvin = (1000000L / _light_warm_mireds);

long _light_mireds { espurna::light::MiredsDefault };

bool _light_state_changed = false;
LightStateListener _light_state_listener = nullptr;

void _lightProcessInputValuesNoop(LightChannels&, long) {
}

using LightProcessInputValues = void (*)(LightChannels&, long brightness);
LightProcessInputValues _light_process_input_values { _lightProcessInputValuesNoop };

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
my92xx* _my92xx { nullptr };
#endif

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
std::unique_ptr<LightProvider> _light_provider;
#endif

} // namespace

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

namespace {

// After the channel value was updated through the API (i.e. through changing the `inputValue`),
// these functions are expected to be called. Which one is chosen is based on the current settings values.
// TODO: existing mapping class handles setting `inputValue` & getting `target` value applied by the transition handler
// should it also handle setting the `value` so there's no need to refer to channels through numbers?

struct LightBrightness {
    LightBrightness() = delete;
    explicit LightBrightness(long brightness) :
        _brightness(std::clamp(brightness, espurna::light::BrightnessMin, espurna::light::BrightnessMax))
    {}

    long operator()(long input) const {
        return (input * _brightness) / espurna::light::BrightnessMax;
    }

private:
    long _brightness;
};

void _lightValuesWithBrightness(LightChannels& channels, long brightness) {
    const auto Brightness = LightBrightness{brightness};
    for (auto& channel : channels) {
        channel.apply(Brightness);
    }
}

void _lightValuesWithBrightnessExceptWhite(LightChannels& channels, long brightness) {
    const auto Brightness = LightBrightness{brightness};
    auto it = channels.begin();

    (*it).apply(Brightness);
    ++it;

    (*it).apply(Brightness);
    ++it;

    (*it).apply(Brightness);
    ++it;

    while (it != channels.end()) {
        (*it).apply();
        ++it;
    }
}

// When `useWhite` is enabled, white channels are 'detached' from the processing and their value depends on the RGB ones.
// Common calculation is to subtract 'white value' from the RGB based on the minimum channel value, e.g. [250, 150, 50] becomes [200, 100, 0, 50]
//
// With `useCCT` also enabled, value is instead split between Warm and Cold channels based on the current `mireds`.
// Otherwise, Warm channel is using the remainder and Cold uses the `inputValue` directly.
//
// (TODO: notice that this also means HSV mode will hardly agree with our changes and will try to bounce
// the brigthness all over the place. at least for now, only `useRGB` mode works correctly)

// Map from normal 153...500 to 0...347, so we get a value 0...1
double _lightMiredFactor() {
    if (_light_cold_mireds < _light_warm_mireds) {
        const auto Cold = static_cast<double>(_light_cold_mireds);
        const auto Warm = static_cast<double>(_light_warm_mireds);
        const auto Mireds = static_cast<double>(_light_mireds);
        return (Mireds - Cold) / (Warm - Cold);
    }

    return 0.0;
}

espurna::light::MiredsRange _lightCctRange(long value) {
    const double Factor { _lightMiredFactor() };
    return {
        std::lround(Factor * value),
        std::lround((1.0 - Factor) * value)};
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
    explicit LightRgbWithoutWhite(const LightChannels& channels) :
        _common(makeCommon(channels)),
        _factor(makeFactor(_common))
    {}

    long operator()(long input) const {
        return std::lround(static_cast<float>(input - _common.inputMin) * _factor);
    }

    template <typename... Args>
    void adjustOutput(Args&&... args) {
        _common.outputMax = std::max({_common.outputMax, std::forward<Args>(args)...});
        _factor = makeFactor(_common);
    }

    long inputMin() const {
        return _common.inputMin;
    }

    float factor() const {
        return _factor;
    }

private:
    struct Common {
        long inputMin;
        long inputMax;
        long outputMax;
    };

    static float makeFactor(const Common& common) {
        return (common.outputMax > 0)
            ? static_cast<float>(common.inputMax) / static_cast<float>(common.outputMax)
            : 0.0f;
    }

    static Common makeCommon(const LightChannels& channels) {
        Common out;
        out.inputMax = std::max({
                channels[0].inputValue, channels[1].inputValue, channels[2].inputValue});
        out.inputMin = std::min({
                channels[0].inputValue, channels[1].inputValue, channels[2].inputValue});
        out.outputMax = std::max({
            channels[0].inputValue - out.inputMin,
            channels[1].inputValue - out.inputMin,
            channels[2].inputValue - out.inputMin
        });

        return out;
    }

    Common _common;
    float _factor;
};

struct LightScaledWhite {
    LightScaledWhite() = delete;
    explicit LightScaledWhite(float factor) :
        _factor(factor)
    {}

    long operator()(long input) const {
        return std::lround(static_cast<float>(input) * _factor * espurna::light::build::WhiteFactor);
    }

private:
    float _factor;
};

// General case when `useCCT` is disabled, but there are 4 channels and `useWhite` is enabled
// Keeps 5th channel as-is, without applying the brightness scale or resetting the value to 0

void _lightValuesWithRgbWhite(LightChannels& channels, long brightness) {
    auto rgb = LightRgbWithoutWhite{channels};
    rgb.adjustOutput(rgb.inputMin());

    const auto Brightness = LightBrightness(brightness);
    auto it = channels.begin();
    (*it).apply(rgb, Brightness);
    ++it;

    (*it).apply(rgb, Brightness);
    ++it;

    (*it).apply(rgb, Brightness);
    ++it;

    (*it) = rgb.inputMin();
    (*it).apply(LightScaledWhite{rgb.factor()}, Brightness);
    ++it;

    if (it != channels.end()) {
        (*it).apply();
    }
}

// Instead of the above, use `mireds` value as a range for warm and cold channels, based on the calculated rgb common values
// Every value is also scaled by `brightness` after applying all of the previous steps

void _lightValuesWithRgbCct(LightChannels& channels, long brightness) {
    auto rgb = LightRgbWithoutWhite{channels};

    const auto Range = _lightCctRange(rgb.inputMin());
    rgb.adjustOutput(Range.warm(), Range.cold());

    const auto Brightness = LightBrightness(brightness);
    auto it = channels.begin();
    (*it).apply(rgb, Brightness);
    ++it;

    (*it).apply(rgb, Brightness);
    ++it;

    (*it).apply(rgb, Brightness);
    ++it;

    const auto White = LightScaledWhite{rgb.factor()};
    (*it) = Range.warm();
    (*it).apply(White, Brightness);
    ++it;

    (*it) = Range.cold();
    (*it).apply(White, Brightness);
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

void _lightFromCommaSeparatedPayload(espurna::StringView payload) {
    constexpr size_t BufferSize { 16 };
    if (payload.length() < BufferSize) {
        char buffer[BufferSize] = {0};
        std::copy(payload.begin(), payload.end(), buffer);

        auto it = _light_channels.begin();
        char* tok = std::strtok(buffer, ",");

        while ((it != _light_channels.end()) && (tok != nullptr)) {
            char* endp { nullptr };
            auto value = std::strtol(tok, &endp, 10);
            if ((endp == tok) || (*endp != '\0')) {
                break;
            }

            (*it) = value;
            ++it;

            tok = std::strtok(nullptr, ",");
        }

        // same as previous versions, set the rest to zeroes
        while (it != _light_channels.end()) {
            (*it) = 0;
            ++it;
        }
    }
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
    _lightFromCommaSeparatedPayload(payload);
}

// HSV string is expected to be "H,S,V", where:
// - H [0...360]
// - S [0...100]
// - V [0...100]

void _lightFromHsvPayload(espurna::StringView payload) {
    if (!_light_has_color || !payload.length() || (payload[0] == '\0')) {
        return;
    }

    constexpr size_t BufferSize { 16 };

    if (payload.length() < BufferSize) {
        char buffer[BufferSize] = {0};
        std::copy(payload.begin(), payload.end(), buffer);

        long values[3] {0, 0, 0};
        char* tok = std::strtok(buffer, ",");

        auto it = std::begin(values);
        while ((it != std::end(values)) && (tok != nullptr)) {
            char* endp { nullptr };
            auto value = std::strtol(tok, &endp, 10);
            if ((endp == tok) || (*endp != '\0')) {
                break;
            }

            (*it) = value;
            ++it;

            tok = std::strtok(nullptr, ",");
        }

        if (it != std::end(values)) {
            return;
        }

        lightHsv({values[0], values[1], values[2]});
    }
}

// Thanks to Sacha Telgenhof for sharing this code in his AiLight library
// https://github.com/stelgenhof/AiLight
// Color temperature is measured in mireds (kelvin = 1e6/mired)
long _toKelvin(long mireds) {
    return std::clamp(static_cast<long>(1000000L / mireds), _light_warm_kelvin, _light_cold_kelvin);
}

long _toMireds(long kelvin) {
    return std::clamp(static_cast<long>(lround(1000000L / kelvin)), _light_cold_mireds, _light_warm_mireds);
}

void _lightMireds(long kelvin) {
    _light_mireds = _toMireds(kelvin);
}

void _lightMiredsCCT(long kelvin) {
    _lightMireds(kelvin);

    const auto Range = _lightCctRange(espurna::light::ValueMax);
    _light_mapping.warm(Range.warm());
    _light_mapping.cold(Range.cold());
}

// TODO: is there a sane way to deduce this back from RGB variant?
// TODO: should mireds require CCT mode, so we only deal with white value?

#if 0

long _lightCCTMireds() {
    auto cold = static_cast<double>(_light_cold_mireds);
    auto warm = static_cast<double>(_light_warm_mireds);

    auto factor = (static_cast<double>(lightColdWhite()) / espurna::light::ValueMax);

    return cold + (factor * (warm - cold));
}

#endif

// TODO: function ptr like for input values?

void _fromKelvin(long kelvin) {
    // work through the brightness function instead of adjusting here
    // (but, note that +color +cct -white variant will set every rgb channel to 0)
    if (_light_use_color && _light_use_cct) {
        if (_light_use_white) {
            _lightMireds(kelvin);
        } else {
            _light_mapping.red(espurna::light::ValueMax);
            _light_mapping.green(espurna::light::ValueMax);
            _light_mapping.blue(espurna::light::ValueMax);
        }
        return;
    }

    if (!_light_use_color && _light_use_cct) {
        _lightMiredsCCT(kelvin);
        return;
    }

    // otherwise, only apply approximated color values
    kelvin /= 100;
    _light_mapping.red((kelvin <= 66)
        ? espurna::light::ValueMax
        : std::lround(329.698727446 * fs_pow(static_cast<double>(kelvin - 60), -0.1332047592)));
    _light_mapping.green((kelvin <= 66)
        ? std::lround(99.4708025861 * fs_log(kelvin) - 161.1195681661)
        : std::lround(288.1221695283 * fs_pow(static_cast<double>(kelvin), -0.0755148492)));
    _light_mapping.blue((kelvin >= 66)
        ? espurna::light::ValueMax
        : ((kelvin <= 19)
            ? espurna::light::ValueMin
            : std::lround(138.5177312231 * fs_log(static_cast<double>(kelvin - 10)) - 305.0447927307)));
    _lightMireds(kelvin);
}

void _fromMireds(long mireds) {
    _fromKelvin(_toKelvin(mireds));
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

void _lightFromGroupPayload(const char* payload) {
    if (!payload || *payload == '\0') {
        return;
    }

    constexpr size_t BufferSize { 32 };
    const size_t PayloadLen { strlen(payload) };

    if (PayloadLen < BufferSize) {
        char buffer[BufferSize] = {0};
        std::copy(payload, payload + PayloadLen, buffer);

        char* tok = std::strtok(buffer, ",");
        auto it = _light_channels.begin();

        while ((it != _light_channels.end()) && (tok != nullptr)) {
            char* endp { nullptr };
            auto value = std::strtol(tok, &endp, 10);
            if ((endp == tok) || (*endp != '\0')) {
                return;
            }

            (*it) = value;
            ++it;

            tok = std::strtok(nullptr, ",");
        }
    }
}

espurna::light::Hsv _lightHsv(espurna::light::Rgb rgb) {
    auto r = static_cast<double>(rgb.red()) / espurna::light::ValueMax;
    auto g = static_cast<double>(rgb.green()) / espurna::light::ValueMax;
    auto b = static_cast<double>(rgb.blue()) / espurna::light::ValueMax;

    auto max = std::max({r, g, b});
    auto min = std::min({r, g, b});

    auto v = max;

    if (min != max) {
        auto s = (max - min) / max;

        auto delta = max - min;
        auto rc = (max - r) / delta;
        auto gc = (max - g) / delta;
        auto bc = (max - b) / delta;

        double h { 0.0 };
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

        return espurna::light::Hsv(
            std::lround(h * 360.0),
            std::lround(s * 100.0),
            std::lround(v * 100.0));
    }

    return espurna::light::Hsv(espurna::light::Hsv::HueMin, espurna::light::Hsv::SaturationMin, v);

}

String _lightHsvPayload(espurna::light::Rgb rgb) {
    String out;
    out.reserve(12);

    auto hsv = _lightHsv(rgb);

    long values[3] {hsv.hue(), hsv.saturation(), hsv.value()};
    for (const auto& value : values) {
        if (out.length()) {
            out += ',';
        }
        out += value;
    }

    return out;
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
    if (operation.length()) {
        char* endp { nullptr };
        auto updated = std::strtol(operation.begin(), &endp, 10);
        if ((endp == operation.begin()) || (*endp != '\0')) {
            return value;
        }

        switch (operation[0]) {
        case '+':
        case '-':
            return updated + value;
        }

        return updated;
    }

    return value;
}

void _lightAdjustBrightness(espurna::StringView payload) {
    lightBrightness(_lightAdjustValue(_light_brightness, payload));
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
    _fromKelvin(_lightAdjustValue(_toKelvin(_light_mireds), payload));
}

void _lightAdjustMireds(espurna::StringView payload) {
    _fromMireds(_lightAdjustValue(_light_mireds, payload));
}

} // namespace

// -----------------------------------------------------------------------------
// PROVIDER
// -----------------------------------------------------------------------------

namespace {

// Gamma Correction lookup table (8 bit, ~2.2)
// TODO: input value modifier, instead of a transition-only thing?
// TODO: calculate on the fly instead of limiting this to an 8bit value?

constexpr long LightGammaMin { 0 };
constexpr long LightGammaMax { 255 };

long _lightGammaMap(size_t index) {
    const static std::array<uint8_t, 256> Gamma PROGMEM {
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
        return _lightGammaMap(static_cast<size_t>(Scaled));
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

    void start(Duration duration) {
        _timer.once(duration,
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

void _lightProviderSchedule(espurna::duration::Milliseconds);

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

    if (next) {
        _lightProviderSchedule(_light_transition->step());
    } else {
        _light_transition.reset(nullptr);
    }

    _light_provider_update.stop();
}

void _lightProviderSchedule(espurna::duration::Milliseconds next) {
    _light_provider_update.start(next);
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
        _mireds = (value >> (8ull * 6ull)) & 0xffffull;
        _brightness = (value >> (8ull * 5ull)) & 0xffull;

        _values[4] = ((value >> (8ull * 4ull)) & 0xffull);
        _values[3] = ((value >> (8ull * 3ull)) & 0xffull);
        _values[2] = ((value >> (8ull * 2ull)) & 0xffull);
        _values[1] = ((value >> (8ull * 1ull)) & 0xffull);
        _values[0] = ((value & 0xffull));
    }

    LightRtcmem(const LightValues& values, long brightness, long mireds) :
        _values(values),
        _brightness(brightness),
        _mireds(mireds)
    {}

    uint64_t serialize() const {
        return ((static_cast<uint64_t>(_mireds) & 0xffffull) << (8ull * 6ull))
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

    long mireds() const {
        return _mireds;
    }

private:
    LightValues _values = defaultValues();
    long _brightness { espurna::light::BrightnessMax };
    long _mireds { espurna::light::MiredsDefault };
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

    LightRtcmem light(values, _light_brightness, _light_mireds);
    Rtcmem->light = light.serialize();
}

void _lightRestoreRtcmem() {
    uint64_t value = Rtcmem->light;
    LightRtcmem light(value);

    const auto& values = light.values();
    for (size_t channel = 0; channel < _light_channels.size(); ++channel) {
        _light_channels[channel] = values[channel];
    }

    _light_mireds = light.mireds(); // channels are already set
    lightBrightness(light.brightness());
}

void _lightSaveSettings() {
    if (!_light_save) {
        return;
    }

    for (size_t channel = 0; channel < _light_channels.size(); ++channel) {
        espurna::light::settings::value(channel, _light_channels[channel].inputValue);
    }

    espurna::light::settings::brightness(_light_brightness);
    espurna::light::settings::mireds(_light_mireds);

    saveSettings();
}

void _lightRestoreSettings() {
    for (size_t channel = 0; channel < _light_channels.size(); ++channel) {
        _light_channels[channel] = espurna::light::settings::value(channel);
    }

    _light_mireds = espurna::light::settings::mireds();
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
    return tryParseId(value, lightChannels, id);
}

} // namespace

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

namespace {

int _lightMqttReportMask() {
    return espurna::light::DefaultReport & ~(static_cast<int>(mqttForward() ? espurna::light::Report::None : espurna::light::Report::Mqtt));
}

int _lightMqttReportGroupMask() {
    return _lightMqttReportMask() & ~static_cast<int>(espurna::light::Report::MqttGroup);
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

void _lightMqttCallback(unsigned int type, const char* topic, char* payload) {
    String mqtt_group_color = espurna::light::settings::mqttGroup();

    if (type == MQTT_CONNECT_EVENT) {

        mqttSubscribe(MQTT_TOPIC_TRANSITION);

        mqttSubscribe(MQTT_TOPIC_CHANNEL "/+");
        mqttSubscribe(MQTT_TOPIC_BRIGHTNESS);

        if (!_light_has_controls) {
            mqttSubscribe(MQTT_TOPIC_LIGHT);
        }

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
        if ((mqtt_group_color.length() > 0) && (mqtt_group_color.equals(topic))) {
            _lightFromGroupPayload(payload);
            _lightUpdateFromMqttGroup();
            return;
        }

        // Match topic
        String t = mqttMagnitude(topic);

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

        // Transition setting
        if (t.equals(MQTT_TOPIC_TRANSITION)) {
            char* endp { nullptr };
            auto result = strtoul(payload, &endp, 10);
            if (!endp || (endp == payload)) {
                return;
            }

            lightTransition(
                espurna::duration::Milliseconds(result),
                _light_transition_step);
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
            if (_lightTryParseChannel(mqttMagnitudeTail(t, MQTT_TOPIC_CHANNEL), id)) {
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
    if (_light_use_color) {
        const auto rgb = _lightToTargetRgb();
        mqttSend(MQTT_TOPIC_COLOR_HEX, _lightRgbHexPayload(rgb).c_str());
        mqttSend(MQTT_TOPIC_COLOR_RGB, _lightRgbPayload(rgb).c_str());
        mqttSend(MQTT_TOPIC_COLOR_HSV, _lightHsvPayload(rgb).c_str());
    }

    if (_light_use_color || _light_use_cct) {
        mqttSend(MQTT_TOPIC_MIRED, String(_light_mireds).c_str());
    }

    for (size_t channel = 0; channel < _light_channels.size(); ++channel) {
        mqttSend(MQTT_TOPIC_CHANNEL, channel, String(_light_channels[channel].target).c_str());
    }

    mqttSend(MQTT_TOPIC_BRIGHTNESS, String(_light_brightness).c_str());

    if (!_light_has_controls) {
        mqttSend(MQTT_TOPIC_LIGHT, _light_state ? "1" : "0");
    }
}

void lightMQTTGroup() {
    const String mqtt_group_color = espurna::light::settings::mqttGroup();
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
                request.send(String(_light_mireds));
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
                request.send(String(_toKelvin(_light_mireds)));
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
            auto value = request.param(F("value"));

            const char* p { value.c_str() };
            char* endp { nullptr };

            auto result = strtoul(p, &endp, 10);
            if (!endp || (endp == p)) {
                return false;
            }

            lightTransition(
                espurna::duration::Milliseconds(result),
                _light_transition_step);

            return true;
        }
    );

    apiRegister(F(MQTT_TOPIC_BRIGHTNESS),
        [](ApiRequest& request) {
            request.send(String(static_cast<int>(_light_brightness)));
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
                request.send(String(static_cast<int>(_light_channels[id].target)));
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

    if (!_light_has_controls) {
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
}

} // namespace

#endif // API_SUPPORT

#if WEB_SUPPORT

namespace {

bool _lightWebSocketOnKeyCheck(espurna::StringView key, const JsonVariant&) {
    return espurna::settings::query::samePrefix(key, STRING_VIEW("light"))
        || espurna::settings::query::samePrefix(key, STRING_VIEW("use"))
        || espurna::settings::query::samePrefix(key, STRING_VIEW("lt"));
}

void _lightWebSocketStatus(JsonObject& root) {
    if (_light_use_color) {
        if (_light_use_rgb) {
            root["rgb"] = _lightRgbHexPayload(_lightToInputRgb());
        } else {
            root["hsv"] = _lightHsvPayload(_lightToTargetRgb());
        }
    }

    if (_light_use_cct) {
        JsonObject& mireds = root.createNestedObject("mireds");
        mireds["value"] = _light_mireds;
        mireds["cold"] = _light_cold_mireds;
        mireds["warm"] = _light_warm_mireds;
        root["useCCT"] = _light_use_cct;
    }

    JsonArray& channels = root.createNestedArray("channels");
    for (auto& channel : _light_channels) {
        channels.add(channel.inputValue);
    }

    root["brightness"] = _light_brightness;
    root["lightstate"] = _light_state;
}

void _lightWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, PSTR("light"));
}

void _lightWebSocketOnConnected(JsonObject& root) {
    root["mqttGroupColor"] = espurna::light::settings::mqttGroup();
    root["useColor"] = _light_use_color;
    root["useWhite"] = _light_use_white;
    root["useGamma"] = _light_use_gamma;
    root["useTransitions"] = _light_use_transitions;
    root["useRGB"] = _light_use_rgb;
    root["ltSave"] = _light_save;
    root["ltSaveDelay"] = _light_save_delay.count();
    root["ltTime"] = _light_transition_time.count();
    root["ltStep"] = _light_transition_step.count();
#if RELAY_SUPPORT
    root["ltRelay"] = espurna::light::settings::relay();
#endif
}

void _lightWebSocketOnAction(uint32_t client_id, const char* action, JsonObject& data) {
    STRING_VIEW_INLINE(Color, "color");

    if (_light_has_color) {
        if (Color == action) {
            STRING_VIEW_INLINE(Rgb, "rgb");
            STRING_VIEW_INLINE(Hsv, "hsv");

            if (data.containsKey(Rgb)) {
                _lightFromRgbPayload(data[Rgb].as<espurna::StringView>());
                lightUpdate();
            } else if (data.containsKey(Hsv)) {
                _lightFromHsvPayload(data[Hsv].as<espurna::StringView>());
                lightUpdate();
            }
        }
    }

    STRING_VIEW_INLINE(Mireds, "mireds");
    STRING_VIEW_INLINE(Brightness, "brightness");
    STRING_VIEW_INLINE(Id, "id");
    STRING_VIEW_INLINE(Channel, "channel");
    STRING_VIEW_INLINE(Value, "value");

    if (Mireds == action) {
        if (data.containsKey(Mireds)) {
            _fromMireds(data[Mireds].as<long>());
            lightUpdate();
        }
    } else if (Channel == action) {
        if (data.containsKey(Id) && data.containsKey(Value)) {
            lightChannel(data[Id].as<size_t>(), data[Value].as<long>());
            lightUpdate();
        }
    } else if (Brightness == action) {
        if (data.containsKey(Value)) {
            lightBrightness(data[Value].as<long>());
            lightUpdate();
        }
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

    out.brightness = _light_brightness;
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

alignas(4) static constexpr char LightCommandNotify[] PROGMEM = "NOTIFY";

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

alignas(4) static constexpr char LightCommand[] PROGMEM = "LIGHT";

static void _lightCommand(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        if (!_lightParsePayload(ctx.argv[1])) {
            terminalError(ctx, F("Invalid payload"));
            return;
        }
        lightUpdate();
    }

    ctx.output.printf("%s\n", _light_state ? "ON" : "OFF");
    terminalOK(ctx);
}

alignas(4) static constexpr char LightCommandBrightness[] PROGMEM = "BRIGHTNESS";

static void _lightCommandBrightness(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        _lightAdjustBrightness(ctx.argv[1]);
        lightUpdate();
    }
    ctx.output.printf("%ld\n", _light_brightness);
    terminalOK(ctx);
}

alignas(4) static constexpr char LightCommandChannel[] PROGMEM = "CHANNEL";

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

alignas(4) static constexpr char LightCommandRgb[] PROGMEM = "RGB";

static void _lightCommandRgb(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        _lightFromRgbPayload(ctx.argv[1]);
        lightUpdate();
    }

    ctx.output.printf_P(PSTR("rgb %s\n"),
        _lightRgbPayload(_lightToTargetRgb()).c_str());
    terminalOK(ctx);
}

alignas(4) static constexpr char LightCommandHsv[] PROGMEM = "HSV";

static void _lightCommandHsv(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        _lightFromHsvPayload(ctx.argv[1]);
        lightUpdate();
    }

    ctx.output.printf_P(PSTR("hsv %s\n"),
        _lightHsvPayload().c_str());
    terminalOK(ctx);
}

alignas(4) static constexpr char LightCommandKelvin[] PROGMEM = "KELVIN";

static void _lightCommandKelvin(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        _lightAdjustKelvin(ctx.argv[1]);
        lightUpdate();
    }

    ctx.output.printf_P(PSTR("kelvin %ld\n"),
        _toKelvin(_light_mireds));
    terminalOK(ctx);
}

alignas(4) static constexpr char LightCommandMired[] PROGMEM = "MIRED";

static void _lightCommandMired(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() > 1) {
        _lightAdjustMireds(ctx.argv[1]);
        lightUpdate();
    }

    ctx.output.printf_P(PSTR("mireds %ld\n"), _light_mireds);
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

// HSV to RGB transformation -----------------------------------------------
//
// INPUT: [0,100,57]
// IS: [145,0,0]
// SHOULD: [255,0,0]

void lightHsv(espurna::light::Hsv hsv) {
    double r { 0.0 };
    double g { 0.0 };
    double b { 0.0 };

    auto v = static_cast<double>(hsv.value()) / 100.0;
    long brightness { std::lround(v * static_cast<double>(espurna::light::BrightnessMax)) };

    if (hsv.saturation()) {
        auto h = hsv.hue();
        if (h < 0) {
            h = 0;
        } else if (h >= 360) {
            h = 359;
        }

        auto s = static_cast<double>(hsv.saturation()) / 100.0;

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

        r = (r + m) * 255.0;
        g = (g + m) * 255.0;
        b = (b + m) * 255.0;
    } else {
        r = brightness;
        g = brightness;
        b = brightness;
    }

    _light_mapping.red(std::lround(r));
    _light_mapping.green(std::lround(g));
    _light_mapping.blue(std::lround(b));
    lightBrightness(brightness);
}

void lightHs(long hue, long saturation) {
    lightHsv({hue, saturation, espurna::light::Hsv::ValueMax});
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
    _light_process_input_values(_light_channels, _light_brightness);

    if (!_light_state_changed && !observer.changed()) {
        _light_update.cancel();
        return;
    }

    _light_state_changed = false;
    _light_update.run([](LightTransition transition, int report, bool save) {
        // Channel output values will be set by the handler class and the specified provider
        // We either set the values immediately or schedule an ongoing transition
        _light_transition = std::make_unique<LightTransitionHandler>(_light_channels, transition, _light_state);
        _lightProviderSchedule(_light_transition->step());
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

void _lightUpdate(LightTransition transition, espurna::light::Report report, bool save) {
    _lightUpdate(transition, static_cast<int>(report), save, false);
}

void _lightUpdate(LightTransition transition) {
    _lightUpdate(transition, espurna::light::DefaultReport, _light_save, false);
}

void _lightUpdate(bool save) {
    _lightUpdate(lightTransition(), espurna::light::DefaultReport, save, false);
}

} // namespace

void lightSequence(LightSequenceCallbacks callbacks) {
    _light_sequence = std::move(callbacks);
}

void lightUpdateSequence(LightTransition transition) {
    _lightUpdate(transition, 0, false, true);
}

void lightUpdate(LightTransition transition, int report, bool save) {
    _lightUpdate(transition, report, save);
}

void lightUpdate(LightTransition transition, espurna::light::Report report, bool save) {
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

void lightMireds(long mireds) {
    _fromMireds(mireds);
}

espurna::light::MiredsRange lightMiredsRange() {
    return { _light_cold_mireds, _light_warm_mireds };
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
    return _light_brightness;
}

void lightBrightnessPercent(long percent) {
    lightBrightness((percent / 100l) * espurna::light::BrightnessMax);
}

void lightBrightness(long brightness) {
    _light_brightness = std::clamp(brightness, espurna::light::BrightnessMin, espurna::light::BrightnessMax);
}

void lightBrightnessStep(long steps, long multiplier) {
    lightBrightness(static_cast<int>(_light_brightness) + (steps * multiplier));
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
    _light_use_cct = !_light_use_white
        && use_cct && has_cold_white && has_warm_white;
    if (!_light_use_cct) {
        espurna::light::settings::cct(false);
    }

    const auto last_process_input_values = _light_process_input_values;
    _light_process_input_values =
        (_light_use_color) ? (
            (_light_use_cct) ? _lightValuesWithRgbCct :
            (_light_use_white) ? _lightValuesWithRgbWhite :
            _lightValuesWithBrightnessExceptWhite) :
        _lightValuesWithBrightness;

    _light_use_rgb = espurna::light::settings::rgb();

    // TODO: provide single entrypoint for colortemp
    _light_cold_mireds = espurna::light::settings::miredsCold();
    _light_warm_mireds = espurna::light::settings::miredsWarm();
    _light_cold_kelvin = (1000000L / _light_cold_mireds);
    _light_warm_kelvin = (1000000L / _light_warm_mireds);

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

    if (!_light_update && (last_process_input_values != _light_process_input_values)) {
        lightUpdate(false);
    }
}

#if RELAY_SUPPORT

void _lightRelayBoot() {
    if (_light_has_controls) {
        return;
    }

    auto next_id = relayCount();
    if (relayAdd(std::make_unique<LightGlobalProvider>())) {
        _light_state_listener = [next_id](bool state) {
            relayStatus(next_id, state);
        };
        _light_has_controls = true;
    }
}

#endif

void _lightBoot() {
    const size_t Channels { _light_channels.size() };
    if (Channels) {
        DEBUG_MSG_P(PSTR("[LIGHT] Number of channels: %u\n"), Channels);

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
}

} // namespace

// -----------------------------------------------------------------------------

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
            _my92xx = new my92xx(
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

#if RELAY_SUPPORT
    if (espurna::light::settings::relay()) {
        _lightRelayBoot();
    }
#endif

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

    espurnaRegisterReload(_lightConfigure);
    espurnaRegisterLoop([]() {
        _lightSequenceCheck();
        _lightUpdate();
        _lightProviderUpdate();
        _lightPostLoop();
    });
}

#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
