/*

BLYNK MODULE

Copyright (C) 2018 by Thomas Häger <thaeger at hdsnetz dot de>

*/

#if BLYNK_SUPPORT

//
// Blynk wrapper
//

// ref: blynk-library/src/BlynkSimpleEsp8266.h
// adapted esp8266 class, without 2.4.x requirement
#include <Blynk/BlynkConfig.h>
#include <BlynkApiArduino.h>
#include <Blynk/BlynkProtocol.h>
#include <Adapters/BlynkArduinoClient.h>

#if BLYNK_SECURE_CLIENT

#include <TimeLib.h>
#include <WiFiClientSecure.h>

#if BLYNK_USE_CUSTOM_CERT
#include "config/user_blynk_certificate.h"
#else
#include "config/blynk_certificate.h"
#endif

template <typename Client>
class BlynkArduinoClientSecure
    : public BlynkArduinoClientGen<Client>
{
public:
    BlynkArduinoClientSecure(Client& client)
        : BlynkArduinoClientGen<Client>(client)
    {
        this->client->allowSelfSignedCerts();
    }

    void setX509Time(time_t ts) {
        this->client->setX509Time(ts);
    }

    bool connect() {
        setX509Time(now());
        BearSSL::X509List cert(_blynk_cert);
        this->client->setTrustAnchors(&cert);

        if (BlynkArduinoClientGen<Client>::connect()) {
            return this->client->connected();
        }
        return false;
    }
};

#endif


template <typename Transport>
class BlynkWifi
    : public BlynkProtocol<Transport>
{
    typedef BlynkProtocol<Transport> Base;
public:

    BlynkWifi(Transport& transp)
        : Base(transp),
          backoff(0),
          backoff_step(3),
          backoff_limit(15),
          timeout(BLYNK_TIMEOUT_MS),
          last_attempt(0)
    {}

    char* getToken() { return _token; }
    char* getHost() { return _host; }
    uint16_t getPort() { return _port; }

    void config(const char* token, const char* host = BLYNK_HOST, uint16_t port = BLYNK_PORT) {
        Base::begin(_token);
        this->conn.begin(_host, _port);
    }

    void resetConfig() {
        free(_token);
        free(_host);
        _port = 0;
    }

    void storeConfig(const String& token, const String& host, const uint16_t port) {
        resetConfig();

        _token = strdup(token.c_str());
        _host = strdup(host.c_str());
        _port = port;

        config(_token, _host, _port);
    }

    void setTimeout(uint32_t time) {
        timeout = time;
    }

    bool reachedBackoffLimit() {
        return (backoff >= backoff_limit);
    }

    void resetBackoff() {
        backoff = 0;
        last_attempt = 0;
    }

    bool connectWithBackoff() {
        if (!last_attempt) last_attempt = millis();

        uint32_t current_timeout = (0 == backoff)
            ? this->timeout
            : (this->timeout * backoff);

        if ((millis() - last_attempt) < timeout) {
            return false;
        }

        last_attempt = millis();
        if (!Base::connect(current_timeout)) {
            if (!reachedBackoffLimit()) backoff += backoff_step;
            return false;
        }

        resetBackoff();

        return true;
    }

private:
    char* _token;
    char* _host;
    uint16_t _port;

    uint8_t backoff;
    uint8_t backoff_step;
    uint8_t backoff_limit;

    uint32_t timeout;
    uint32_t last_attempt;

};


// TODO construct only when enabled?
#if BLYNK_SECURE_CLIENT
BearSSL::WiFiClientSecure _blynkWifiClient;
BlynkArduinoClientSecure<BearSSL::WiFiClientSecure> _blynkTransport(_blynkWifiClient);
BlynkWifi<BlynkArduinoClientSecure<BearSSL::WiFiClientSecure>> Blynk(_blynkTransport);
#else
WiFiClient _blynkWifiClient;
BlynkArduinoClient _blynkTransport(_blynkWifiClient);
BlynkWifi Blynk(_blynkTransport);
#endif


//
// Module state & methods
//


bool _blynk_enabled = false;

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
    _blynk_enabled = getSetting("blnkEnabled", BLYNK_ENABLED).toInt() == 1;

    if (!_blynk_enabled) {
        Blynk.disconnect();
        return;
    }

    const String token = getSetting("blnkToken", BLYNK_AUTH_TOKEN);
    const String host = getSetting("blnkHost", BLYNK_HOST);
    uint16_t port = strtoul(getSetting("blnkPort", BLYNK_PORT).c_str(), nullptr, 10);

    if (!token.length() || !host.length() || !port) {
        return;
    }

    // ref: blynk-library/src/Blynk/BlynkConfig.h
    // use default settings from Blynk itself to set proper port
    if (host.equals(F(BLYNK_DEFAULT_DOMAIN))) {
        if (BLYNK_SECURE_CLIENT) {
            port = BLYNK_DEFAULT_PORT_SSL;
        } else {
            port = BLYNK_DEFAULT_PORT;
        }
    }

    Blynk.storeConfig(token, host, port);
    Blynk.resetBackoff();
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

// TODO support Blynk.setProperty, like .(pin, "label", text)
// TODO support input widgets (terminal, numeric, text, time)

// Generic writer function called when Blynk app writes to vpin
BLYNK_WRITE_DEFAULT() {
    //DEBUG_MSG_P(PSTR("[BLYNK] Received VPin #%u param \"%s\"\n"), request.pin, param.asStr());

    // For now, only handling relay state through 0 or 1
    if (param.getLength() != 1) return;

    const bool value = param.asInt() == 1;

    uint8_t relayID = 0;
    bool assigned = _blynkVPinRelay(request.pin, &relayID);

    if (assigned) {
        relayStatus(relayID, value);
    }
}


//
// Module Setup
//

void blynkLoop(){
    if (!_blynk_enabled) return;
    if (!wifiConnected()) return;

    if (!Blynk.connected()) {
        Blynk.connectWithBackoff();
    } else {
        Blynk.run();
    }
}

void blynkSetup(){

  #if BLYNK_SECURE_CLIENT
      DEBUG_MSG_P(PSTR("[BLYNK] Secure WiFiClient\n"));
  #else
      DEBUG_MSG_P(PSTR("[BLYNK] Basic WiFiClient\n"));
  #endif

  _blynkConfigure();

  #if WEB_SUPPORT
      wsOnSendRegister(_blnkWebSocketOnSend);
      wsOnReceiveRegister(_blnkWebSocketOnReceive);
  #endif

  espurnaRegisterLoop(blynkLoop);
  espurnaRegisterReload(_blynkConfigure);

}

#endif
