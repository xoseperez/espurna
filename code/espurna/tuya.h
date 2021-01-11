// -----------------------------------------------------------------------------
// TUYA
// -----------------------------------------------------------------------------

#pragma once

#include "espurna.h"

namespace tuya {

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
void setupChannels();
void sendChannel(unsigned char, unsigned int);
#endif

#if RELAY_SUPPORT
void setupSwitches();
void sendSwitch(unsigned char, bool);
#endif

void setup();

} // namespace tuya
