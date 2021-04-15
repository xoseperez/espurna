/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <vector>

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

enum class LedDelayMode {
    Finite,
    Infinite,
    None
};

struct LedDelay {
    LedDelay() = delete;

    constexpr LedDelay(unsigned long on_ms, unsigned long off_ms, unsigned char repeats) :
        _mode(repeats ? LedDelayMode::Finite : LedDelayMode::Infinite),
        _on(microsecondsToClockCycles(on_ms * 1000)),
        _off(microsecondsToClockCycles(off_ms * 1000)),
        _repeats(repeats)
    {}

    constexpr LedDelay(unsigned long on_ms, unsigned long off_ms) :
        LedDelay(on_ms, off_ms, 0)
    {}

    constexpr LedDelayMode mode() const {
        return _mode;
    }

    constexpr unsigned long on() const {
        return _on;
    }

    constexpr unsigned long off() const {
        return _off;
    }

    unsigned char repeats() const {
        return _repeats;
    }

    bool repeat() {
        if (_repeats) {
            --_repeats;
        }

        return _repeats;
    }

private:
    LedDelayMode _mode;
    unsigned long _on;
    unsigned long _off;
    unsigned char _repeats;
};

size_t ledCount();

bool ledStatus(size_t id, bool status);
bool ledStatus(size_t id);
void ledSetup();
