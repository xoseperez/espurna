/*

IR MODULE

Copyright (C) 2018 by Alexander Kolesnikov (raw and MQTT implementation)
Copyright (C) 2017-2019 by François Déchery
Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

// Remote Buttons SET 1 (for the original Remote shipped with the controller)
#if IR_BUTTON_SET == 1

/*
   +------+------+------+------+
   |  UP  | Down | OFF  |  ON  |
   +------+------+------+------+
   |  R   |  G   |  B   |  W   |
   +------+------+------+------+
   |  1   |  2   |  3   |FLASH |
   +------+------+------+------+
   |  4   |  5   |  6   |STROBE|
   +------+------+------+------+
   |  7   |  8   |  9   | FADE |
   +------+------+------+------+
   |  10  |  11  |  12  |SMOOTH|
   +------+------+------+------+
*/

    constexpr const size_t IR_BUTTON_COUNT = 24;

    const uint32_t IR_BUTTON[IR_BUTTON_COUNT][3] PROGMEM = {

        { 0xFF906F, IR_BUTTON_ACTION_BRIGHTER, 1 },
        { 0xFFB847, IR_BUTTON_ACTION_BRIGHTER, 0 },
        { 0xFFF807, IR_BUTTON_ACTION_STATE, 0 },
        { 0xFFB04F, IR_BUTTON_ACTION_STATE, 1 },

        { 0xFF9867, IR_BUTTON_ACTION_RGB, 0xFF0000 },
        { 0xFFD827, IR_BUTTON_ACTION_RGB, 0x00FF00 },
        { 0xFF8877, IR_BUTTON_ACTION_RGB, 0x0000FF },
        { 0xFFA857, IR_BUTTON_ACTION_RGB, 0xFFFFFF },

        { 0xFFE817, IR_BUTTON_ACTION_RGB, 0xD13A01 },
        { 0xFF48B7, IR_BUTTON_ACTION_RGB, 0x00E644 },
        { 0xFF6897, IR_BUTTON_ACTION_RGB, 0x0040A7 },
        { 0xFFB24D, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_FLASH },

        { 0xFF02FD, IR_BUTTON_ACTION_RGB, 0xE96F2A },
        { 0xFF32CD, IR_BUTTON_ACTION_RGB, 0x00BEBF },
        { 0xFF20DF, IR_BUTTON_ACTION_RGB, 0x56406F },
        { 0xFF00FF, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_STROBE },

        { 0xFF50AF, IR_BUTTON_ACTION_RGB, 0xEE9819 },
        { 0xFF7887, IR_BUTTON_ACTION_RGB, 0x00799A },
        { 0xFF708F, IR_BUTTON_ACTION_RGB, 0x944E80 },
        { 0xFF58A7, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_FADE },

        { 0xFF38C7, IR_BUTTON_ACTION_RGB, 0xFFFF00 },
        { 0xFF28D7, IR_BUTTON_ACTION_RGB, 0x0060A1 },
        { 0xFFF00F, IR_BUTTON_ACTION_RGB, 0xEF45AD },
        { 0xFF30CF, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_SMOOTH }

    };

#endif

//Remote Buttons SET 2 (another identical IR Remote shipped with another controller)
#if IR_BUTTON_SET == 2

/*
    +------+------+------+------+
    |  UP  | Down | OFF  |  ON  |
    +------+------+------+------+
    |  R   |  G   |  B   |  W   |
    +------+------+------+------+
    |  1   |  2   |  3   |FLASH |
    +------+------+------+------+
    |  4   |  5   |  6   |STROBE|
    +------+------+------+------+
    |  7   |  8   |  9   | FADE |
    +------+------+------+------+
    |  10  |  11  |  12  |SMOOTH|
    +------+------+------+------+
*/

    constexpr const size_t IR_BUTTON_COUNT = 24;

    const unsigned long IR_BUTTON[IR_BUTTON_COUNT][3] PROGMEM = {

        { 0xFF00FF, IR_BUTTON_ACTION_BRIGHTER, 1 },
        { 0xFF807F, IR_BUTTON_ACTION_BRIGHTER, 0 },
        { 0xFF40BF, IR_BUTTON_ACTION_STATE, 0 },
        { 0xFFC03F, IR_BUTTON_ACTION_STATE, 1 },

        { 0xFF20DF, IR_BUTTON_ACTION_RGB, 0xFF0000 },
        { 0xFFA05F, IR_BUTTON_ACTION_RGB, 0x00FF00 },
        { 0xFF609F, IR_BUTTON_ACTION_RGB, 0x0000FF },
        { 0xFFE01F, IR_BUTTON_ACTION_RGB, 0xFFFFFF },

        { 0xFF10EF, IR_BUTTON_ACTION_RGB, 0xD13A01 },
        { 0xFF906F, IR_BUTTON_ACTION_RGB, 0x00E644 },
        { 0xFF50AF, IR_BUTTON_ACTION_RGB, 0x0040A7 },
        { 0xFFD02F, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_FLASH },

        { 0xFF30CF, IR_BUTTON_ACTION_RGB, 0xE96F2A },
        { 0xFFB04F, IR_BUTTON_ACTION_RGB, 0x00BEBF },
        { 0xFF708F, IR_BUTTON_ACTION_RGB, 0x56406F },
        { 0xFFF00F, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_STROBE },

        { 0xFF08F7, IR_BUTTON_ACTION_RGB, 0xEE9819 },
        { 0xFF8877, IR_BUTTON_ACTION_RGB, 0x00799A },
        { 0xFF48B7, IR_BUTTON_ACTION_RGB, 0x944E80 },
        { 0xFFC837, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_FADE },

        { 0xFF28D7, IR_BUTTON_ACTION_RGB, 0xFFFF00 },
        { 0xFFA857, IR_BUTTON_ACTION_RGB, 0x0060A1 },
        { 0xFF6897, IR_BUTTON_ACTION_RGB, 0xEF45AD },
        { 0xFFE817, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_SMOOTH }

    };

