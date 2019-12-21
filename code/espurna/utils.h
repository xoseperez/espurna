/*

UTILS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#define INLINE inline __attribute__((always_inline))

extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

String getIdentifier();
void setDefaultHostname();

void setBoardName();
String getBoardName();
String getAdminPass();

const String& getCoreVersion();
const String& getCoreRevision();

unsigned char getHeartbeatMode();
unsigned char getHeartbeatInterval();
void heartbeat();

String getEspurnaModules();
String getEspurnaOTAModules();
String getEspurnaSensors();

String getEspurnaWebUI();

String buildTime();
unsigned long getUptime();
bool haveRelaysOrSensors();

void infoMemory(const char * name, unsigned int total_memory, unsigned int free_memory);
void info();

bool sslCheckFingerPrint(const char * fingerprint);
bool sslFingerPrintArray(const char * fingerprint, unsigned char * bytearray);
bool sslFingerPrintChar(const char * fingerprint, char * destination);

bool eraseSDKConfig();

char * ltrim(char * s);
char * strnstr(const char * buffer, const char * token, size_t n);
bool isNumber(const char * s);

void nice_delay(unsigned long ms);

double roundTo(double num, unsigned char positions);

uint32_t u32fromString(const String& string, int base);
uint32_t u32fromString(const String& string);
String u32toString(uint32_t bitset, int base);
