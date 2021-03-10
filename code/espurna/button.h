/*

BUTTON MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include "libs/BasePin.h"
#include "libs/DebounceEvent.h"

#include <memory>

constexpr size_t ButtonsActionMax { 255ul };

constexpr size_t ButtonsPresetMax { 8ul };
constexpr size_t ButtonsMax { 32ul };

enum class ButtonProvider : int {
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

// button actions, limited to 8-bit number (0b11111111 / 0xff / 255)

enum class ButtonAction : uint8_t  {
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

struct button_t {
    button_t(ButtonActions&& actions, ButtonEventDelays&& delays);
    button_t(BasePinPtr&& pin, const debounce_event::types::Config& config,
        ButtonActions&& actions, ButtonEventDelays&& delays);

    bool state();
    ButtonEvent loop();

    std::unique_ptr<debounce_event::EventEmitter> event_emitter;

    ButtonActions actions;
    ButtonEventDelays event_delays;
};

using ButtonEventHandler = void(*)(size_t id, ButtonEvent event);
void buttonSetCustomAction(ButtonEventHandler);
void buttonSetNotifyAction(ButtonEventHandler);

bool buttonState(size_t id);
ButtonAction buttonAction(size_t id, const ButtonEvent event);

void buttonEvent(size_t id, ButtonEvent event);

size_t buttonCount();
void buttonSetup();
