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
        : Base(transp),
          _token(nullptr),
          _host(nullptr),
          _port(0),
          backoff(0),
          backoff_step(3),
          backoff_limit(15),
          last_attempt(0)
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

    bool reachedBackoffLimit() {
        return (backoff >= backoff_limit);
    }

    bool connectWithBackoff() {
        if (!last_attempt) last_attempt = millis();

        uint32_t timeout = (0 == backoff)
            ? BLYNK_CONNECTION_TIMEOUT
            : (BLYNK_CONNECTION_TIMEOUT * backoff);

        if ((millis() - last_attempt) < timeout) {
            return false;
        }

        last_attempt = millis();
        if (!connect(BLYNK_CONNECTION_TIMEOUT)) {
            if (!reachedBackoffLimit()) backoff += backoff_step;
            return false;
        }

        backoff = 0;

        return true;
    }

private:
    char* _token;
    char* _host;
    uint16_t _port;

    uint8_t backoff;
    uint8_t backoff_step;
    uint8_t backoff_limit;

    uint32_t last_attempt;

};

// TODO construct later?
// TODO async client?
WiFiClient _blynkWifiClient;
BlynkArduinoClient _blynkTransport(_blynkWifiClient);
BlynkWifi Blynk(_blynkTransport);


// vpin <-> relays, sensors mapping
bool _blynkIndexFromVPin(std::function<uint8_t()> range_func, const char* key, uint8_t vpin, uint8_t* res) {
    for (size_t id=0; id < range_func(); id++) {
        String mapping = getSetting(key, id, "");
        if (!mapping.length()) continue;

        if (strtoul(mapping.c_str(), nullptr, 10) == vpin) {
            *res = id;
            return true;
        }
    }

    return false;
}

bool _blynkVPinFromIndex(const char* key, uint8_t index, uint8_t* vpin) {
    String mapping = getSetting(key, index, "");
    if (mapping.length()) {
        *vpin = strtoul(mapping.c_str(), nullptr, 10);
        return true;
    }

    return false;
}


bool _blynkVPinRelay(uint8_t vpin, uint8_t* relayID) {
    return _blynkIndexFromVPin(relayCount, "blnkRelay", vpin, relayID);
}

bool _blynkRelayVPin(uint8_t relayID, uint8_t* vpin) {
    return _blynkVPinFromIndex("blnkRelay", relayID, vpin);
}

bool _blynkVPinMagnitude(uint8_t vpin, uint8_t* magnitudeIndex) {
    return _blynkIndexFromVPin(magnitudeCount, "blnkMagnitude", vpin, magnitudeIndex);
}

bool _blynkMagnitudeVPin(uint8_t magnitudeIndex, uint8_t* vpin) {
    return _blynkVPinFromIndex("blnkMagnitude", magnitudeIndex, vpin);
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

            uint8_t vpin = 0;
            bool assigned = _blynkMagnitudeVPin(i, &vpin);

            if (assigned) {
                element["idx"] = vpin;
            } else {
                element["idx"] = "";
            }
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
