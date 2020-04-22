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

  Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

  Modified to include generic INPUT / OUTPUT pin support through a custom interface.
  Definitions are incompatible with DebounceEvent, you should not include it's headers.

*/

#include <functional>
#include <memory>

#include "libs/DebounceEvent.h"

namespace debounce_event {

EventEmitter::EventEmitter(types::Pin pin, types::EventHandler callback, const types::Config& config, unsigned long debounce_delay, unsigned long repeat) :
    _pin(pin),
    _callback(callback),
    _config(config),
    _is_switch(config.mode == types::Mode::Switch),
    _default_value(config.default_value == types::PinValue::High),
    _delay(debounce_delay),
    _repeat(repeat),
    _value(false),
    _ready(false),
    _reset_count(true),
    _event_start(0),
    _event_length(0),
    _event_count(0)
{
    if (!pin) return;

    if (_config.pin_mode == types::PinMode::InputPullup) {
        _pin->pinMode(INPUT_PULLUP);
    } else if (_config.pin_mode == types::PinMode::InputPulldown) {
        // ESP8266 does not have INPUT_PULLDOWN definition, and instead
        // has a GPIO16-specific INPUT_PULLDOWN_16:
        // - https://github.com/esp8266/Arduino/issues/478
        // - https://github.com/esp8266/Arduino/commit/1b3581d55ebf0f8c91e081f9af4cf7433d492ec9
        #ifdef ESP8266
            if (_pin->pin == 16) {
                _pin->pinMode(_default_value ? INPUT : INPUT_PULLDOWN_16);
            } else {
                _pin->pinMode(INPUT);
            }
        #else
            _pin->pinMode(INPUT_PULLDOWN);
        #endif
    } else {
        _pin->pinMode(INPUT);
    }

    _value = _is_switch ? (_pin->digitalRead() == (HIGH)) : _default_value;
}

EventEmitter::EventEmitter(types::Pin pin, const types::Config& config, unsigned long delay, unsigned long repeat) :
    EventEmitter(pin, nullptr, config, delay, repeat)
{}

bool EventEmitter::isPressed() {
    return (_value != _default_value);
}

const types::Pin EventEmitter::getPin() const {
    return _pin;
}

const types::Config EventEmitter::getConfig() const {
    return _config;
}

unsigned long EventEmitter::getEventLength() {
    return _event_length;
}
unsigned long EventEmitter::getEventCount() {
    return _event_count;
}

// TODO: current implementation allows pin == nullptr

types::Event EventEmitter::loop() {

    static_assert((HIGH) == 1, "Arduino API HIGH is not 1");
    static_assert((LOW) == 0, "Arduino API LOW is not 0");

    auto event = types::EventNone;
    bool value = _pin->digitalRead() == (HIGH);

    if (value != _value) {

        // TODO: check each loop instead of blocking?
        auto start = millis();
        while (millis() - start < _delay) delay(1);

        value = _pin->digitalRead() == (HIGH);
        if (value != _value) {

            _value = !_value;

            if (_is_switch) {
                event = types::EventChanged;
            } else {
                if (_value == _default_value) {
                    _event_length = millis() - _event_start;
                    _ready = true;
                } else {
                    event = types::EventPressed;
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
        event = types::EventReleased;
    }

    if (_callback && (event != types::EventNone)) {
        _callback(*this, event, _event_count, _event_length);
    }

    return event;

}

} // namespace debounce_event
