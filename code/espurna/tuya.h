// -----------------------------------------------------------------------------
// TUYA
// -----------------------------------------------------------------------------

#pragma once

namespace Tuya {
    void tuyaSendChannel(unsigned char, unsigned int);
    void tuyaSendSwitch(unsigned char, bool);
    void tuyaSetup();
    void tuyaSetupLight();
    void tuyaSyncSwitchStatus();
    void tuyaSetupSwitch();
}

using Tuya::tuyaSetup;
using Tuya::tuyaSetupSwitch;
using Tuya::tuyaSyncSwitchStatus;
using Tuya::tuyaSendSwitch;

#if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
    using Tuya::tuyaSetupLight;
    using Tuya::tuyaSendChannel;
#endif

