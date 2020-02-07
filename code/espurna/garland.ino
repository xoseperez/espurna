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
const char* NAME_GARLAND_BRIGHTNESS     = "garlandBrightness";

const char* NAME_GARLAND_SWITCH         = "garland_switch";
const char* NAME_GARLAND_SET_BRIGHTNESS = "garland_set_brightness";

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
void garlandDisable() {
  pixels.clear();
}

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
  _garland_enabled     = getSetting(NAME_GARLAND_ENABLED, true);
  DEBUG_MSG_P(PSTR("[GARLAND] _garland_enabled = %d\n"), _garland_enabled);
}

//------------------------------------------------------------------------------
void garlandConfigure() {
  commonSetup();
}

//------------------------------------------------------------------------------
void _garlandReload() {
  commonSetup();
}

#if WEB_SUPPORT
//------------------------------------------------------------------------------
void _garlandWebSocketOnConnected(JsonObject& root) {
  root[NAME_GARLAND_ENABLED]    = garlandEnabled();
  root[NAME_GARLAND_BRIGHTNESS] = anim.getBrightness();
  root["garlandVisible"]        = 1;
}

//------------------------------------------------------------------------------
bool _garlandWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
  if (strncmp(key, NAME_GARLAND_ENABLED,   strlen(NAME_GARLAND_ENABLED))   == 0) return true;
  if (strncmp(key, NAME_GARLAND_BRIGHTNESS,   strlen(NAME_GARLAND_BRIGHTNESS))   == 0) return true;
  return false;
}

//------------------------------------------------------------------------------
void _garlandWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
  if (strcmp(action, NAME_GARLAND_SWITCH) == 0) {
    if (data.containsKey("status") && data.is<int>("status")) {
      const char* payload = data["status"];
      if (strlen(payload) == 1) {
          if (payload[0] == '0') {
            _garland_enabled = false;
            pixels.clear();
            pixels.show();
          }
          else if (payload[0] == '1') _garland_enabled = true;
      }
    }
  }

  if (strcmp(action, NAME_GARLAND_SET_BRIGHTNESS) == 0) {
    if (data.containsKey("brightness")) {
      byte new_brightness = data.get<byte>("brightness");
      DEBUG_MSG_P(PSTR("[GARLAND] new brightness = %d\n"), new_brightness);
      anim.setBrightness(new_brightness);
    }
  }
}
#endif

//------------------------------------------------------------------------------
void garlandSetup() {
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

    DEBUG_MSG_P(PSTR("[GARLAND] Anim: %d, Pal: %d, Period: %d, Inter: %d, avg_calc: %d, avg_show: %d\n"), animInd, paletteInd, period, _interval_effect_update, anim.getAvgCalcTime(), anim.getAvgShowTime());
    anim.doSetUp();
  }
}

#endif // GARLAND_SUPPORT
