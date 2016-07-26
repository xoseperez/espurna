/*

  Debounce buttons and trigger events
  Copyright (C) 2015 by Xose Pérez <xose dot perez at gmail dot com>

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

DebounceEvent::DebounceEvent(uint8_t pin, callback_t callback, uint8_t defaultStatus, unsigned long delay) {
    _callback = callback;
    DebounceEvent(pin, defaultStatus, delay);
}

DebounceEvent::DebounceEvent(uint8_t pin, uint8_t defaultStatus, unsigned long delay) {

    // store configuration
    _pin = pin;
    _status = _defaultStatus = defaultStatus;
    _delay = delay;

    // set up button
    if (_defaultStatus == LOW) {
        pinMode(_pin, INPUT);
    } else {
        pinMode(_pin, INPUT_PULLUP);
    }

}

bool DebounceEvent::loop() {

    // holds whether status has changed or not
    static bool pending = false;
    bool changed = false;
    _event = EVENT_NONE;

    if (digitalRead(_pin) != _status) {

        // Debounce
        delay(_delay);
        uint8_t newStatus = digitalRead(_pin);
        if (newStatus != _status) {

            changed = true;
            pending = false;
            _status = newStatus;

            // released
            if (_status == _defaultStatus) {

                // get event
                if (millis() - _this_start > LONG_CLICK_DELAY) {
                    _event = EVENT_LONG_CLICK;
                } else if (millis() - _last_start < DOUBLE_CLICK_DELAY ) {
                    _event = EVENT_DOUBLE_CLICK;
                } else {
                    changed = false;
                    pending = true;
                    //_event = EVENT_SINGLE_CLICK;
                }

            // pressed
            } else {

                _last_start = _this_start;
                _this_start = millis();
                _event = EVENT_PRESSED;

            }

        }
    }

    if (pending && (millis() - _this_start > DOUBLE_CLICK_DELAY) && (!changed) && (_status == _defaultStatus)) {
        pending = false;
        changed = true;
        _event = EVENT_SINGLE_CLICK;
    }

    if (changed) {
        if (_callback) {
            _callback(_pin, EVENT_CHANGED);
            _callback(_pin, _event);
        }
    }


    return changed;

}

bool DebounceEvent::pressed() {
    return (_status != _defaultStatus);
}

uint8_t DebounceEvent::getEvent() {
    return _event;
}
