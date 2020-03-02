/*
 
  Original code:

  Debounce buttons and trigger events
  Copyright (C) 2015-2018 by Xose Pérez <xose dot perez at gmail dot com>

  The DebounceEvent library is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  The DebounceEvent library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with the DebounceEvent library.  If not, see <http://www.gnu.org/licenses/>.

  ----------------------------------------------------------------------------------

  Modified to include generic INPUT / OUTPUT pin support through a custom interface.

  Definitions are incompatible with DebounceEvent, you should not include it's headers.

*/

#include <functional>
#include <memory>

#include "libs/DebounceEvent.h"

namespace DebounceEvent {

// We need to explicitly call the constructor, because we need to set the const `pin`:
// https://isocpp.org/wiki/faq/multiple-inheritance#virtual-inheritance-ctors
DigitalPin::DigitalPin(unsigned char pin) :
    PinBase(pin)
{}

inline void DigitalPin::pinMode(int8_t mode) {
    ::pinMode(this->pin, mode);
}

inline void DigitalPin::digitalWrite(int8_t val) {
    ::digitalWrite(this->pin, val);
}

inline int DigitalPin::digitalRead() {
    return ::digitalRead(this->pin);
}

// TODO: current implementation allows pin == nullptr

EventHandler::EventHandler(std::shared_ptr<PinBase> pin, EventHandler::callback_f callback, int mode, unsigned long debounce_delay, unsigned long repeat) :
    _pin(pin),
    _callback(callback),
    _mode(mode),
    _is_switch(mode & Types::ModeSwitch),
    _default_status(mode & Types::ModeDefaultHigh),
    _delay(debounce_delay),
    _repeat(repeat),
    _status(false),
    _ready(false),
    _reset_count(true),
    _event_start(0),
    _event_length(0),
    _event_count(0)
{
    if (_mode & Types::ModeSetPullup) {
        _pin->pinMode(INPUT_PULLUP);
    } else if (_mode & Types::ModeSetPulldown) {
        // ESP8266 does not have INPUT_PULLDOWN definition, and instead
        // has a GPIO16-specific INPUT_PULLDOWN_16:
        // - https://github.com/esp8266/Arduino/issues/478
        // - https://github.com/esp8266/Arduino/commit/1b3581d55ebf0f8c91e081f9af4cf7433d492ec9
        #ifdef ESP8266
            if (_pin->pin == 16) {
                _pin->pinMode(_default_status ? INPUT : INPUT_PULLDOWN_16);
            } else {
                _pin->pinMode(INPUT);
            }
        #else
            _pin->pinMode(INPUT_PULLDOWN);
        #endif
    } else {
        _pin->pinMode(INPUT);
    }

    _status = _is_switch ? (_pin->digitalRead() == (HIGH)) : _default_status;
}

EventHandler::EventHandler(std::shared_ptr<PinBase> pin, int mode, unsigned long delay, unsigned long repeat) :
    EventHandler(pin, nullptr, mode, delay, repeat)
{}

bool EventHandler::pressed() {
    return (_status != _default_status);
}

const unsigned char EventHandler::getPin() const {
    return _pin->pin;
}

const int EventHandler::getMode() const {
    return _mode;
}

unsigned long EventHandler::getEventLength() {
    return _event_length;
}
unsigned long EventHandler::getEventCount() {
    return _event_count;
}

Types::event_t EventHandler::loop() {

    static_assert((HIGH) == 1, "Arduino API HIGH is not 1");
    static_assert((LOW) == 0, "Arduino API LOW is not 0");

    auto event = Types::EventNone;
    bool status = _pin->digitalRead() == (HIGH);

    if (status != _status) {

        // TODO: check each loop instead of blocking?
        auto start = millis();
        while (millis() - start < _delay) delay(1);

        status = _pin->digitalRead() == (HIGH);
        if (status != _status) {

            _status = !_status;

            if (_is_switch) {
                event = Types::EventChanged;
            } else {
                if (_status == _default_status) {
                    _event_length = millis() - _event_start;
                    _ready = true;
                } else {
                    event = Types::EventPressed;
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
        event = Types::EventReleased;
    }

    if (_callback && (event != Types::EventNone)) {
        _callback(this, event, _event_count, _event_length);
    }

    return event;

}

} // namespace DebounceEvent
