/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <vector>

constexpr size_t LedsMax { 8ul };

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

    LedDelayMode mode() const {
        return _mode;
    }

    unsigned long on() const {
        return _on;
    }

    unsigned long off() const {
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

struct LedPattern {
    using Delays = std::vector<LedDelay>;

    LedPattern() = default;
    LedPattern(const Delays& delays);

    void start();
    void stop();

    bool started();
    bool ready();

    Delays delays;
    Delays queue;
    unsigned long clock_last;
    unsigned long clock_delay;
};

struct led_t {
    led_t() = delete;
    led_t(unsigned char pin, bool inverse, LedMode mode) :
        _pin(pin),
        _inverse(inverse),
        _mode(mode)
    {
        init();
    }

    unsigned char pin() const {
        return _pin;
    }

    LedMode mode() const {
        return _mode;
    }

    void mode(LedMode mode) {
        _mode = mode;
    }

    bool inverse() const {
        return _inverse;
    }

    LedPattern& pattern() {
        return _pattern;
    }

    void init();

    void start() {
        _pattern.stop();
    }

    bool started() {
        return _pattern.started();
    }

    void stop() {
        _pattern.stop();
    }

    bool status();
    bool status(bool new_status);

    bool toggle();

private:
    unsigned char _pin;
    bool _inverse;
    LedMode _mode;
    LedPattern _pattern;
};

size_t ledCount();

void ledUpdate(bool do_update);
bool ledStatus(size_t id, bool status);
bool ledStatus(size_t id);
void ledSetup();
