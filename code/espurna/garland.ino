/*

GARLAND MODULE

Copyright (C) 2020 by Dmitry Blinov <dblinov76 at gmail dot com>

*/

#if GARLAND_SUPPORT

#include "garland\anim.h"
#include "garland\color.h"
#include "garland\palette.h"
#include <Adafruit_NeoPixel.h>

const char* NAME_GARLAND_ENABLED     = "garlandEnabled";

#define ANIMS                          1 //number of animations
#define PALS                           7 //number of palettes

bool _garland_enabled                = true;
int  _curpixel                       = 0;
int  _prevpixel                      = 0;
int  _inc                            = 1;
uint8_t r,g,b;

unsigned long  anim_start  = 0;
unsigned short anim_cycles = 0;

int paletteInd;
int animInd = 1;
extern Adafruit_NeoPixel pixels;

Palette * pals[PALS] = {&PalRgb, &PalRainbow, &PalRainbowStripe, &PalParty, &PalHeat, &PalFire, &PalIceBlue};

Anim anim = Anim();

//------------------------------------------------------------------------------
void garlandEnabled(bool enabled) {
    _garland_enabled = enabled;
}

//------------------------------------------------------------------------------
bool garlandEnabled() {
    return _garland_enabled;
}

//------------------------------------------------------------------------------
// Setup
//------------------------------------------------------------------------------
void commonSetup() {
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

  pixels.begin();
  randomSeed(analogRead(0)*analogRead(1));
  paletteInd = random(PALS);
  anim.setAnim(animInd);
  anim.setPeriod(20);
  anim.setPalette(pals[2]);
  anim.doSetUp();
  // EffectAutoChangeTimer.start(20000); // 10000 for "release" AnimStart
}

//------------------------------------------------------------------------------
// Loop
//------------------------------------------------------------------------------
void garlandLoop(void) {

  if (!garlandEnabled())
    return;

  if (anim.run()) {
  }
}

#endif // GARLAND_SUPPORT
