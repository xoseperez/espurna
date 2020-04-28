/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <vector>
#include <memory>

constexpr size_t LedsMax = 8;

enum class LedMode {
    NetworkAutoconfig,
    NetworkConnected,
    NetworkConnectedInverse,
    NetworkConfig,
    NetworkConfigInverse,
    NetworkIdle,
    None
};

enum class led_delay_mode_t {
    Finite,
    Infinite,
    None
};

struct led_delay_t {
    led_delay_t() = delete;
    led_delay_t(unsigned long on_ms, unsigned long off_ms);
    led_delay_t(unsigned long on_ms, unsigned long off_ms, unsigned char repeats);

    led_delay_mode_t type;
    unsigned long on;
    unsigned long off;
    unsigned char repeats;
};

struct led_pattern_t {
    led_pattern_t() = default;
    led_pattern_t(const std::vector<led_delay_t>& delays);

    void start();
    void stop();

    bool started();
    bool ready();

    std::vector<led_delay_t> delays;
    std::vector<led_delay_t> queue;
    unsigned long clock_last;
    unsigned long clock_delay;
};

struct led_t {
    led_t() = delete;
    led_t(unsigned char pin, bool inverse, unsigned char mode, unsigned char relayID);

    bool status();
    bool status(bool new_status);

    bool toggle();

    unsigned char pin;
    bool inverse;
    unsigned char mode;
    unsigned char relayID;

    led_pattern_t pattern;
};

void ledUpdate(bool do_update);
unsigned char ledCount();
bool ledStatus(unsigned char id, bool status);
bool ledStatus(unsigned char id);
void ledSetup();

