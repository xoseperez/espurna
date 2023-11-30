// -----------------------------------------------------------------------------
// TUYA
// -----------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <memory>

class RelayProviderBase;

namespace tuya {

void setupChannels();
void sendChannel(unsigned char, unsigned int);

void setupSwitches();
void sendSwitch(unsigned char, bool);

void setup();

std::unique_ptr<RelayProviderBase> makeRelayProvider(size_t);

} // namespace tuya
