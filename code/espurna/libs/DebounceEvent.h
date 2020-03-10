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

class EventHandler;

namespace types {

    enum Event {
        EventNone,
        EventChanged,
        EventPressed,
        EventReleased
    };

    enum Config {
        ConfigPushbutton = 1 << 0,
        ConfigSwitch = 1 << 1,
        ConfigDefaultHigh = 1 << 2,
        ConfigSetPullup = 1 << 3,
        ConfigSetPulldown = 1 << 4
    };

    using Pin = std::shared_ptr<BasePin>;
    using EventCallback = std::function<void(const EventHandler& self, types::Event event, uint8_t count, unsigned long length)>;

}

class EventHandler {

    public:

        EventHandler(types::Pin pin, int config = types::ConfigPushbutton | types::ConfigDefaultHigh, unsigned long delay = DebounceDelay, unsigned long repeat = RepeatDelay);
        EventHandler(types::Pin pin, types::EventCallback callback, int mode = types::ConfigPushbutton | types::ConfigDefaultHigh, unsigned long delay = DebounceDelay, unsigned long repeat = RepeatDelay);

        types::Event loop();
        bool pressed();

        const types::Pin getPin() const;
        const int getConfig() const;

        unsigned long getEventLength();
        unsigned long getEventCount();

    private:

        types::Pin _pin;
        types::EventCallback _callback;

        const int _config;

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


} // namespace debounce_event
