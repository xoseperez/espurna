/*

DEBUG MODULE

*/

#pragma once

#include <cstdint>
#include <cstddef>

class PrintRaw;
class PrintHex;

enum class DebugLogMode : int {
    Disabled = 0,
    Enabled = 1,
    SkipBoot = 2
};

using DebugPrefix = char[10];

static constexpr bool debugWithPrefix(const DebugPrefix& prefix) {
    return prefix[0] != '\0';
}

static constexpr size_t debugPrefixLength(const DebugPrefix& prefix) {
    return debugWithPrefix(prefix)
        ? (sizeof(DebugPrefix) - 1)
        : 0;
}

bool debugLogBuffer();

void debugWebSetup();
void debugConfigure();
void debugConfigureBoot();
void debugShowBanner();
void debugSetup();

void debugSendRaw(const char* line, bool timestamp = false);
void debugSendBytes(const uint8_t* bytes, size_t size);

void debugSend(const char* format, ...);
