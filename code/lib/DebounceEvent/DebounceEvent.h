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

#ifndef _DEBOUNCE_EVENT_h
#define _DEBOUNCE_EVENT_h

#define DEBOUNCE_DELAY 50
#define LONG_CLICK_DELAY 1000
#define DOUBLE_CLICK_DELAY 500

#define EVENT_NONE 0
#define EVENT_CHANGED 1
#define EVENT_PRESSED 2
#define EVENT_RELEASED 3
#define EVENT_SINGLE_CLICK 3
#define EVENT_DOUBLE_CLICK 4
#define EVENT_LONG_CLICK 5

typedef void(*callback_t)(uint8_t pin, uint8_t event);

class DebounceEvent {

    private:

        uint8_t _pin;
        uint8_t _status;
        uint8_t _event;
        unsigned long _this_start;
        unsigned long _last_start;
        uint8_t _defaultStatus;
        unsigned long _delay;
        callback_t _callback = false;

    public:

        DebounceEvent(uint8_t pin, callback_t callback, uint8_t defaultStatus = HIGH, unsigned long delay = DEBOUNCE_DELAY);
        DebounceEvent(uint8_t pin, uint8_t defaultStatus = HIGH, unsigned long delay = DEBOUNCE_DELAY);
        bool pressed();
        bool loop();
        uint8_t getEvent();

};

#endif
