/*

GARLAND MODULE

Copyright (C) 2017 by Dmitry Blinov <dblinov76 at gmail dot com>

*/

#if GARLAND_SUPPORT

#include <Adafruit_NeoPixel.h>

bool _garland_enabled = true;

const char* NAME_GARLAND_ENABLED     = "garlandEnabled";

#define PIN 4 // WS2812 pin number
#define LEDS 60 // number of LEDs in the strip. Not sure why, but 100 leds don't work with software serial! Works with hardware serial though

//Adafruit's class to operate strip
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LEDS, PIN, NEO_GRB + NEO_KHZ800); 

int curpixel = 0;
int prevpixel = 0;
int inc = 1;
uint8_t r,g,b;

//------------------------------------------------------------------------------
void garlandEnabled(bool enabled) {
    _garland_enabled = enabled;
}

//------------------------------------------------------------------------------
bool garlandEnabled() {
    return _garland_enabled;
}

//------------------------------------------------------------------------------
std::vector<garland_callback_f> _garland_callbacks;

void garlandRegister(garland_callback_f callback) {
    _garland_callbacks.push_back(callback);
}

//------------------------------------------------------------------------------
// Setup
//------------------------------------------------------------------------------
void commonSetup() {
    DEBUG_MSG_P(PSTR("commonSetup\n"));
    pixels.begin();
    curpixel = 0;
    r = 255;
    g = b = 0;
    DEBUG_MSG_P(PSTR("~commonSetup\n"));
}

//------------------------------------------------------------------------------
void garlandConfigure() {
  commonSetup();
}

//------------------------------------------------------------------------------
void _garlandReload() {
}

#if WEB_SUPPORT
//------------------------------------------------------------------------------
void _garlandWebSocketOnConnected(JsonObject& root) {
  root["garlandEnabled"] = garlandEnabled();
  root["garlandVisible"] = 1;
}

//------------------------------------------------------------------------------
bool _garlandWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return false;
}

//------------------------------------------------------------------------------
void _garlandWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
}
#endif

//------------------------------------------------------------------------------
void garlandSetup() {
  DEBUG_MSG_P(PSTR("garlandSetup\n"));
  garlandConfigure();


  // Websockets
  #if WEB_SUPPORT
      wsRegister()
          .onConnected(_garlandWebSocketOnConnected)
          .onKeyCheck(_garlandWebSocketOnKeyCheck)
          .onAction(_garlandWebSocketOnAction);
  #endif

  espurnaRegisterLoop(garlandLoop);
  espurnaRegisterReload(_garlandReload);
}

//------------------------------------------------------------------------------
// Loop
//------------------------------------------------------------------------------
void garlandLoop(void) {

  if (!garlandEnabled())
    return;

  prevpixel = curpixel;
  curpixel += inc;
  if (curpixel >= LEDS || curpixel < 0) {
    inc *= -1;
    curpixel += inc;
    r += 13;
    g += 27;
    b += 31;
    DEBUG_MSG_P(PSTR("r: %d, g: %d, b: %d\n"), r, g, b);  
  }

  pixels.setPixelColor(prevpixel, pixels.Color(0, 0, 0));
  pixels.setPixelColor(curpixel, pixels.Color(r, g, b));
  pixels.show();
}

#endif // GARLAND_SUPPORT
