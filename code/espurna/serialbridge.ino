/*

SERIAL_BRIDGE MODULE

Copyright (C) 2018 by Throsten von Eicken

Module key prefix: sbr

*/

#if SERIAL_BRIDGE_SUPPORT

#include "SerialBridge.h"

SerialBridge _sbr;

// Settings
int _sbrPort, _sbrBaud, _sbrRxBuf;
int _sbrAvrBaud;

#if WEB_SUPPORT

void _sbrWebSocketOnSend(JsonObject& root) {
    root["sbrVisible"] = 1;
    root["sbrPort"] = getSetting("sbrPort", SBR_PORT);
    root["sbrBaud"] = getSetting("sbrBaud", SBR_BAUD);
    root["sbrRxBuf"] = getSetting("sbrRxBuf", SBR_RXBUF);
    root["sbrAvrBaud"] = getSetting("sbrAvrBaud", SBR_AVRBAUD);
    root["sbrAvrReset"] = getSetting("sbrAvrReset", SBR_AVRRESET);
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
    _sbr.begin(
        getSetting("sbrPort", SBR_PORT).toInt(),
        getSetting("sbrBaud", SBR_BAUD).toInt(),
        getSetting("sbrRxBuf", SBR_RXBUF).toInt());

    #if WEB_SUPPORT
        wsOnSendRegister(_sbrWebSocketOnSend);
        wsOnActionRegister(_sbrWebSocketOnAction);
    #endif

    settingsRegisterKeyCheck(_sbrKeyCheck);
    espurnaRegisterLoop(_sbrLoop);
}

#endif
