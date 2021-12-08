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

// TODO: lowercase
namespace Light {

constexpr size_t ChannelsMax { 5 };

constexpr long ValueStep { LIGHT_STEP };

constexpr long ValueMin { LIGHT_MIN_VALUE };
constexpr long ValueMax { LIGHT_MAX_VALUE };

constexpr long BrightnessMin { LIGHT_MIN_BRIGHTNESS };
constexpr long BrightnessMax { LIGHT_MAX_BRIGHTNESS };

constexpr long MiredsCold { LIGHT_COLDWHITE_MIRED };
constexpr long MiredsWarm { LIGHT_WARMWHITE_MIRED };

constexpr long PwmMin { LIGHT_MIN_PWM };
constexpr long PwmMax { LIGHT_MAX_PWM };
constexpr long PwmLimit { LIGHT_LIMIT_PWM };

enum class Report {
    None = 0,
    Web = 1 << 0,
    Mqtt = 1 << 1,
    MqttGroup = 1 << 2
};

constexpr int operator|(Report lhs, int rhs) {
    return static_cast<int>(lhs) | rhs;
}

constexpr int operator|(int lhs, Report rhs) {
    return lhs | static_cast<int>(rhs);
}

constexpr int operator|(Report lhs, Report rhs) {
    return static_cast<int>(lhs) | static_cast<int>(rhs);
}

constexpr int operator&(int lhs, Report rhs) {
    return lhs & static_cast<int>(rhs);
}

constexpr int operator&(Report lhs, int rhs) {
    return static_cast<int>(lhs) & rhs;
}

constexpr int DefaultReport {
    Report::Web | Report::Mqtt | Report::MqttGroup
};

struct Hsv {
    static constexpr long HueMin { 0 };
    static constexpr long HueMax { 360 };

    static constexpr long SaturationMin { 0 };
    static constexpr long SaturationMax { 100 };

    static constexpr long ValueMin { 0 };
    static constexpr long ValueMax { 100 };

    Hsv() = default;
    Hsv(const Hsv&) = default;
    Hsv(Hsv&&) = default;

    Hsv& operator=(const Hsv&) = default;
    Hsv& operator=(Hsv&&) = default;

    Hsv(long hue, long saturation, long value) :
        _hue(std::clamp(hue, HueMin, HueMax)),
        _saturation(std::clamp(saturation, SaturationMin, SaturationMax)),
        _value(std::clamp(value, ValueMin, ValueMax))
    {}

    long hue() const {
        return _hue;
    }

    long saturation() const {
        return _saturation;
    }

    long value() const {
        return _value;
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

    Rgb(long red, long green, long blue) :
        _red(std::clamp(red, Min, Max)),
        _green(std::clamp(green, Min, Max)),
        _blue(std::clamp(blue, Min, Max))
    {}

    long red() const {
        return _red;
    }

    long green() const {
        return _green;
    }

    long blue() const {
        return _blue;
    }

private:
    long _red { Min };
    long _green { Min };
    long _blue { Min };
};

struct MiredsRange {
    constexpr MiredsRange() = default;
    MiredsRange(long cold, long warm) :
        _cold(cold),
        _warm(warm)
    {}

    long cold() const {
        return _cold;
    }

    long warm() const {
        return _warm;
    }

private:
    long _cold { MiredsCold };
    long _warm { MiredsWarm };
};

} // namespace Light

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

void lightTransition(espurna::duration::Milliseconds time, espurna::duration::Milliseconds step);
void lightTransition(LightTransition transition);

void lightColor(const char* color, bool rgb);
void lightColor(const String& color, bool rgb);

void lightColor(const char* color);
void lightColor(const String& color);

String lightRgbPayload();
String lightHsvPayload();
String lightColor();

bool lightSave();
void lightSave(bool save);

Light::Rgb lightRgb();
void lightRgb(Light::Rgb);

Light::Hsv lightHsv();
void lightHs(long hue, long saturation);
void lightHsv(Light::Hsv);

void lightMireds(long mireds);
Light::MiredsRange lightMiredsRange();

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

void lightUpdate(LightTransition transition, Light::Report report, bool save);
void lightUpdate(LightTransition transition, int report, bool save);
void lightUpdate(LightTransition transition);
void lightUpdate(bool save);
void lightUpdate();

bool lightHasColor();
bool lightUseRGB();
bool lightUseCCT();

void lightMQTT();
void lightOnReport(LightReportListener);

void lightSetProvider(std::unique_ptr<LightProvider>&&);
bool lightAdd();

void lightSetup();
