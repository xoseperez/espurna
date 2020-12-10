/*

LLMNR MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "llmnr.h"

#if LLMNR_SUPPORT

void llmnrSetup() {
    LLMNR.begin(getSetting("hostname").c_str());
    DEBUG_MSG_P(PSTR("[LLMNR] Configured\n"));
}

#endif // LLMNR_SUPPORT
