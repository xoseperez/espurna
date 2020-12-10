/*

RTMEM MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>
#include <cstdint>

#include "espurna.h"

// Base address of USER RTC memory
// https://github.com/esp8266/esp8266-wiki/wiki/Memory-Map#memmory-mapped-io-registers
#define RTCMEM_ADDR_BASE (0x60001200)

// RTC memory is accessed using blocks of 4 bytes.
// Blocks 0..63 are reserved by the SDK, 64..192 are available to the user.
// Blocks 64..96 are reserved by the eboot 'struct eboot_command' (128 -> (128 / 4) -> 32):
// https://github.com/esp8266/Arduino/blob/master/bootloaders/eboot/eboot_command.h
#define RTCMEM_OFFSET 32u
#define RTCMEM_ADDR (RTCMEM_ADDR_BASE + (RTCMEM_OFFSET * 4u))

#define RTCMEM_BLOCKS 96u

// Change this when modifying RtcmemData
#define RTCMEM_MAGIC 0x46535076

// XXX: All access must be 4-byte aligned and always at full length.
//      Exactly like PROGMEM works. For example, using bitfields / inner structs / etc:
// ...
// uint32_t a : 8;
// uint32_t b : 8;
// uint32_t c : 8;
// uint32_t d : 8;
// ...
//
// This would not write the expected thing:
// mem->d = 4;

// TODO replace with custom memory segment in ldscript?
//      `magic` would need to be tracked differently

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
};

static_assert(sizeof(RtcmemData) <= (RTCMEM_BLOCKS * 4u), "RTCMEM struct is too big");
constexpr uint8_t RtcmemSize = (sizeof(RtcmemData) / 4u);

extern volatile RtcmemData* Rtcmem;

bool rtcmemStatus();
void rtcmemSetup();
