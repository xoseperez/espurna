/*

LIGHT MODULE

*/

#pragma once

#include "espurna.h"

namespace Light {
namespace build {

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

#ifdef MY92XX_MAPPING

constexpr unsigned char _my92xx_mapping[MY92XX_CHANNELS] {
    MY92XX_MAPPING
};

constexpr unsigned char my92xxChannel(size_t)
    __attribute__((deprecated("MY92XX_CH# flags should be used instead of MY92XX_MAPPING")));
constexpr unsigned char my92xxChannel(size_t channel) {
    return _my92xx_mapping[channel];
}

#else

constexpr unsigned char my92xxChannel(size_t channel) {
    return (channel == 0) ? MY92XX_CH1 :
        (channel == 1) ? MY92XX_CH2 :
        (channel == 2) ? MY92XX_CH3 :
        (channel == 3) ? MY92XX_CH4 :
        (channel == 4) ? MY92XX_CH5 : Light::ChannelsMax;
}

#endif

#endif

} // namespace build
} // namespace Light
