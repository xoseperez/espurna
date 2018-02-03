
/*

GENERIC INPUT MODULE

Copyright (C) 2018 by Thomas Stärk

*/

// -----------------------------------------------------------------------------
// GENERIC INPUT
// -----------------------------------------------------------------------------

#include <Arduino.h>
//#include "inputfilter.h"

InputFilter::InputFilter(uint8_t input, uint8_t pin, uint8_t mode, unsigned long filter){
    _init(input, pin, mode, filter);
}

// ------------------------------------------
// parameter
//      pin = GPIO Number
//      mode = inputmode, see inputfilter.h
//      filter = filter time for rising / falling signal in milliseconds
// return
//      void
// ------------------------------------------
void InputFilter::_init(uint8_t input, uint8_t pin, uint8_t mode, unsigned long filter) {

    // store configuration
    _input = input;
    _pin = pin;
    _mode = mode;
    _filter = filter;
    _status = _defaultStatus = (mode & GEN_INPUT_DEFAULT_HIGH) > 0;


    // set up button
    if (_pin == 16) {
        if (_defaultStatus) {
            pinMode(_pin, INPUT);
        } else {
            pinMode(_pin, INPUT_PULLDOWN_16);
        }
    } else {
        if ((mode & GEN_INPUT_SET_PULLUP) > 0) {
            pinMode(_pin, INPUT_PULLUP);
        } else {
            pinMode(_pin, INPUT);
        }
    }

}

// ------------------------------------------
// parameter
//      void
// return
//      input number 1..8 corresponding to "#define INPUT1,..."
// ------------------------------------------
uint8_t InputFilter::getInputNumber() {
    return _input;
}

// ------------------------------------------
// parameter
//      void
// return
//      status type, see inputfilter.h
// ------------------------------------------
uint8_t InputFilter::getInputState() {
    if (_status == _defaultStatus)
        return GIS_OFF;
    else
        return GIS_ON;
}

// ------------------------------------------
// parameter
//      void
// return
//      event type, see inputfilter.h
// ------------------------------------------
unsigned char InputFilter::loop() {

    bool trigger_event = false;
    unsigned char event = GIE_NONE;
    bool pinstate = digitalRead(_pin);

    if (pinstate != _prev_pinstate) {
      _start = millis();
      _prev_pinstate = pinstate;
    }

    if (_status != pinstate) {
      if (_filter == 0) {
        trigger_event = true;
        _status = pinstate;
      }
      else {
          if ((millis() - _start) >= _filter) {
            _status = pinstate;
            trigger_event = true;
          }
      }

      if (trigger_event) {
        if (pinstate == _defaultStatus) {
            DEBUG_MSG_P(PSTR("[INPUTFILTER] Input #%d state %d => OFF\n"), _pin, pinstate);
            event = GIE_SWITCHED_OFF;
        }
        else {
            DEBUG_MSG_P(PSTR("[INPUTFILTER] Input #%d state %d => ON\n"), _pin, pinstate);
            event = GIE_SWITCHED_ON;
        }
      }
    }

    return event;

}
