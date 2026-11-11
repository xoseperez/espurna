/*

RTMEM MODULE FOR ESP32

*/

#pragma once

#include <Arduino.h>
#include <cstdint>

#define RTCMEM_BLOCKS 128u
#define RTCMEM_MAGIC 0x46535076

struct RtcmemEnergy {
    uint32_t kwh;
    uint32_t ws;
};

struct RtcmemData {
    uint32_t magic;
    uint32_t sys;
    uint32_t relay;
    uint32_t mqtt;
    uint64_t light;
    RtcmemEnergy energy[4];
    uint32_t gpio_ignore;
};

static_assert(sizeof(RtcmemData) <= (RTCMEM_BLOCKS * 4u), "RTCMEM struct is too big");

extern volatile RtcmemData* Rtcmem;

bool rtcmemStatus();
void rtcmemSetup();
