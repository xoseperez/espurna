/*

  Debounce buttons and trigger events
  Copyright (C) 2015-2017 by Xose Pérez <xose dot perez at gmail dot com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <Arduino.h>
#include "DebounceEvent.h"

DebounceEvent::DebounceEvent(uint8_t pin, TDebounceEventCallback callback, uint8_t mode, unsigned long delay, unsigned long repeat) {
    _callback = callback;
    _init(pin, mode, delay, repeat);
}

DebounceEvent::DebounceEvent(uint8_t pin, uint8_t mode, unsigned long delay, unsigned long repeat) {
    _init(pin, mode, delay, repeat);
}


void DebounceEvent::_init(uint8_t pin, uint8_t mode, unsigned long delay, unsigned long repeat) {

    // store configuration
    _pin = pin;
    _mode = mode & 0x01;
    _status = _defaultStatus = (mode & BUTTON_DEFAULT_HIGH) > 0;
    _delay = delay;
    _repeat = repeat;

    // set up button
    if (_pin == 16) {
        if (_defaultStatus) {
            pinMode(_pin, INPUT);
        } else {
            pinMode(_pin, INPUT_PULLDOWN_16);
        }
    } else {
        if ((mode & BUTTON_SET_PULLUP) > 0) {
            pinMode(_pin, INPUT_PULLUP);
        } else {
            pinMode(_pin, INPUT);
        }
    }

}

unsigned char DebounceEvent::loop() {

    unsigned char event = EVENT_NONE;

    if (digitalRead(_pin) != _status) {

        // Debounce
        unsigned long start = millis();
        while (millis() - start < _delay) delay(1);

        if (digitalRead(_pin) != _status) {

            _status = !_status;

            if (_mode == BUTTON_SWITCH) {
                _event_start = millis();
                if (_reset_count) {
                    _event_count = 1;
                    _reset_count = false;
                } else {
                    ++_event_count;
                }
                _ready = true;
            } else {

                // released
                if (_status == _defaultStatus) {

                    _event_length = millis() - _event_start;
                    _ready = true;

                // pressed
                } else {

                    event = EVENT_PRESSED;
                    _event_start = millis();
                    _event_length = 0;
                    if (_reset_count) {
                        _event_count = 1;
                        _reset_count = false;
                    } else {
                        ++_event_count;
                    }
                    _ready = false;

                }

            }

        }
    }

    if (_ready && (millis() - _event_start > _repeat)) {
        _ready = false;
        _reset_count = true;
        event = (_mode == BUTTON_SWITCH) ? EVENT_CHANGED : EVENT_RELEASED ;
    }

    if (event != EVENT_NONE) {
        if (_callback) _callback(_pin, event, _event_count, _event_length);
    }

    return event;

}
