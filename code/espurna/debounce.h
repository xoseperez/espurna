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

#pragma once

#include <Arduino.h>

namespace DebounceEvent {

#include <functional>
#include <memory>

namespace Types {
    enum event_t {
        EventNone,
        EventChanged,
        EventPressed,
        EventReleased
    };

    enum mode_t {
        ModePushbutton = 1 << 0,
        ModeSwitch = 1 << 1,
        ModeDefaultHigh = 1 << 2,
        ModeSetPullup = 1 << 3
    };
}

constexpr const unsigned long DebounceDelay = 50UL;
constexpr const unsigned long RepeatDelay = 500UL;

// base interface for generic pin handler. 
class PinBase {
    public:
        PinBase(unsigned char pin) :
            pin(pin)
        {}

        virtual void pinMode(int8_t mode) = 0;
        virtual void digitalWrite(int8_t val) = 0;
        virtual int digitalRead() = 0;

        const unsigned char pin;
};

// real hardware pin
class DigitalPin : public PinBase {
    public:
        DigitalPin(unsigned char pin) :
            PinBase(pin)
        {}

        void pinMode(int8_t mode) {
            // Note: proxy for pinMode so it doesn't ignore INPUT_PULLUP with GPIO16
            if (((mode == INPUT) || (mode == INPUT_PULLUP)) && this->pin == 16) {
                ::pinMode(this->pin, ((mode == INPUT_PULLUP) ? INPUT : INPUT_PULLDOWN_16));
                return;
            }
            ::pinMode(this->pin, mode);
        }
        void digitalWrite(int8_t val) {
            ::digitalWrite(this->pin, val);
        }
        int digitalRead() {
            return ::digitalRead(this->pin);
        }
};

class DebounceEvent {


    public:

        // TODO: not used in espurna buttons node
        using callback_f = std::function<void(DebounceEvent* self, uint8_t event, uint8_t count, uint16_t length)>;

        DebounceEvent(std::shared_ptr<PinBase> pin, int mode = Types::ModePushbutton | Types::ModeDefaultHigh, unsigned long delay = DebounceDelay, unsigned long repeat = RepeatDelay);
        DebounceEvent(std::shared_ptr<PinBase> pin, callback_f callback, int mode = Types::ModePushbutton | Types::ModeDefaultHigh, unsigned long delay = DebounceDelay, unsigned long repeat = RepeatDelay);

        Types::event_t loop();
        bool pressed();

        unsigned long getEventLength();
        unsigned long getEventCount();

        std::shared_ptr<PinBase> pin;
        callback_f callback;

        const int mode;

    private:

        const bool _is_switch;
        const bool _default_status;

        const unsigned long _delay;
        const unsigned long _repeat;

        bool _status;

        bool _ready;
        bool _reset_count;

        unsigned long _event_start;
        unsigned long _event_length;
        unsigned char _event_count;

};

DebounceEvent::DebounceEvent(std::shared_ptr<PinBase> pin, DebounceEvent::callback_f callback, int mode, unsigned long debounce_delay, unsigned long repeat) :
    pin(pin),
    callback(callback),
    mode(mode),
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
    pin->pinMode((mode & Types::ModeSetPullup) ? INPUT_PULLUP : INPUT);
    _status = (mode & Types::ModeSwitch) ? pin->digitalRead() : _default_status;
}

DebounceEvent::DebounceEvent(std::shared_ptr<PinBase> pin, int mode, unsigned long delay, unsigned long repeat) :
    DebounceEvent(pin, nullptr, mode, delay, repeat)
{}

bool DebounceEvent::pressed() {
    return (_status != _default_status);
}

unsigned long DebounceEvent::getEventLength() {
    return _event_length;
}
unsigned long DebounceEvent::getEventCount() {
    return _event_count;
}

Types::event_t DebounceEvent::loop() {

    auto event = Types::EventNone;

    if (pin->digitalRead() != _status) {

        // TODO: check each loop instead of blocking?
        auto start = millis();
        while (millis() - start < _delay) delay(1);

        if (pin->digitalRead() != _status) {

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

    if (callback && (event != Types::EventNone)) {
        callback(this, event, _event_count, _event_length);
    }

    return event;

}

// compat definitions from the original lib
#define BUTTON_PUSHBUTTON DebounceEvent::Types::ModePushbutton
#define BUTTON_SWITCH DebounceEvent::Types::ModeSwitch
#define BUTTON_DEFAULT_HIGH DebounceEvent::Types::ModeDefaultHigh
#define BUTTON_SET_PULLUP DebounceEvent::Types::ModeSetPullup

#define EVENT_NONE DebounceEvent::Types::EventNone
#define EVENT_CHANGED DebounceEvent::Types::EventChanged
#define EVENT_PRESSED DebounceEvent::Types::EventPressed
#define EVENT_RELEASED DebounceEvent::Types::EventReleased

} // namespace DebounceEvent
