/*

UTILS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

#include "system.h"

#define INLINE inline __attribute__((always_inline))

extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

void setDefaultHostname();
void setBoardName();

const String& getDevice();
const String& getManufacturer();
const String& getCoreVersion();
const String& getCoreRevision();
const String& getVersion();

String getAdminPass();
String getBoardName();
String buildTime();
bool haveRelaysOrSensors();

String getUptime();

void infoHeapStats(const char* name, const HeapStats& stats);
void infoHeapStats(bool show_frag_stats = false);
void infoMemory(const char* name, unsigned int total_memory, unsigned int free_memory);
void info(bool first = false);

bool sslCheckFingerPrint(const char * fingerprint);
bool sslFingerPrintArray(const char * fingerprint, unsigned char * bytearray);
bool sslFingerPrintChar(const char * fingerprint, char * destination);

char* ltrim(char* s);
char* strnstr(const char* buffer, const char* token, size_t n);
bool isNumber(const char* s);

void nice_delay(unsigned long ms);

double roundTo(double num, unsigned char positions);

size_t hexEncode(const uint8_t* in, size_t in_size, char* out, size_t out_size);
size_t hexDecode(const char* in, size_t in_size, uint8_t* out, size_t out_size);
