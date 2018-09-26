/*

BLYNK MODULE

Copyright (C) 2018 by Thomas Häger <thaeger at hdsnetz dot de>

*/

#if BLYNK_SUPPORT
#include <ESP8266WiFi.h>
#include <BlynkSimpleStream.h>

bool _blnk_enabled = false;
WiFiClient _wific;

unsigned char _blynkRelay(unsigned int vpin) {
    for (unsigned char relayID=0; relayID<relayCount(); relayID++) {
        if (getSetting("blnkRelayVpin", relayID, 0).toInt() == vpin) {
            return relayID;
        }
    }
    return -1;
}

void _blnkConfigure() {
    _blnk_enabled = getSetting("blnkEnabled", BLYNK_ENABLED).toInt() == 1;
    if (_blnk_enabled && (getSetting("blnkHost", BLYNK_HOST).length() == 0)) {
        _blnk_enabled = false;
        setSetting("blnkEnabled", 0);
    }
    if(!_blnk_enabled) {
      Blynk.disconnect();
      _wific.stop();
      return;
    }
}

void _wifi_connect() {

  if (_wific.connect(&getSetting("blnkHost", BLYNK_HOST)[0], getSetting("blnkPort", BLYNK_PORT).toInt()))
    DEBUG_MSG_P(PSTR("[BLYNK] WiFi Client connected\n"));
  else
    DEBUG_MSG_P(PSTR("[BLYNK] WiFi could not connect to %s:%s.\n"),&getSetting("blnkHost", BLYNK_HOST)[0],&getSetting("blnkHost", BLYNK_PORT)[0]);
}

BLYNK_WRITE_DEFAULT(){
  unsigned char relayID = _blynkRelay(request.pin);
  if (relayID >= 0) {
      int value = param[0].asInt();
      DEBUG_MSG_P(PSTR("[BLYNK] Received value %d for VPIN %u\n"), value, request.pin);
      relayStatus(relayID, value == 1);
  }
}

BLYNK_CONNECTED(){
  DEBUG_MSG_P(PSTR("[BLYNK] Connected to Blynk server\n"));
  blynkSendRelays();
}

#if WEB_SUPPORT

bool _blnkWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "blnk", 4) == 0);
}

void _blnkWebSocketOnSend(JsonObject& root) {
    root["blnkVisible"] = 1;
    root["blnkEnabled"] = getSetting("blnkEnabled", BLYNK_ENABLED).toInt() == 1;
    root["blnkAuthKey"] = getSetting("blnkAuthKey", BLYNK_AUTH_TOKEN);
    root["blnkHost"] = getSetting("blnkHost", BLYNK_HOST);
    root["blnkPort"] = getSetting("blnkPort", BLYNK_PORT).toInt();

    JsonArray& relays = root.createNestedArray("blnkRelays");
    for (unsigned char i=0; i<relayCount(); i++) {
        relays.add(getSetting("blnkRelayVpin", i, -1).toInt());
    }

    #if SENSOR_SUPPORT
        JsonArray& list = root.createNestedArray("blnkMagnitudes");
        for (byte i=0; i<magnitudeCount(); i++) {
            JsonObject& element = list.createNestedObject();
            element["name"] = magnitudeName(i);
            element["type"] = magnitudeType(i);
            element["index"] = magnitudeIndex(i);
            element["idx"] = getSetting("blnkMagnitude", i, -1).toInt();
        }
    #endif
}
#endif //WEB_SUPPORT

void blynkSendRelays() {
    for (uint8_t relayID=0; relayID < relayCount(); relayID++) {
        blynkSendRelay(relayID, relayStatus(relayID) ? 1 : 0);
    }
}

void blynkSendMeasurement(unsigned char key, char * payload) {
    if (!_blnk_enabled) return;
    int  vpin = getSetting("blnkMagnitude", key, -1).toInt();
    if(vpin>-1) {
      //DEBUG_MSG_P(PSTR("[BLYNK] Send sensor data to VPIN %u Data: %s\n"), vpin, payload);
      Blynk.virtualWrite(vpin,payload);
    }
}

void blynkSendRelay(unsigned char key, unsigned char status) {
    if (!_blnk_enabled) return;
    DEBUG_MSG_P(PSTR("[BLYNK] Send new status %u to Relay %u\n"),status,key);
    unsigned int  vpin = getSetting("blnkRelayVpin", key, 0).toInt();
    Blynk.virtualWrite(vpin,status);
}

int blynkVpin(unsigned char relayID) {
    char buffer[17];
    snprintf_P(buffer, sizeof(buffer), PSTR("blnkRelayVpin%u"), relayID);
    return getSetting(buffer).toInt();
}



void blynkSetup(){

  _blnkConfigure();

  #if WEB_SUPPORT
      wsOnSendRegister(_blnkWebSocketOnSend);
      wsOnReceiveRegister(_blnkWebSocketOnReceive);
  #endif

  espurnaRegisterLoop(blnkLoop);
  espurnaRegisterReload(_blnkConfigure);

}


void blnkLoop(){

  if(!_blnk_enabled) return;

  if (!wifiConnected() || (WiFi.getMode() != WIFI_STA)) return;

  if(!_wific.connected()) _wifi_connect();

  if (!Blynk.connected()) {
    char auth[50];
    getSetting("blnkAuthKey", BLYNK_AUTH_TOKEN).toCharArray(auth,50);
    Blynk.config(_wific, auth);
    if (!Blynk.connect(3000))
      DEBUG_MSG_P(PSTR("[BLYNK] Could not connect to Bylnk Server. Token[%s]\n"),auth);
  }

  Blynk.run();

}

#endif
