/*

LED MODULE

*/

#pragma once

#include "espurna.h"

constexpr const unsigned char _ledPin(unsigned char index) {
    return (
        (index == 0) ? LED1_PIN :
        (index == 1) ? LED2_PIN :
        (index == 2) ? LED3_PIN :
        (index == 3) ? LED4_PIN :
        (index == 4) ? LED5_PIN :
        (index == 5) ? LED6_PIN :
        (index == 6) ? LED7_PIN :
        (index == 7) ? LED8_PIN : GPIO_NONE
    );
}

constexpr const unsigned char _ledMode(unsigned char index) {
    return (
        (index == 0) ? LED1_MODE :
        (index == 1) ? LED2_MODE :
        (index == 2) ? LED3_MODE :
        (index == 3) ? LED4_MODE :
        (index == 4) ? LED5_MODE :
        (index == 5) ? LED6_MODE :
        (index == 6) ? LED7_MODE :
        (index == 7) ? LED8_MODE : LED_MODE_WIFI
    );
}

constexpr const unsigned char _ledRelay(unsigned char index) {
    return (
        (index == 0) ? (LED1_RELAY - 1) :
        (index == 1) ? (LED2_RELAY - 1) :
        (index == 2) ? (LED3_RELAY - 1) :
        (index == 3) ? (LED4_RELAY - 1) :
        (index == 4) ? (LED5_RELAY - 1) :
        (index == 5) ? (LED6_RELAY - 1) :
        (index == 6) ? (LED7_RELAY - 1) :
        (index == 7) ? (LED8_RELAY - 1) : RELAY_NONE
    );
}

constexpr const bool _ledInverse(unsigned char index) {
    return (
        (index == 0) ? (1 == LED1_PIN_INVERSE) :
        (index == 1) ? (1 == LED2_PIN_INVERSE) :
        (index == 2) ? (1 == LED3_PIN_INVERSE) :
        (index == 3) ? (1 == LED4_PIN_INVERSE) :
        (index == 4) ? (1 == LED5_PIN_INVERSE) :
        (index == 5) ? (1 == LED6_PIN_INVERSE) :
        (index == 6) ? (1 == LED7_PIN_INVERSE) :
        (index == 7) ? (1 == LED8_PIN_INVERSE) : false
    );
}
