/*

BLYNK MODULE

Copyright (C) 2018 by Thomas Häger <thaeger at hdsnetz dot de>

*/

#if BLYNK_SUPPORT

bool _blynk_enabled = false;

// ref: blynk-library/src/BlynkSimpleEsp8266.h
// adapted esp8266 class, without 2.4.x requirement
#include <BlynkApiArduino.h>
#include <Blynk/BlynkProtocol.h>
#include <Adapters/BlynkArduinoClient.h>


class BlynkWifi
    : public BlynkProtocol<BlynkArduinoClient>
{
    typedef BlynkProtocol<BlynkArduinoClient> Base;
public:

    BlynkWifi(BlynkArduinoClient& transp)
        : Base(transp)
    {}

    char* getToken() { return _token; }
    char* getHost() { return _host; }
    uint16_t getPort() { return _port; }

    bool isEnabled() {
        return (getSetting("blnkEnabled", BLYNK_ENABLED).toInt() == 1)
            && (getSetting("blnkToken", BLYNK_AUTH_TOKEN).length())
            && (getSetting("blnkHost", BLYNK_HOST).length())
            && (getSetting("blnkPort", BLYNK_PORT).length());
    }

    void config(const char* token,
                const char* host = BLYNK_HOST,
                uint16_t    port = BLYNK_PORT)
    {
        if (_token) free(_token);
        _token = strdup(token);

        if (_host) free(_host);
        _host = strdup(host);

        _port = port;

        Base::begin(_token);
        this->conn.begin(_host, _port);
    }

    void configFromSettings() {
        config(
            getSetting("blnkToken", BLYNK_AUTH_TOKEN).c_str(),
            getSetting("blnkHost", BLYNK_HOST).c_str(),
            getSetting("blnkPort", BLYNK_PORT).toInt()
        );
    }

    bool connectWithBackoff() {
        static uint8_t backoff = 0;
        static uint32_t last_attempt = millis();

        uint32_t timeout = (BLYNK_CONNECTION_TIMEOUT * (backoff + 1));
        if ((millis() - last_attempt) < timeout) {
            return false;
        }

        last_attempt = millis();
        if (!connect(BLYNK_CONNECTION_TIMEOUT)) {
            if (backoff < 15) backoff += 3;
            return false;
        }

        backoff = 1;

        return true;
    }

private:
    char* _token;
    char* _host;
    uint16_t _port;
};

// TODO construct later?
// TODO async client?
WiFiClient _blynkWifiClient;
BlynkArduinoClient _blynkTransport(_blynkWifiClient);
BlynkWifi Blynk(_blynkTransport);


// vpin <-> relays, sensors mapping
bool _blynkVPinRelay(uint8_t vpin, uint8_t* relayID) {
    for (size_t id=0; id<relayCount(); id++) {
        String mapping = getSetting("blnkRelayVPin", id, "");
        if (!mapping.length()) continue;

        if (mapping.toInt() == vpin) {
            *relayID = id;
            return true;
        }
    }

    return false;
}

bool _blynkRelayVPin(uint8_t relayID, uint8_t* vpin) {
    String mapping = getSetting("blnkRelayVPin", relayID, "");
    if (mapping.length()) {
        *vpin = mapping.toInt();
        return true;
    }

    return false;
}

#if WEB_SUPPORT

bool _blnkWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "blnk", 4) == 0);
}