#endif

//Remote Buttons SET 3 (samsung AA59-00608A 8 Toggle Buttons for generic 8CH module)
#if IR_BUTTON_SET == 3
/*
    +------+------+------+
    |  1   |  2   |  3   |
    +------+------+------+
    |  4   |  5   |  6   |
    +------+------+------+
    |  7   |  8   |  9   |
    +------+------+------+
    |      |  0   |      |
    +------+------+------+
*/
    constexpr const size_t IR_BUTTON_COUNT = 8;

    const unsigned long IR_BUTTON[IR_BUTTON_COUNT][3] PROGMEM = {

        { 0xE0E020DF, IR_BUTTON_ACTION_TOGGLE, 0 }, // Toggle Relay #0
        { 0xE0E0A05F, IR_BUTTON_ACTION_TOGGLE, 1 }, // Toggle Relay #1
        { 0xE0E0609F, IR_BUTTON_ACTION_TOGGLE, 2 }, // Toggle Relay #2

        { 0xE0E010EF, IR_BUTTON_ACTION_TOGGLE, 3 }, // Toggle Relay #3
        { 0xE0E0906F, IR_BUTTON_ACTION_TOGGLE, 4 }, // Toggle Relay #4
        { 0xE0E050AF, IR_BUTTON_ACTION_TOGGLE, 5 }, // Toggle Relay #5

        { 0xE0E030CF, IR_BUTTON_ACTION_TOGGLE, 6 }, // Toggle Relay #6
        { 0xE0E0B04F, IR_BUTTON_ACTION_TOGGLE, 7 } // Toggle Relay #7
        //{ 0xE0E0708F, IR_BUTTON_ACTION_TOGGLE, 8 } //Extra Button

        //{ 0xE0E08877, IR_BUTTON_ACTION_TOGGLE, 9 } //Extra Button
    };
#endif

//Remote Buttons SET 4
#if IR_BUTTON_SET == 4
/*
    +------+------+------+
    | OFF  | SRC  | MUTE |
    +------+------+------+
    ...
    +------+------+------+
*/
    constexpr const size_t IR_BUTTON_COUNT = 1;

    const unsigned long IR_BUTTON[IR_BUTTON_COUNT][3] PROGMEM = {

        { 0xFFB24D, IR_BUTTON_ACTION_TOGGLE, 0 } // Toggle Relay #0

    };

#endif

//Remote Buttons SET 5 (another identical IR Remote shipped with another controller as SET 1 and 2)
#if IR_BUTTON_SET == 5

/*
    +------+------+------+------+
    |  UP  | Down | OFF  |  ON  |
    +------+------+------+------+
    |  R   |  G   |  B   |  W   |
    +------+------+------+------+
    |  1   |  2   |  3   |FLASH |
    +------+------+------+------+
    |  4   |  5   |  6   |STROBE|
    +------+------+------+------+
    |  7   |  8   |  9   | FADE |
    +------+------+------+------+
    |  10  |  11  |  12  |SMOOTH|
    +------+------+------+------+
*/
    constexpr const size_t IR_BUTTON_COUNT = 24;

    const unsigned long IR_BUTTON[IR_BUTTON_COUNT][3] PROGMEM = {

        { 0xF700FF, IR_BUTTON_ACTION_BRIGHTER, 1 },
        { 0xF7807F, IR_BUTTON_ACTION_BRIGHTER, 0 },
        { 0xF740BF, IR_BUTTON_ACTION_STATE, 0 },
        { 0xF7C03F, IR_BUTTON_ACTION_STATE, 1 },

        { 0xF720DF, IR_BUTTON_ACTION_RGB, 0xFF0000 },
        { 0xF7A05F, IR_BUTTON_ACTION_RGB, 0x00FF00 },
        { 0xF7609F, IR_BUTTON_ACTION_RGB, 0x0000FF },
        { 0xF7E01F, IR_BUTTON_ACTION_RGB, 0xFFFFFF },

        { 0xF710EF, IR_BUTTON_ACTION_RGB, 0xD13A01 },
        { 0xF7906F, IR_BUTTON_ACTION_RGB, 0x00E644 },
        { 0xF750AF, IR_BUTTON_ACTION_RGB, 0x0040A7 },
        { 0xF7D02F, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_FLASH },

        { 0xF730CF, IR_BUTTON_ACTION_RGB, 0xE96F2A },
        { 0xF7B04F, IR_BUTTON_ACTION_RGB, 0x00BEBF },
        { 0xF7708F, IR_BUTTON_ACTION_RGB, 0x56406F },
        { 0xF7F00F, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_STROBE },

        { 0xF708F7, IR_BUTTON_ACTION_RGB, 0xEE9819 },
        { 0xF78877, IR_BUTTON_ACTION_RGB, 0x00799A },
        { 0xF748B7, IR_BUTTON_ACTION_RGB, 0x944E80 },
        { 0xF7C837, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_FADE },

        { 0xF728D7, IR_BUTTON_ACTION_RGB, 0xFFFF00 },
        { 0xF7A857, IR_BUTTON_ACTION_RGB, 0x0060A1 },
        { 0xF76897, IR_BUTTON_ACTION_RGB, 0xEF45AD },
        { 0xF7E817, IR_BUTTON_ACTION_EFFECT, LIGHT_EFFECT_SMOOTH }

    };

#endif

