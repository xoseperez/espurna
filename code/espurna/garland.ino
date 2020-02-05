/*

GARLAND MODULE

Copyright (C) 2020 by Dmitry Blinov <dblinov76 at gmail dot com>

*/

#include "config\general.h"
#if GARLAND_SUPPORT

#include "garland\anim.h"
#include "garland\color.h"
#include "garland\palette.h"
#include <Adafruit_NeoPixel.h>

const char* NAME_GARLAND_ENABLED        = "garlandEnabled";

#define ANIMS                             7 //number of animations
#define PALS                              7 //number of palettes
#define EFFECT_UPDATE_INTERVAL_MIN     5000 // 5 sec
#define EFFECT_UPDATE_INTERVAL_MAX    10000 // 5 sec

bool _garland_enabled                   = true;
unsigned long _last_update              = 0;
unsigned long _interval_effect_update;

int paletteInd;
int animInd                             = -1; 
extern Adafruit_NeoPixel pixels;

Palette * pals[PALS] = {&PalRgb, &PalRainbow, &PalRainbowStripe, &PalParty, &PalHeat, &PalFire, &PalIceBlue};

Anim anim = Anim();


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
  anim.setPeriod(6);
  anim.setPalette(pals[0]);
  anim.doSetUp();

  _interval_effect_update = random(EFFECT_UPDATE_INTERVAL_MIN, EFFECT_UPDATE_INTERVAL_MAX);
}

//------------------------------------------------------------------------------
// Loop
//------------------------------------------------------------------------------
void garlandLoop(void) {

  if (!garlandEnabled())
    return;


  if (anim.run()) {
  }

  if (millis() - _last_update > _interval_effect_update) {
    _last_update = millis();
    _interval_effect_update = random(EFFECT_UPDATE_INTERVAL_MIN, EFFECT_UPDATE_INTERVAL_MAX);
  
    int prevAnimInd = animInd;
    while (prevAnimInd == animInd) animInd = random(ANIMS);

    // animInd++;
    // if (animInd == ANIMS)
    //  animInd = 0;

    anim.setAnim(animInd);

    byte period = random(5, 30);
    anim.setPeriod(period);

    int prevPalInd = paletteInd;
    while (prevPalInd == paletteInd) paletteInd = random(PALS);    
    anim.setPalette(pals[paletteInd]);

    DEBUG_MSG_P(PSTR("Animation: %d, Palette: %d, Period: %d, interval: %d, avg_calc_time: %d, avg_show_time: %d\n"), animInd, paletteInd, period, _interval_effect_update, anim.getAvgCalcTime(), anim.getAvgShowTime());
    anim.doSetUp();
  }
}

#endif // GARLAND_SUPPORT
