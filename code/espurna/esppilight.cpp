#if ESPILIGHT_SUPPORT

#include <ESPiLight.h>
#include "mqtt.h"

ESPiLight* _receiver = nullptr;

void _rfCallback(const String& protocol, const String& message, int status, size_t repeats, const String& deviceId) {
    DEBUG_MSG_P(PSTR("[RF] protocol=%s message=%s status=%d repeats=%u deviceId=%s\n"),
        protocol.c_str(), message.c_str(), status, repeats, deviceId.c_str());
}

rfESPiLightSetup() {
    const auto rx = getSetting("rfbRX", RFB_RX_PIN);
    if (!gpioGetLock(rx)) return;

    _receiver = new ESPiLight(-1);
    _receiver.setCallback(_rfCallback);
    _receiver.initReceiver(rx);

    espurnaRegisterLoop([]() { _receiver.loop(); });
}

#endif

