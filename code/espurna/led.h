/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <cstddef>

enum class LedMode {
    Manual,
    WiFi,
    Follow,
    FollowInverse,
    FindMe,
    FindMeWiFi,
    On,
    Off,
    Relay,
    RelayWiFi
};

size_t ledCount();

bool ledStatus(size_t id, bool status);
bool ledStatus(size_t id);
void ledSetup();
