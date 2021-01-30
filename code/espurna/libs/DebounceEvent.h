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

#pragma once

#include <Arduino.h>

#include <functional>
#include <memory>

#include "BasePin.h"

namespace debounce_event {

constexpr const unsigned long DebounceDelay = 50UL;
constexpr const unsigned long RepeatDelay = 500UL;

class EventEmitter;

namespace types {

    enum class Mode {
        Pushbutton,
        Switch
    };

    enum class PinValue {
        Low,
        High,
        Initial
    };

    enum class PinMode {
        Input,
        InputPullup,
        InputPulldown
    };

    struct Config {
        Mode mode;
        PinValue default_value;
        PinMode pin_mode;
    };

    enum Event {
        EventNone,
        EventPressed,
        EventReleased
    };

    using EventHandler = std::function<void(const EventEmitter& self, types::Event event, uint8_t count, unsigned long length)>;

}

class EventEmitter {

    public:

        EventEmitter(BasePinPtr&& pin, const types::Config& config = {types::Mode::Pushbutton, types::PinValue::High, types::PinMode::Input}, unsigned long delay = DebounceDelay, unsigned long repeat = RepeatDelay);
        EventEmitter(BasePinPtr&& pin, types::EventHandler callback, const types::Config& = {types::Mode::Pushbutton, types::PinValue::High, types::PinMode::Input}, unsigned long delay = DebounceDelay, unsigned long repeat = RepeatDelay);

        types::Event loop();
        bool isPressed();

        const BasePinPtr& pin() const;
        const types::Config& config() const;

        unsigned long getEventLength();
        unsigned long getEventCount();

    private:
        BasePinPtr _pin;
        types::EventHandler _callback;

        const types::Config _config;

        const bool _is_switch { false };

        const unsigned long _delay { 0ul };
        const unsigned long _repeat { 0ul };

        bool _default_value { true };
        bool _value { true };

        bool _ready { false };
        bool _reset_count { true };

        unsigned long _event_start { 0ul };
        unsigned long _event_length { 0ul };
        unsigned char _event_count { 0ul };

};


} // namespace debounce_event
