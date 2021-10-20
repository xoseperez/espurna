// -----------------------------------------------------------------------------
// TUYA
// -----------------------------------------------------------------------------

#pragma once

namespace tuya {

void setupChannels();
void sendChannel(unsigned char, unsigned int);

void setupSwitches();
void sendSwitch(unsigned char, bool);

void setup();

} // namespace tuya
