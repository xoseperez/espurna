/*

Fan MODULE

Copyright (C) 2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstddef>
#include <memory>

enum class FanSpeed {
    Off,
    Low,
    Medium,
    High
};

class RelayProviderBase;
std::unique_ptr<RelayProviderBase> fanMakeRelayProvider(size_t);

bool fanStatus();
void fanStatus(bool);

void fanSpeed(FanSpeed);
FanSpeed fanSpeed();

void fanSetup();
