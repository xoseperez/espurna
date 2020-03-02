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

#include <functional>
#include <memory>

namespace DebounceEvent {

constexpr const unsigned long DebounceDelay = 50UL;
constexpr const unsigned long RepeatDelay = 500UL;

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
        ModeSetPullup = 1 << 3,
        ModeSetPulldown = 1 << 4
    };
}

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
class DigitalPin : virtual public PinBase {
    public:
        DigitalPin(unsigned char pin);

        void pinMode(int8_t mode) final;
        void digitalWrite(int8_t val) final;
        int digitalRead() final;
};

class EventHandler {


    public:

        // TODO: not used in espurna buttons node
        using callback_f = std::function<void(EventHandler* self, uint8_t event, uint8_t count, uint16_t length)>;

        EventHandler(std::shared_ptr<PinBase> pin, int mode = Types::ModePushbutton | Types::ModeDefaultHigh, unsigned long delay = DebounceDelay, unsigned long repeat = RepeatDelay);
        EventHandler(std::shared_ptr<PinBase> pin, callback_f callback, int mode = Types::ModePushbutton | Types::ModeDefaultHigh, unsigned long delay = DebounceDelay, unsigned long repeat = RepeatDelay);

        Types::event_t loop();
        bool pressed();

        const unsigned char getPin() const;
        const int getMode() const;

        unsigned long getEventLength();
        unsigned long getEventCount();

    private:

        std::shared_ptr<PinBase> _pin;
        callback_f _callback;

        const int _mode;

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


} // namespace DebounceEvent
