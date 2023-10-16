/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

#include "libs/BasePin.h"
#include "libs/DebounceEvent.h"

#include <cstdint>
#include <cstddef>
#include <memory>

constexpr size_t ButtonsMax { 32ul };

enum class ButtonEvent {
    None,
    Pressed,
    Released,
    Click,
    DoubleClick,
    LongClick,
    LongLongClick,
    TripleClick,
};

enum class ButtonAction {
    None,
    Toggle,
    On,
    Off,
    AccessPoint,
    Reset,
    Pulse,
    FactoryReset,
    Wps,
    SmartConfig,
    BrightnessIncrease,
    BrightnessDecrease,
    DisplayOn,
    Custom,
    FanLow,
    FanMedium,
    FanHigh,
    TerminalCommand,
};

using ButtonEventHandler = void(*)(size_t id, ButtonEvent event);
void buttonSetCustomAction(ButtonEventHandler);

ButtonAction buttonAction(size_t id, const ButtonEvent event);
void buttonEvent(size_t id, ButtonEvent event);
void buttonOnEvent(ButtonEventHandler);

size_t buttonCount();
void buttonSetup();
