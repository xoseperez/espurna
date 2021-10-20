/*

LLMNR MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if LLMNR_SUPPORT

#include <ESP8266LLMNR.h>

#include "llmnr.h"

void llmnrSetup() {
    auto hostname = getHostname();
    LLMNR.begin(hostname.c_str());
    DEBUG_MSG_P(PSTR("[LLMNR] Configured for %s\n"), hostname.c_str());
}

#endif // LLMNR_SUPPORT
