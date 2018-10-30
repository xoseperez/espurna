/*

SERIAL_BRIDGE MODULE

Copyright (C) 2018 by Throsten von Eicken

Module key prefix: sbr

*/

#if SERIAL_BRIDGE_SUPPORT

#include "SerialBridge.h"

SerialBridge _sbr;

bool _sbrKeyCheck(const char * key) {
    return (strncmp(key, "sbr", 3) == 0);
}

void _sbrLoop() {
    _sbr.loop();
}

void serialBridgeSetup() {
    _sbr.debug(&debugSend_P);
    _sbr.begin(2323);
    settingsRegisterKeyCheck(_sbrKeyCheck);
    espurnaRegisterLoop(_sbrLoop);
}

#endif