void _blnkWebSocketOnSend(JsonObject& root) {
    root["blnkVisible"] = 1;
    root["blnkEnabled"] = getSetting("blnkEnabled", BLYNK_ENABLED).toInt() == 1;
    root["blnkToken"] = getSetting("blnkToken", BLYNK_AUTH_TOKEN);
    root["blnkHost"] = getSetting("blnkHost", BLYNK_HOST);
    root["blnkPort"] = getSetting("blnkPort", BLYNK_PORT).toInt();

    JsonArray& relays = root.createNestedArray("blnkRelays");
    for (size_t id=0; id<relayCount(); id++) {
        uint8_t vpin = 0;
        bool assigned = _blynkRelayVPin(id, &vpin);

        if (assigned) {
            relays.add(vpin);
        } else {
            relays.add("");
        }
    }

    #if SENSOR_SUPPORT
        JsonArray& list = root.createNestedArray("blnkMagnitudes");
        for (size_t i=0; i<magnitudeCount(); i++) {
            JsonObject& element = list.createNestedObject();
            element["name"] = magnitudeName(i);
            element["type"] = magnitudeType(i);
            element["index"] = magnitudeIndex(i);
            element["idx"] = getSetting("blnkMagnitude", i, "");
        }
    #endif
}

#endif //WEB_SUPPORT

void _blynkConfigure() {
    _blynk_enabled = Blynk.isEnabled();

    if(!_blynk_enabled) {
        Blynk.disconnect();
        return;
    }

    Blynk.configFromSettings();
}

// Public API to send data to the Blynk
void blynkSendMeasurement(unsigned char magnitude_index, char* payload) {
    if (!_blynk_enabled) return;

    String mapping = getSetting("blnkMagnitude", magnitude_index, "");
    if (!mapping.length()) return;

    //DEBUG_MSG_P(PSTR("[BLYNK] Update VPin #%u with Sensor #%u data: %s\n"), mapping.toInt(), magnitude_index, payload);
    Blynk.virtualWrite(mapping.toInt(), payload);
}

void blynkSendRelay(unsigned int relayID, bool status) {
    if (!_blynk_enabled) return;

    uint8_t vpin = 0;
    bool assigned = _blynkRelayVPin(relayID, &vpin);
    if (!assigned) return;

    //DEBUG_MSG_P(PSTR("[BLYNK] Update VPin #%u with Relay #%u status: %u\n"), relayID, status ? 1 : 0);
    Blynk.virtualWrite(vpin, status);
}

void blynkSendRelay(unsigned int relayID) {
    if (!_blynk_enabled) return;
    blynkSendRelay(relayID, relayStatus(relayID));
}

void blynkSendRelays() {
    for (size_t relayID=0; relayID<relayCount(); relayID++) {
        blynkSendRelay(relayID);
    }
}

// ref: blynk-library/src/Blynk/BlynkHandlers.h
// overridable blynk functions
BLYNK_CONNECTED() {
    DEBUG_MSG_P(PSTR("[BLYNK] Connected to %s:%d\n"),
        Blynk.getHost(), Blynk.getPort());
    blynkSendRelays();
}

BLYNK_DISCONNECTED() {
    DEBUG_MSG_P(PSTR("[BLYNK] Disconnected\n"));
}

BLYNK_WRITE_DEFAULT() {
    //DEBUG_MSG_P(PSTR("[BLYNK] Received VPin #%u param \"%s\"\n"), request.pin, param.asStr());

    // TODO receive events not only for relay?
    // do not process values longer that 1 char
    if (param.getLength() != 1) return;

    const bool value = param.asInt() == 1;

    uint8_t relayID = 0;
    bool assigned = _blynkVPinRelay(request.pin, &relayID);

    if (assigned) {
        relayStatus(relayID, value);
    }
}

void blynkLoop(){

    if (!_blynk_enabled) return;
    if (!wifiConnected()) return;

    if (!Blynk.connected()) {
        if (!Blynk.connectWithBackoff()) {
            DEBUG_MSG_P(PSTR("[BLYNK] %s:%u cannot be reached\n"),
                Blynk.getHost(), Blynk.getPort());
            return;
        }
    }

    Blynk.run();

}

void blynkSetup(){

  _blynkConfigure();

  #if WEB_SUPPORT
      wsOnSendRegister(_blnkWebSocketOnSend);
      wsOnReceiveRegister(_blnkWebSocketOnReceive);
  #endif

  espurnaRegisterLoop(blynkLoop);
  espurnaRegisterReload(_blynkConfigure);

}

#endif
