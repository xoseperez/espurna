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

    // store configuration
    _pin = pin;
    _status = _defaultStatus = defaultStatus;
    _delay = delay;
    _callback = callback;

    // set up button
    if (_defaultStatus == LOW) {
        pinMode(_pin, INPUT);
    } else {
        pinMode(_pin, INPUT_PULLUP);
    }

}

bool DebounceEvent::loop() {

    // holds whether status has changed or not
    bool changed = false;

    if (digitalRead(_pin) != _status) {
        delay(_delay);
        uint8_t newStatus = digitalRead(_pin);
        if (newStatus != _status) {

            changed = true;
            _status = newStatus;

            // raise events if callback defined
            if (_callback) {

                // raise change event
                _callback(_pin, EVENT_CHANGED);

                if (_status == _defaultStatus) {
                    // raise released event
                    _callback(_pin, EVENT_RELEASED);
                } else {
                    // raise pressed event
                    _callback(_pin, EVENT_PRESSED);
                }

            }
        }
    }

    return changed;

}

bool DebounceEvent::pressed() {
    return (_status != _defaultStatus);
}
