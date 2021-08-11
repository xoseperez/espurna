/*

DEBUG MODULE

*/

#pragma once

#include <cstdint>

class PrintRaw;
class PrintHex;

enum class DebugLogMode : int {
    Disabled = 0,
    Enabled = 1,
    SkipBoot = 2
};

bool debugLogBuffer();

void debugWebSetup();
void debugConfigure();
void debugConfigureBoot();
void debugSetup();

void debugSendRaw(const char* line, bool timestamp = false);
void debugSendBytes(const uint8_t* bytes, size_t size);

void debugSend(const char* format, ...);
