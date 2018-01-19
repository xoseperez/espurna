/*

RF MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if RF_SUPPORT

#include <RemoteReceiver.h>

unsigned long rfCode = 0;
unsigned long rfCodeON = 0;
unsigned long rfCodeOFF = 0;

// -----------------------------------------------------------------------------
// RF
// -----------------------------------------------------------------------------

void _rfWebSocketOnSend(JsonObject& root) {
    root["rfVisible"] = 1;
    root["rfChannel"] = getSetting("rfChannel", RF_CHANNEL);
    root["rfDevice"] = getSetting("rfDevice", RF_DEVICE);
}

void _rfBuildCodes() {

    unsigned long code = 0;

    // channel
    unsigned int channel = getSetting("rfChannel", RF_CHANNEL).toInt();
    for (byte i = 0; i < 5; i++) {
        code *= 3;
        if (channel & 1) code += 1;
        channel >>= 1;
    }

    // device
    unsigned int device = getSetting("rfDevice", RF_DEVICE).toInt();
    for (byte i = 0; i < 5; i++) {
        code *= 3;
        if (device != i) code += 2;
    }

    // status
    code *= 9;
    rfCodeOFF = code + 2;
    rfCodeON = code + 6;

    DEBUG_MSG_P(PSTR("[RF] Code ON : %lu\n"), rfCodeON);
    DEBUG_MSG_P(PSTR("[RF] Code OFF: %lu\n"), rfCodeOFF);

}

// -----------------------------------------------------------------------------

void rfLoop() {
    return;
    if (rfCode == 0) return;
    DEBUG_MSG_P(PSTR("[RF] Received code: %lu\n"), rfCode);
    if (rfCode == rfCodeON) relayStatus(0, true);
    if (rfCode == rfCodeOFF) relayStatus(0, false);
    rfCode = 0;
}

void rfCallback(unsigned long code, unsigned int period) {
    rfCode = code;
}

void rfSetup() {

    pinMode(RF_PIN, INPUT_PULLUP);
    _rfBuildCodes();
    RemoteReceiver::init(RF_PIN, 3, rfCallback);
    RemoteReceiver::disable();
    DEBUG_MSG_P(PSTR("[RF] Disabled\n"));

    static WiFiEventHandler e1 = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
        RemoteReceiver::disable();
        DEBUG_MSG_P(PSTR("[RF] Disabled\n"));
    });

    static WiFiEventHandler e2 = WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected& event) {
        RemoteReceiver::disable();
        DEBUG_MSG_P(PSTR("[RF] Disabled\n"));
        });

    static WiFiEventHandler e3 = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected& event) {
        RemoteReceiver::enable();
        DEBUG_MSG_P(PSTR("[RF] Enabled\n"));
    });

    static WiFiEventHandler e4 = WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected& event) {
        RemoteReceiver::enable();
        DEBUG_MSG_P(PSTR("[RF] Enabled\n"));
    });

    #if WEB_SUPPORT
        wsOnSendRegister(_rfWebSocketOnSend);
        wsOnAfterParseRegister(_rfBuildCodes);
    #endif

    // Register loop
    espurnaRegisterLoop(rfLoop);

}

#endif
