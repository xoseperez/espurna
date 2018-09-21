/*

BLYNK MODULE

Copyright (C) 2018 by Thomas Häger <thaeger at hdsnetz dot de>

*/

#if BLYNK_SUPPORT
#include <ESP8266WiFi.h>
#include <BlynkSimpleStream.h>

bool _blnk_enabled = false;
WiFiClient _wific;


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
        relays.add(blynkVpin(i));
    }

    #if SENSOR_SUPPORT
        JsonArray& list = root.createNestedArray("blnkMagnitudes");
        for (byte i=0; i<magnitudeCount(); i++) {
            JsonObject& element = list.createNestedObject();
            element["name"] = magnitudeName(i);
            element["type"] = magnitudeType(i);
            element["index"] = magnitudeIndex(i);
            element["Vpin"] = getSetting("blnkMagnitude", i, 0).toInt();
        }
    #endif
}

int blynkVpin(unsigned char relayID) {
    char buffer[17];
    snprintf_P(buffer, sizeof(buffer), PSTR("blnkRelayVpin%u"), relayID);
    DEBUG_MSG_P(PSTR("[BLYNK] Buffer: %s\n"),buffer);

    return getSetting(buffer).toInt();
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

BLYNK_WRITE_DEFAULT(){
  int pin = request.pin;
  if (pin==1)
    DEBUG_MSG_P(PSTR("[BLYNK] Received: %s from Pin: %u\n"),param,pin);
}

void blynkSetup(){
  _blnkConfigure();

  #if WEB_SUPPORT
      wsOnSendRegister(_blnkWebSocketOnSend);
      wsOnReceiveRegister(_blnkWebSocketOnReceive);
  #endif

  // Main callbacks
  espurnaRegisterLoop(_blnkLoop);
  espurnaRegisterReload(_blnkConfigure);
}

void _wifi_connect() {

  if (_wific.connect(&getSetting("blnkHost", BLYNK_HOST)[0], getSetting("blnkPort", BLYNK_PORT).toInt()))
    DEBUG_MSG_P(PSTR("[BLYNK] WiFi Client connected\n"));
  else
    DEBUG_MSG_P(PSTR("[BLYNK] WiFi could not connect to %s:%s.\n"),&getSetting("blnkHost", BLYNK_HOST)[0],&getSetting("blnkHost", BLYNK_PORT)[0]);
}

void _blnkLoop(){

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
