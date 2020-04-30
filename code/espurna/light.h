// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

#pragma once

#include "espurna.h"

// TODO: lowercase
namespace Light {
    constexpr size_t ChannelsMax = 5;

    constexpr long VALUE_MIN = LIGHT_MIN_VALUE;
    constexpr long VALUE_MAX = LIGHT_MAX_VALUE;

    constexpr long BRIGHTNESS_MIN = LIGHT_MIN_BRIGHTNESS;
    constexpr long BRIGHTNESS_MAX = LIGHT_MAX_BRIGHTNESS;

    constexpr long PWM_MIN = LIGHT_MIN_PWM;
    constexpr long PWM_MAX = LIGHT_MAX_PWM;
    constexpr long PWM_LIMIT = LIGHT_LIMIT_PWM;

    enum Communications : unsigned char {
        COMMS_NONE = 0,
        COMMS_NORMAL = 1 << 0,
        COMMS_GROUP = 1 << 1
    };
}

size_t lightChannels();
unsigned int lightTransitionTime();
void lightTransitionTime(unsigned long ms);

void lightColor(const char * color, bool rgb);
void lightColor(const char * color);
void lightColor(unsigned long color);
String lightColor(bool rgb);
String lightColor();

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

void lightUpdate(bool save, bool forward, bool group_forward);
void lightUpdate(bool save, bool forward);

bool lightHasColor();
bool lightUseCCT();

void lightMQTT();
void lightSetupChannels(unsigned char size);

void lightSetup();
