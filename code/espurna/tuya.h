// -----------------------------------------------------------------------------
// TUYA
// -----------------------------------------------------------------------------

#pragma once

#include "espurna.h"

namespace tuya {

void sendChannel(unsigned char, unsigned int);
void sendSwitch(unsigned char, bool);
void setup();

}
