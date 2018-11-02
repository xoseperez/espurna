/*

SERIAL_BRIDGE MODULE

Copyright (C) 2018 by Throsten von Eicken

Module key prefix: sbr

*/

#if SERIAL_BRIDGE_SUPPORT

#include "SerialBridge.h"

SerialBridge _sbr;

#if WEB_SUPPORT

void _sbrWebSocketOnSend(JsonObject& root) {
    root["sbrVisible"] = 1;
    root["sbrPort"] = 1234;
    root["sbrBaud"] = 38400;
}

void _sbrWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    //if (strcmp(action, "rfblearn") == 0) _rfLearn(data["id"], data["status"]);
    //if (strcmp(action, "rfbforget") == 0) _rfForget(data["id"], data["status"]);
    //if (strcmp(action, "rfbsend") == 0) _rfStore(data["id"], data["status"], data["data"].as<long>());
}

#endif

bool _sbrKeyCheck(const char * key) {
    return (strncmp(key, "sbr", 3) == 0);
}

void _sbrLoop() {
    _sbr.loop();
}

void serialBridgeSetup() {
    _sbr.debug(&debugSend_P);
    _sbr.begin(2323);

    #if WEB_SUPPORT
        wsOnSendRegister(_sbrWebSocketOnSend);
        wsOnActionRegister(_sbrWebSocketOnAction);
    #endif

    settingsRegisterKeyCheck(_sbrKeyCheck);
    espurnaRegisterLoop(_sbrLoop);
}

#endif
