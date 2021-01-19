/*

Fan MODULE

Copyright (C) 2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

enum class FanSpeed {
    Off,
    Low,
    Medium,
    High
};

void fanSpeed(FanSpeed);
FanSpeed fanSpeed();

void fanSetup();
