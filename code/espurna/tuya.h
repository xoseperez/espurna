// -----------------------------------------------------------------------------
// TUYA
// -----------------------------------------------------------------------------

#pragma once

#include "espurna.h"

namespace Tuya {
    void tuyaSendChannel(unsigned char, unsigned int);
    void tuyaSendSwitch(unsigned char, bool);
    void tuyaSetup();
    void tuyaSetupLight();
    void tuyaSyncSwitchStatus();
    void tuyaSetupSwitch();
}
