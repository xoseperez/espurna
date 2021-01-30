/*

LLMNR MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "llmnr.h"

#if LLMNR_SUPPORT

#include <ESP8266LLMNR.h>

void llmnrSetup() {
    auto hostname = getSetting("hostname", getIdentifier());
    LLMNR.begin(hostname.c_str());
    DEBUG_MSG_P(PSTR("[LLMNR] Configured for %s\n"), hostname.c_str());
}

#endif // LLMNR_SUPPORT
