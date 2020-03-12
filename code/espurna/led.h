/*

LED MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

struct led_t {
    led_t();
    led_t(unsigned char id);

    bool status();
    bool status(bool new_status);

    bool toggle();

    unsigned char pin;
    bool inverse;
    unsigned char mode;
    unsigned char relayID;
};

struct led_delay_t {
    led_delay_t(unsigned long on_ms, unsigned long off_ms);
    const unsigned long on;
    const unsigned long off;
};

enum class LedMode {
    NetworkAutoconfig,
    NetworkConnected,
    NetworkConnectedInverse,
    NetworkConfig,
    NetworkConfigInverse,
    NetworkIdle,
    None
};

const led_delay_t& _ledGetDelay(LedMode mode);
void ledUpdate(bool do_update);
void ledSetup();

