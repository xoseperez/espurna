/*

RF MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if RF_SUPPORT

#include <RCSwitch.h>

RCSwitch * _rfModem;

unsigned long _rf_learn_start = 0;
unsigned char _rf_learn_id = 0;
bool _rf_learn_status = true;
bool _rf_learn_active = false;

// -----------------------------------------------------------------------------
// RF
// -----------------------------------------------------------------------------

unsigned long _rfRetrieve(unsigned char id, bool status) {
    String code = getSetting(status ? "rfbON" : "rfbOFF", id, "0");
    return strtoul(code.c_str(), 0, 16);
}

void _rfStore(unsigned char id, bool status, unsigned long code) {
    DEBUG_MSG_P(PSTR("[RF] Storing %d-%s => %X\n"), id, status ? "ON" : "OFF", code);
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer), PSTR("%X"), code);
    setSetting(status ? "rfbON" : "rfbOFF", id, buffer);
}

void _rfLearn(unsigned char id, bool status) {
    _rf_learn_start = millis();
    _rf_learn_id = id;
    _rf_learn_status = status;
    _rf_learn_active = true;
}

void _rfForget(unsigned char id, bool status) {

    delSetting(status ? "rfbON" : "rfbOFF", id);

    // Websocket update
    #if WEB_SUPPORT
        char wsb[100];
        snprintf_P(wsb, sizeof(wsb), PSTR("{\"rfb\":[{\"id\": %d, \"status\": %d, \"data\": \"\"}]}"), id, status ? 1 : 0);
        wsSend(wsb);
    #endif

}


bool _rfMatch(unsigned long code, unsigned char& relayID, unsigned char& value) {

    bool found = false;
    DEBUG_MSG_P(PSTR("[RF] Trying to match code %X\n"), code);

    for (unsigned char i=0; i<relayCount(); i++) {

        unsigned long code_on = _rfRetrieve(i, true);
        unsigned long code_off = _rfRetrieve(i, false);

        if (code == code_on) {
            DEBUG_MSG_P(PSTR("[RF] Match ON code for relay %d\n"), i);
            value = 1;
            found = true;
        }

        if (code == code_off) {
            DEBUG_MSG_P(PSTR("[RF] Match OFF code for relay %d\n"), i);
            if (found) value = 2;
            found = true;
        }

        if (found) {
            relayID = i;
            return true;
        }

    }

    return false;

}

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

void _rfWebSocketOnSend(JsonObject& root) {
    char buffer[20];
    root["rfbVisible"] = 1;
    root["rfbCount"] = relayCount();
    JsonArray& rfb = root.createNestedArray("rfb");
    for (byte id=0; id<relayCount(); id++) {
        for (byte status=0; status<2; status++) {
            JsonObject& node = rfb.createNestedObject();
            snprintf_P(buffer, sizeof(buffer), PSTR("%X"), _rfRetrieve(id, status == 1));
            node["id"] = id;
            node["status"] = status;
            node["data"] = String(buffer);
        }
    }
}

void _rfWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "rfblearn") == 0) _rfLearn(data["id"], data["status"]);
    if (strcmp(action, "rfbforget") == 0) _rfForget(data["id"], data["status"]);
    if (strcmp(action, "rfbsend") == 0) _rfStore(data["id"], data["status"], data["data"].as<long>());
}

// -----------------------------------------------------------------------------

void rfLoop() {

    if (_rfModem->available()) {

        static unsigned long last = 0;
        if (millis() - last > RF_DEBOUNCE) {
            last = millis();

            if (_rfModem->getReceivedValue() > 0) {

                unsigned long rf_code = _rfModem->getReceivedValue();

                DEBUG_MSG_P(PSTR("[RF] Received code: %X\n"), rf_code);

                if (_rf_learn_active) {

                    _rf_learn_active = false;

                    _rfStore(_rf_learn_id, _rf_learn_status, rf_code);

                    // Websocket update
                    #if WEB_SUPPORT
                        char wsb[100];
                        snprintf_P(
                            wsb, sizeof(wsb),
                            PSTR("{\"rfb\":[{\"id\": %d, \"status\": %d, \"data\": \"%X\"}]}"),
                            _rf_learn_id, _rf_learn_status ? 1 : 0, rf_code);
                        wsSend(wsb);
                    #endif

                } else {

                    unsigned char id;
                    unsigned char value;
                    if (_rfMatch(rf_code, id, value)) {
                        if (2 == value) {
                            relayToggle(id);
                        } else {
                            relayStatus(id, 1 == value);
                        }
                    }

                }

            }

        }

        _rfModem->resetAvailable();

    }

    if (_rf_learn_active && (millis() - _rf_learn_start > RF_LEARN_TIMEOUT)) {
        _rf_learn_active = false;
    }

}

void rfSetup() {

    _rfModem = new RCSwitch();
    _rfModem->enableReceive(RF_PIN);
    DEBUG_MSG_P(PSTR("[RF] RF receiver on GPIO %u\n"), RF_PIN);

    #if WEB_SUPPORT
        wsOnSendRegister(_rfWebSocketOnSend);
        wsOnActionRegister(_rfWebSocketOnAction);
    #endif

    // Register loop
    espurnaRegisterLoop(rfLoop);

}

#endif
