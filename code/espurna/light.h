// -----------------------------------------------------------------------------
// Light
// -----------------------------------------------------------------------------

#pragma once

namespace Light {
    constexpr const unsigned char VALUE_MIN = LIGHT_MIN_VALUE;
    constexpr const unsigned char VALUE_MAX = LIGHT_MAX_VALUE;

    constexpr const unsigned int BRIGHTNESS_MIN = LIGHT_MIN_BRIGHTNESS;
    constexpr const unsigned int BRIGHTNESS_MAX = LIGHT_MAX_BRIGHTNESS;

    // Default to the Philips Hue value that HA also use.
    // https://developers.meethue.com/documentation/core-concepts
    constexpr const unsigned int MIREDS_COLDWHITE = LIGHT_COLDWHITE_MIRED;
    constexpr const unsigned int MIREDS_WARMWHITE = LIGHT_WARMWHITE_MIRED;

    constexpr const unsigned int KELVIN_WARMWHITE = LIGHT_WARMWHITE_KELVIN;
    constexpr const unsigned int KELVIN_COLDWHITE = LIGHT_COLDWHITE_KELVIN;

    constexpr const unsigned int PWM_MIN = LIGHT_MIN_PWM;
    constexpr const unsigned int PWM_MAX = LIGHT_MAX_PWM;
    constexpr const unsigned int PWM_LIMIT = LIGHT_LIMIT_PWM;
}

