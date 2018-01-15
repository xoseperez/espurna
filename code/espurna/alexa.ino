/*

ALEXA MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ALEXA_SUPPORT

#include <fauxmoESP.h>
#include <map>
fauxmoESP alexa;

// -----------------------------------------------------------------------------
// ALEXA
// -----------------------------------------------------------------------------
struct alexa_dev {
  bool _alexa_change = false;
  bool _alexa_state = false;
};
std::map<unsigned char, alexa_dev> alexa_devices;

void _alexaWebSocketOnSend(JsonObject& root) {
    root["alexaVisible"] = 1;
    root["alexaEnabled"] = getSetting("alexaEnabled", ALEXA_ENABLED).toInt() == 1;
}

void _alexaConfigure() {
    alexa.enable(getSetting("alexaEnabled", ALEXA_ENABLED).toInt() == 1);
}

// -----------------------------------------------------------------------------

void alexaSetup() {

    // Backwards compatibility
    moveSetting("fauxmoEnabled", "alexaEnabled");

    // Load & cache settings
    _alexaConfigure();

    #if WEB_SUPPORT

        // Websockets
        wsOnSendRegister(_alexaWebSocketOnSend);
        wsOnAfterParseRegister(_alexaConfigure);

    #endif

    unsigned int relays = relayCount();
    String hostname = getSetting("hostname");
    if (relays == 1) {
        alexa.addDevice(hostname.c_str());
    } else {
        for (unsigned int i=0; i<relays; i++) {
            alexa.addDevice((hostname + "_" + i).c_str());
        }
    }

    alexa.onSetState([&](unsigned char device_id, const char * name, bool state) {
        alexa_devices[device_id]._alexa_change = true;
        alexa_devices[device_id]._alexa_state = state;
    });

    alexa.onGetState([relays](unsigned char device_id, const char * name) {
        return relayStatus(device_id);
    });

}

void alexaLoop() {

    alexa.handle();

    for (std::map<unsigned char, alexa_dev>::iterator it=alexa_devices.begin(); it!=alexa_devices.end(); ++it) {
      if (it->second._alexa_change) {
          DEBUG_MSG_P(PSTR("[ALEXA] Device #%d state: %s\n"), it->first, it->second._alexa_state ? "ON" : "OFF");
          it->second._alexa_change = false;
          relayStatus(it->first, it->second._alexa_state);
      }
    }

}

#endif
