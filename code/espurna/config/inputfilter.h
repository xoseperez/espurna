/*

GENERIC INPUT MODULE

Copyright (C) 2018 by Thomas Stärk

*/


#ifndef _INPUTFILTER_h
#define _INPUTFILTER_h

#include <functional>

#define GEN_INPUT_FILTERTIME    0   // [ms] filter time

#ifndef INPUT1_FILTER
#define INPUT1_FILTER           0
#endif
#ifndef INPUT2_FILTER
#define INPUT2_FILTER           0
#endif
#ifndef INPUT3_FILTER
#define INPUT3_FILTER           0
#endif
#ifndef INPUT4_FILTER
#define INPUT4_FILTER           0
#endif
#ifndef INPUT5_FILTER
#define INPUT5_FILTER           0
#endif
#ifndef INPUT6_FILTER
#define INPUT6_FILTER           0
#endif
#ifndef INPUT7_FILTER
#define INPUT7_FILTER           0
#endif
#ifndef INPUT8_FILTER
#define INPUT8_FILTER           0
#endif

#ifndef INPUT1_PIN
#define INPUT1_PIN              0
#endif
#ifndef INPUT2_PIN
#define INPUT2_PIN              0
#endif
#ifndef INPUT3_PIN
#define INPUT3_PIN              0
#endif
#ifndef INPUT4_PIN
#define INPUT4_PIN              0
#endif
#ifndef INPUT5_PIN
#define INPUT5_PIN              0
#endif
#ifndef INPUT6_PIN
#define INPUT6_PIN              0
#endif
#ifndef INPUT7_PIN
#define INPUT7_PIN              0
#endif
#ifndef INPUT8_PIN
#define INPUT8_PIN              0
#endif

#ifndef INPUT1_MODE
#define INPUT1_MODE             GEN_INPUT
#endif
#ifndef INPUT2_MODE
#define INPUT2_MODE             GEN_INPUT
#endif
#ifndef INPUT3_MODE
#define INPUT3_MODE             GEN_INPUT
#endif
#ifndef INPUT4_MODE
#define INPUT4_MODE             GEN_INPUT
#endif
#ifndef INPUT5_MODE
#define INPUT5_MODE             GEN_INPUT
#endif
#ifndef INPUT6_MODE
#define INPUT6_MODE             GEN_INPUT
#endif
#ifndef INPUT7_MODE
#define INPUT7_MODE             GEN_INPUT
#endif
#ifndef INPUT8_MODE
#define INPUT8_MODE             GEN_INPUT
#endif

// define input bevaviour
#define GEN_INPUT                0  // high signal at GPIO --> ON
#define GEN_INPUT_DEFAULT_HIGH   2  // inverse logic, high signal at GPIO --> OFF
#define GEN_INPUT_SET_PULLUP     4  // activate internal pullup at GPIO

// Generic Input Event
#define GIE_NONE                0
#define GIE_SWITCHED_ON         1
#define GIE_SWITCHED_OFF        2

// Generic Input state
#define GIS_UNUSED              0   // requested input number is not upload_speed
#define GIS_ON                  1   // input is in ON state
#define GIS_OFF                 2   // input is in OFF state

// specific MQTT extension for generic input
#define MQTT_TOPIC_GENERICINPUT     "input"


class InputFilter {

    public:

        InputFilter(uint8_t input, uint8_t pin, uint8_t mode = GEN_INPUT, unsigned long filter = GEN_INPUT_FILTERTIME);
        unsigned char loop();
        uint8_t getInputNumber(void);   // return input number (corresponding to #define INPUT1...)
        uint8_t getInputState(void);    // return input state GIS_ON or GIS_OFF

    private:
        uint8_t _input;             // input number (1..8)
        uint8_t _pin;               // GPIO number
        uint8_t _mode;              // see above
        unsigned long _filter;      // milliseconds
        bool _status;               // current state
        bool _defaultStatus;        // default pin status, corresponds to OFF state
        bool _prev_pinstate;        // signal state in last loop cycle
        unsigned long _start;       // time of last signal change in milliseconds

        void _init(uint8_t input, uint8_t pin, uint8_t mode, unsigned long filter);

};

#endif
