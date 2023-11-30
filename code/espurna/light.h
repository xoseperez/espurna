// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

#pragma once

#include "espurna.h"

#define MQTT_TOPIC_LIGHT            "light"
#define MQTT_TOPIC_LIGHT_JSON       "light_json"
#define MQTT_TOPIC_CHANNEL          "channel"
#define MQTT_TOPIC_COLOR_RGB        "rgb"
#define MQTT_TOPIC_COLOR_HEX        "hex"
#define MQTT_TOPIC_COLOR_HSV        "hsv"
#define MQTT_TOPIC_ANIM_MODE        "anim_mode"
#define MQTT_TOPIC_ANIM_SPEED       "anim_speed"
#define MQTT_TOPIC_BRIGHTNESS       "brightness"
#define MQTT_TOPIC_MIRED            "mired"
#define MQTT_TOPIC_KELVIN           "kelvin"
#define MQTT_TOPIC_TRANSITION       "transition"

namespace espurna {
namespace light {

constexpr size_t ChannelsMax { 5 };

constexpr long ValueStep { LIGHT_STEP };

// TODO: separate internal and external scaling?
// TODO: allow wider value range than just 8bit?

constexpr long ValueMin { LIGHT_MIN_VALUE };
constexpr long ValueMax { LIGHT_MAX_VALUE };

constexpr long BrightnessMin { LIGHT_MIN_BRIGHTNESS };
constexpr long BrightnessMax { LIGHT_MAX_BRIGHTNESS };

constexpr long MiredsCold { LIGHT_COLDWHITE_MIRED };
constexpr long MiredsWarm { LIGHT_WARMWHITE_MIRED };

struct Report {
    static constexpr int None = 0;
    static constexpr int Web = 1 << 0;
    static constexpr int Mqtt = 1 << 1;
    static constexpr int MqttGroup = 1 << 2;

    static constexpr int Default { Web | Mqtt | MqttGroup };
};

struct Hsv {
    static constexpr long HueMin { 0 };
    static constexpr long HueMax { 360 };

    static constexpr long SaturationMin { 0 };
    static constexpr long SaturationMax { 100 };

    static constexpr long ValueMin { 0 };
    static constexpr long ValueMax { 100 };

    using Array = std::array<long, 3>;

    Hsv() = default;
    Hsv(const Hsv&) = default;
    Hsv(Hsv&&) = default;

    Hsv& operator=(const Hsv&) = default;
    Hsv& operator=(Hsv&&) = default;

#if __cplusplus > 201103L
    constexpr
#endif
    explicit Hsv(Array array) noexcept :
        _hue(array[0]),
        _saturation(array[1]),
        _value(array[2])
    {}

    constexpr Hsv(long hue, long saturation, long value) noexcept :
        _hue(std::clamp(hue, HueMin, HueMax)),
        _saturation(std::clamp(saturation, SaturationMin, SaturationMax)),
        _value(std::clamp(value, ValueMin, ValueMax))
    {}

    constexpr long hue() const {
        return _hue;
    }

    constexpr long saturation() const {
        return _saturation;
    }

    constexpr long value() const {
        return _value;
    }

    constexpr Array asArray() const {
        return Array{{_hue, _saturation, _value}};
    }

private:
    long _hue { HueMin };
    long _saturation { SaturationMin };
    long _value { ValueMin };
};

struct Rgb {
    static constexpr long Min { 0 };
    static constexpr long Max { 255 };

    Rgb() = default;
    Rgb(const Rgb&) = default;
    Rgb(Rgb&&) = default;

    Rgb& operator=(const Rgb&) = default;
    Rgb& operator=(Rgb&&) = default;

    constexpr Rgb(long red, long green, long blue) noexcept :
        _red(std::clamp(red, Min, Max)),
        _green(std::clamp(green, Min, Max)),
        _blue(std::clamp(blue, Min, Max))
    {}

    constexpr long red() const {
        return _red;
    }

    constexpr long green() const {
        return _green;
    }

    constexpr long blue() const {
        return _blue;
    }

private:
    long _red { Min };
    long _green { Min };
    long _blue { Min };
};

struct Mireds {
    long value;
};

struct Kelvin {
    long value;
};

struct TemperatureRange {
    TemperatureRange() = delete;
    constexpr TemperatureRange(long cold, long warm) noexcept :
        _cold(cold),
        _warm(warm)
    {}

    constexpr long cold() const {
        return _cold;
    }

    constexpr long warm() const {
        return _warm;
    }

private:
    long _cold;
    long _warm;
};

} // namespace light
} // namespace espurna

using LightStateListener = std::function<void(bool)>;
using LightReportListener = void(*)();

class LightProvider {
public:
    virtual void update() = 0;
    virtual void state(bool) = 0;
    virtual void channel(size_t ch, float value) = 0;
};

struct LightTransition {
    espurna::duration::Milliseconds time;
    espurna::duration::Milliseconds step;
};

size_t lightChannels();

LightTransition lightTransition();

espurna::duration::Milliseconds lightTransitionTime();
espurna::duration::Milliseconds lightTransitionStep();

// Transition from current state to the previously prepared one
// (using any of functions declared down below which modify global state, channel values or their state)
void lightTransition(espurna::duration::Milliseconds time, espurna::duration::Milliseconds step);
void lightTransition(LightTransition);

// Light internals are forced to be sequential. In case some actions need to happen
// right after transition / channel state / state changes, it will call these functions
using LightSequenceCallbacks = std::forward_list<espurna::Callback>;

void lightSequence(LightSequenceCallbacks);
void lightUpdateSequence(LightTransition);

void lightParseHsv(espurna::StringView);
void lightParseRgb(espurna::StringView);

String lightRgbPayload();
String lightHsvPayload();
String lightColor();

bool lightSave();
void lightSave(bool save);

espurna::light::Rgb lightRgb();
void lightRgb(espurna::light::Rgb);

espurna::light::Hsv lightHsv();
void lightHs(long hue, long saturation);
void lightHsv(espurna::light::Hsv);

void lightTemperature(espurna::light::Kelvin);
void lightTemperature(espurna::light::Mireds);
espurna::light::TemperatureRange lightMiredsRange();

void lightRed(long value);
long lightRed();

void lightGreen(long value);
long lightGreen();

void lightBlue(long value);
long lightBlue();

void lightColdWhite(long value);
long lightColdWhite();

void lightWarmWhite(long value);
long lightWarmWhite();

void lightState(size_t id, bool state);
bool lightState(size_t id);

void lightState(bool state);
bool lightState();

// TODO: overload with struct Percent { ... }, struct Brightness { ... }, etc.
void lightBrightnessPercent(long percent);
void lightBrightness(long brightness);
long lightBrightness();

long lightChannel(size_t id);
void lightChannel(size_t id, long value);

void lightBrightnessStep(long steps);
void lightBrightnessStep(long steps, long multiplier);

void lightChannelStep(size_t id, long steps);
void lightChannelStep(size_t id, long steps, long multiplier);

void lightUpdate(LightTransition transition, int report, bool save);
void lightUpdate(LightTransition transition);
void lightUpdate(bool save);
void lightUpdate();

bool lightHasColor();
bool lightHasWarmWhite();
bool lightHasColdWhite();
bool lightHasWhite();

bool lightUseRGB();
bool lightUseCCT();

void lightMQTT();
void lightOnReport(LightReportListener);

void lightSetProvider(std::unique_ptr<LightProvider>&&);
bool lightAdd();

class RelayProviderBase;
std::unique_ptr<RelayProviderBase> lightMakeStateRelayProvider(size_t);

void lightSetup();
