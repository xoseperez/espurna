/*

UTILS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

struct heap_stats_t {
    uint32_t available;
    uint16_t usable;
    uint8_t frag_pct;
};

PROGMEM const char pstr_unknown[] = "UNKNOWN";

#define INLINE inline __attribute__((always_inline))

extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

void setDefaultHostname();

void setBoardName();

const String& getDevice();
const String& getManufacturer();
const String& getCoreVersion();
const String& getCoreRevision();

int getHeartbeatMode();
unsigned long getHeartbeatInterval();
void heartbeat();

String getAdminPass();
String getBoardName();
String buildTime();
unsigned long getUptime();
bool haveRelaysOrSensors();

void getHeapStats(heap_stats_t& stats);
heap_stats_t getHeapStats();
void wtfHeap(bool value);
unsigned int getFreeHeap();
void setInitialFreeHeap();
unsigned int getInitialFreeHeap();

void infoHeapStats(const char* name, const heap_stats_t& stats);
void infoHeapStats(bool show_frag_stats = true);
void infoMemory(const char * name, unsigned int total_memory, unsigned int free_memory);
void infoUptime();
void info(bool first = false);

bool sslCheckFingerPrint(const char * fingerprint);
bool sslFingerPrintArray(const char * fingerprint, unsigned char * bytearray);
bool sslFingerPrintChar(const char * fingerprint, char * destination);

bool eraseSDKConfig();

char * ltrim(char * s);
char * strnstr(const char * buffer, const char * token, size_t n);
bool isNumber(const char * s);

void nice_delay(unsigned long ms);

double roundTo(double num, unsigned char positions);
