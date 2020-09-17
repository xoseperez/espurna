/*

E1.31 MODULE

Copyright (C) 2020 by Adam Honse <calcprogrammer1 at gmail dot com>

*/

#include "e131.h"

#if E131_SUPPORT

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

// -----------------------------------------------------------------------------

#include "light.h"
#include "wifi.h"
#include "web.h"
#include "ws.h"

#include <ESPAsyncE131.h>

bool         _e131_wifi_connected  = false;
bool         _e131_enabled         = false;
bool         _e131_multicast       = false;
unsigned int _e131_universe        = 1;
bool         _e131_light_0_enabled = false;
unsigned int _e131_light_0_channel = 1;
bool         _e131_light_1_enabled = false;
unsigned int _e131_light_1_channel = 2;
bool         _e131_light_2_enabled = false;
unsigned int _e131_light_2_channel = 3;
bool         _e131_light_3_enabled = false;
unsigned int _e131_light_3_channel = 4;
bool         _e131_light_4_enabled = false;
unsigned int _e131_light_4_channel = 5;
bool         _e131_initialized     = false;

ESPAsyncE131 e131(2);

// -----------------------------------------------------------------------------
// E1.31
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _e131WebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "e131", 4) == 0);
}

void _e131WebSocketOnVisible(JsonObject& root) {
    root["e131Visible"] = 1;
}

void _e131WebSocketOnData(JsonObject& root) {

}

void _e131WebSocketOnConnected(JsonObject& root) {
    root["e131Enabled"]         = getSetting("e131Enabled",       false);
    root["e131Multicast"]       = getSetting("e131Multicast",     false);
    root["e131Universe"]        = getSetting("e131Universe",      1);
    root["e131Light0Enabled"]   = getSetting("e131Light0Enabled", false);
    root["e131Light0Channel"]   = getSetting("e131Light0Channel", 1);
    root["e131Light1Enabled"]   = getSetting("e131Light1Enabled", false);
    root["e131Light1Channel"]   = getSetting("e131Light1Channel", 2);
    root["e131Light2Enabled"]   = getSetting("e131Light2Enabled", false);
    root["e131Light2Channel"]   = getSetting("e131Light2Channel", 3);
    root["e131Light3Enabled"]   = getSetting("e131Light3Enabled", false);
    root["e131Light3Channel"]   = getSetting("e131Light3Channel", 4);
    root["e131Light4Enabled"]   = getSetting("e131Light4Enabled", false);
    root["e131Light4Channel"]   = getSetting("e131Light4Channel", 5);
}

#endif // WEB_SUPPORT

void _e131WifiCallback(justwifi_messages_t code, char * parameter) {

    if (MESSAGE_CONNECTED == code) {
        _e131_wifi_connected = true;
        return;
    }

    if (MESSAGE_DISCONNECTED == code) {
        _e131_wifi_connected = false;
        return;
    }
}

void _e131Loop() {
    if (!_e131_enabled) return;

    //* Initializing multicast mode must be done when the WiFi is connected, so
    //* set a flag to track when WiFi is connected and disconnected
    if (_e131_wifi_connected) {
        if(_e131_initialized == 0) {
            if(_e131_multicast) {
                e131.begin(E131_MULTICAST, _e131_universe, 1);
            }
            else {
                e131.begin(E131_UNICAST);
            }
            
            _e131_initialized = 1;
        }
    }
    else {
        _e131_initialized = 0;
    }

    if(!e131.isEmpty())
    {
        e131_packet_t pkt;
        e131.pull(&pkt);

        if(_e131_light_0_enabled) {
            lightChannel(0, pkt.property_values[_e131_light_0_channel]);
        }

        if(_e131_light_1_enabled) {
            lightChannel(1, pkt.property_values[_e131_light_1_channel]);
        }

        if(_e131_light_2_enabled) {
            lightChannel(2, pkt.property_values[_e131_light_2_channel]);
        }

        if(_e131_light_3_enabled) {
            lightChannel(3, pkt.property_values[_e131_light_3_channel]);
        }

        if(_e131_light_4_enabled) {
            lightChannel(4, pkt.property_values[_e131_light_4_channel]);
        }

        lightUpdate(false, false, false);
    }
}

bool e131Enabled() {
    return _e131_enabled;
}

void e131Setup() {
    _e131_initialized           = 0;
    _e131_enabled               = getSetting("e131Enabled",       false);
    _e131_multicast             = getSetting("e131Multicast",     false);
    _e131_universe              = getSetting("e131Universe",      1);
    _e131_light_0_enabled       = getSetting("e131Light0Enabled", false);
    _e131_light_0_channel       = getSetting("e131Light0Channel", 1);
    _e131_light_1_enabled       = getSetting("e131Light1Enabled", false);
    _e131_light_1_channel       = getSetting("e131Light1Channel", 2);
    _e131_light_2_enabled       = getSetting("e131Light2Enabled", false);
    _e131_light_2_channel       = getSetting("e131Light2Channel", 3);
    _e131_light_3_enabled       = getSetting("e131Light3Enabled", false);
    _e131_light_3_channel       = getSetting("e131Light3Channel", 4);
    _e131_light_4_enabled       = getSetting("e131Light4Enabled", false);
    _e131_light_4_channel       = getSetting("e131Light4Channel", 5);

    espurnaRegisterLoop(_e131Loop);

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_e131WebSocketOnVisible)
            .onConnected(_e131WebSocketOnConnected)
            .onData(_e131WebSocketOnData)
            .onKeyCheck(_e131WebSocketOnKeyCheck);
    #endif

    jw.subscribe(_e131WifiCallback);

    DEBUG_MSG_P(PSTR("[E131] E131 setup code finished \n"));
}

#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#endif // E131_SUPPORT