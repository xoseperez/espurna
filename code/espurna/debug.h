/*

DEBUG MODULE

*/

#pragma once

extern "C" {
     void custom_crash_callback(struct rst_info*, uint32_t, uint32_t);
}

class PrintRaw;
class PrintHex;

bool debugLogBuffer();
void debugSend(const char* format, ...);
void debugSend_P(PGM_P format_P, ...);

void debugWebSetup();
void debugSetup();
