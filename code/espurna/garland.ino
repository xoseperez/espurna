/*

GARLAND MODULE

Copyright (C) 2020 by Dmitry Blinov <dblinov76 at gmail dot com>

*/

#include "config\general.h"
#if GARLAND_SUPPORT

#include "garland\anim.h"
#include "garland\color.h"
#include "garland\palette.h"
#include "garland\small_timer.hpp"
#include <Adafruit_NeoPixel.h>

const char* NAME_GARLAND_ENABLED     = "garlandEnabled";

#define ANIMS                          1 //number of animations
#define PALS                           7 //number of palettes
#define INTERVAL                   10000 //change interval, msec

bool _garland_enabled                = true;

int paletteInd;
int animInd = 1;
extern Adafruit_NeoPixel pixels;

Palette * pals[PALS] = {&PalRgb, &PalRainbow, &PalRainbowStripe, &PalParty, &PalHeat, &PalFire, &PalIceBlue};

Anim anim = Anim();

csTimerDef <INTERVAL> EffectAutoChangeTimer; // auto change animation effect, interval timer

constexpr bool disableAutoChangeEffects = false;

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
  anim.setPalette(pals[0]);
  anim.doSetUp();
  EffectAutoChangeTimer.start(10000); // 10000 for "release" AnimStart
}

//------------------------------------------------------------------------------
// Loop
//------------------------------------------------------------------------------
void garlandLoop(void) {

  if (!garlandEnabled())
    return;


  if (anim.run()) {
  }

  bool needChange = EffectAutoChangeTimer.run();
    // auto change effect
  if (needChange && ! disableAutoChangeEffects) {
    EffectAutoChangeTimer.start();

    switch ( (animInd < 0) ? 0 : random(2)) {
      case 0: {
        int prevAnimInd = animInd;
        while (prevAnimInd == animInd) animInd = random(ANIMS);
        anim.setAnim(animInd);
        anim.setPeriod(random(20, 40));
        anim.setPalette(pals[paletteInd]);
        anim.doSetUp();
        break;
      }
      case 1: {
        int prevPalInd = paletteInd;
        while (prevPalInd == paletteInd) paletteInd = random(PALS);
        anim.setPalette(pals[paletteInd]);
        break;
      }
    }
  }
}

#endif // GARLAND_SUPPORT
