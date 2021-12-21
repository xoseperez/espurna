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

constexpr size_t ButtonsActionMax { 255ul };

constexpr size_t ButtonsPresetMax { 8ul };
constexpr size_t ButtonsMax { 32ul };

enum class ButtonProvider {
    None,
    Gpio,
    Analog
};

enum class ButtonEvent {
    None,
    Pressed,
    Released,
    Click,
    DoubleClick,
    LongClick,
    LongLongClick,
    TripleClick
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
    FanHigh
};

struct ButtonActions {
    ButtonAction pressed;
    ButtonAction released;
    ButtonAction click;
    ButtonAction dblclick;
    ButtonAction lngclick;
    ButtonAction lnglngclick;
    ButtonAction trplclick;
};

struct ButtonEventDelays {
    ButtonEventDelays();
    ButtonEventDelays(unsigned long debounce, unsigned long repeat, unsigned long lngclick, unsigned long lnglngclick);

    unsigned long debounce;
    unsigned long repeat;
    unsigned long lngclick;
    unsigned long lnglngclick;
};

using ButtonEventEmitterPtr = std::unique_ptr<debounce_event::EventEmitter>;

struct Button {
    Button(ButtonActions&& actions, ButtonEventDelays&& delays);
    Button(BasePinPtr&& pin, const debounce_event::types::Config& config,
        ButtonActions&& actions, ButtonEventDelays&& delays);

    bool state();
    ButtonEvent loop();

    ButtonEventEmitterPtr event_emitter;

    ButtonActions actions;
    ButtonEventDelays event_delays;
};

using ButtonEventHandler = void(*)(size_t id, ButtonEvent event);
void buttonSetCustomAction(ButtonEventHandler);

ButtonAction buttonAction(size_t id, const ButtonEvent event);
void buttonEvent(size_t id, ButtonEvent event);
void buttonOnEvent(ButtonEventHandler);

bool buttonAdd();

size_t buttonCount();
void buttonSetup();
