/*

PWM MODULE

Copyright (C) 2019-2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstddef>
#include <cstdint>

// Associate list of PINs with CHANNELs
bool pwmInitPins(const uint8_t* begin, const uint8_t* end);

template <typename T>
bool pwmInitPins(const T& pins) {
    return pwmInitPins(pins.data(), pins.data() + pins.size());
}

// PWM driver accepts certain value range
struct PwmRange {
    uint32_t min;
    uint32_t max;
};

// u32 values accepted by pwmDuty()
PwmRange pwmRange();

// Number of configured PWM channels
size_t pwmChannels();

// Update specific channel with raw value (see pwmRange())
void pwmDuty(size_t channel, uint32_t duty);

// Or, using a percentage value
void pwmDuty(size_t channel, float duty);

// Apply all channel duty values
void pwmUpdate();

// Configure driver. Should be called *before* initializing any pins.
void pwmSetup();
