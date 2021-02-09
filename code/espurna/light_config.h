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

constexpr unsigned char channelPin(unsigned char index) {
    return (
        (index == 0) ? LIGHT_CH1_PIN :
        (index == 1) ? LIGHT_CH2_PIN :
        (index == 2) ? LIGHT_CH3_PIN :
        (index == 3) ? LIGHT_CH4_PIN :
        (index == 4) ? LIGHT_CH5_PIN : GPIO_NONE
    );
}

constexpr bool inverse(unsigned char index) {
    return (
        (index == 0) ? (1 == LIGHT_CH1_INVERSE) :
        (index == 1) ? (1 == LIGHT_CH2_INVERSE) :
        (index == 2) ? (1 == LIGHT_CH3_INVERSE) :
        (index == 3) ? (1 == LIGHT_CH4_INVERSE) :
        (index == 4) ? (1 == LIGHT_CH5_INVERSE) : false
    );
}

#ifdef MY92XX_MAPPING

constexpr unsigned char _my92xx_mapping[LIGHT_CHANNELS] {
    MY92XX_MAPPING
};

constexpr unsigned char my92xxChannel(unsigned char)
    __attribute__((deprecated("MY92XX_CH# flags should be used instead of MY92XX_MAPPING")));
constexpr unsigned char _lightMy92xxChannel(unsigned char channel) {
    return _my92xx_mapping[channel];
}

#else

constexpr unsigned char my92xxChannel(unsigned char channel) {
    return (channel == 0) ? MY92XX_CH1 :
        (channel == 1) ? MY92XX_CH2 :
        (channel == 2) ? MY92XX_CH3 :
        (channel == 3) ? MY92XX_CH4 :
        (channel == 4) ? MY92XX_CH5 : 255u;
}

} // namespace build
} // namespace Light

#endif
