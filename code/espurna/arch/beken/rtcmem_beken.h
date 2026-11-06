/*

RTMEM MODULE FOR BEKEN (LibreTiny)

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

// Only Rtcmem->sys survives a reset here (ref. rtcmem_beken.cpp)
bool rtcmemSysStatus();
uint32_t rtcmemRetention();

// Exception that caused the last reset, written into the register by our crash handlers
enum class RtcmemCrash : uint32_t {
    None = 0,
    Undefined,
    PrefetchAbort,
    DataAbort,
    Reserved,
    // Only in the flash record (written by the malloc failed hook), never in the register
    MallocFailed,
};

RtcmemCrash rtcmemCrash();

// Registers and stack of the last crash, kept in the flash (ref. rtcmem_beken.cpp). Setup reports a new one once,
// in the log and on MQTT, dump prints the whole record (CRASH command)
void rtcmemCrashSetup();
void rtcmemCrashDump(Print&);
