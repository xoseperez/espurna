/*

DEBUG MODULE

*/

#pragma once

#include <pgmspace.h>

extern "C" {
    void custom_crash_callback(struct rst_info*, uint32_t, uint32_t);
}

class PrintRaw;
class PrintHex;

enum class DebugLogMode : int {
    DISABLED = 0,
    ENABLED = 1,
    SKIP_BOOT = 2
};

bool debugLogBuffer();

void debugWebSetup();
void debugConfigure();
void debugConfigueBoot();
void debugSetup();

void debugSend(const char* format, ...);
void debugSend_P(PGM_P format, ...); // PGM_P is `const char*`

#if DEBUG_SUPPORT
    #define DEBUG_MSG(...) debugSend(__VA_ARGS__)
    #define DEBUG_MSG_P(...) debugSend_P(__VA_ARGS__)
#endif

#ifndef DEBUG_MSG
    #define DEBUG_MSG(...)
    #define DEBUG_MSG_P(...)
#endif

