// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

#pragma once

#include "espurna.h"

#define MQTT_TOPIC_LIGHT            "light"
#define MQTT_TOPIC_LIGHT_JSON       "light_json"
#define MQTT_TOPIC_CHANNEL          "channel"
#define MQTT_TOPIC_COLOR_RGB        "rgb"
#define MQTT_TOPIC_COLOR_HSV        "hsv"
#define MQTT_TOPIC_ANIM_MODE        "anim_mode"
#define MQTT_TOPIC_ANIM_SPEED       "anim_speed"
#define MQTT_TOPIC_BRIGHTNESS       "brightness"
#define MQTT_TOPIC_MIRED            "mired"
#define MQTT_TOPIC_KELVIN           "kelvin"
#define MQTT_TOPIC_TRANSITION       "transition"

// TODO: lowercase
namespace Light {

constexpr size_t Channels = LIGHT_CHANNELS;
constexpr size_t ChannelsMax = 5;

constexpr long VALUE_MIN = LIGHT_MIN_VALUE;
constexpr long VALUE_MAX = LIGHT_MAX_VALUE;

constexpr long BRIGHTNESS_MIN = LIGHT_MIN_BRIGHTNESS;
constexpr long BRIGHTNESS_MAX = LIGHT_MAX_BRIGHTNESS;

constexpr long PWM_MIN = LIGHT_MIN_PWM;
constexpr long PWM_MAX = LIGHT_MAX_PWM;
constexpr long PWM_LIMIT = LIGHT_LIMIT_PWM;

enum class Report {
    None = 0,
    Web = 1 << 0,
    Mqtt = 1 << 1,
    MqttGroup = 1 << 2,
    Broker = 1 << 3
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
    Report::Web | Report::Mqtt | Report::MqttGroup | Report::Broker
};

} // namespace Light

using LightStateListener = std::function<void(bool)>;
using LightReportListener = void(*)();

class LightProvider {
public:
    virtual void update() = 0;
    virtual void state(bool) = 0;
    virtual void channel(unsigned char ch, double value) = 0;
};

struct LightTransition {
    unsigned long time;
    unsigned long step;
};

size_t lightChannels();

LightTransition lightTransition();

unsigned long lightTransitionTime();
unsigned long lightTransitionStep();

void lightTransition(unsigned long time, unsigned long step);
void lightTransition(LightTransition transition);

void lightColor(const char* color, bool rgb);
void lightColor(const String& color, bool rgb);

void lightColor(const String& color);
void lightColor(const char* color);

void lightColor(unsigned long color);
String lightColor(bool rgb);
String lightColor();

bool lightSave();
void lightSave(bool save);

void lightState(unsigned char i, bool state);
bool lightState(unsigned char i);

void lightState(bool state);
bool lightState();

void lightBrightness(long brightness);
long lightBrightness();

long lightChannel(unsigned char id);
void lightChannel(unsigned char id, long value);

void lightBrightnessStep(long steps, long multiplier = LIGHT_STEP);
void lightChannelStep(unsigned char id, long steps, long multiplier = LIGHT_STEP);

void lightUpdate(bool save, LightTransition transition, Light::Report report);
void lightUpdate(bool save, LightTransition transition, int report);
void lightUpdate(LightTransition transition);
void lightUpdate(bool save);
void lightUpdate();

bool lightHasColor();
bool lightUseCCT();

void lightMQTT();

void lightSetReportListener(LightReportListener);
void lightSetStateListener(LightStateListener);
void lightSetProvider(std::unique_ptr<LightProvider>&&);
bool lightAdd();

void lightSetup();
