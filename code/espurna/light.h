// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

#pragma once

namespace Light {
    constexpr const long VALUE_MIN = LIGHT_MIN_VALUE;
    constexpr const long VALUE_MAX = LIGHT_MAX_VALUE;

    constexpr const long BRIGHTNESS_MIN = LIGHT_MIN_BRIGHTNESS;
    constexpr const long BRIGHTNESS_MAX = LIGHT_MAX_BRIGHTNESS;

    // Default to the Philips Hue value that HA also use.
    // https://developers.meethue.com/documentation/core-concepts
    constexpr const long MIREDS_COLDWHITE = LIGHT_COLDWHITE_MIRED;
    constexpr const long MIREDS_WARMWHITE = LIGHT_WARMWHITE_MIRED;

    constexpr const long KELVIN_WARMWHITE = LIGHT_WARMWHITE_KELVIN;
    constexpr const long KELVIN_COLDWHITE = LIGHT_COLDWHITE_KELVIN;

    constexpr const long PWM_MIN = LIGHT_MIN_PWM;
    constexpr const long PWM_MAX = LIGHT_MAX_PWM;
    constexpr const long PWM_LIMIT = LIGHT_LIMIT_PWM;

    enum Communications : unsigned char {
        COMMS_NONE = 0,
        COMMS_NORMAL = 1 << 0,
        COMMS_GROUP = 1 << 1
    };
}

size_t lightChannels();

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
