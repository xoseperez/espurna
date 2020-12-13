/*

GARLAND MODULE

Copyright (C) 2020 by Dmitry Blinov <dblinov76 at gmail dot com>

*/

#include "garland.h"
#if GARLAND_SUPPORT

#include <Adafruit_NeoPixel.h>

#include "garland/anims.h"
#include "garland/color.h"
#include "garland/palette.h"
#include "garland/scene.h"
#include "web.h"
#include "ws.h"

const char* NAME_GARLAND_ENABLED        = "garlandEnabled";
const char* NAME_GARLAND_BRIGHTNESS     = "garlandBrightness";

const char* NAME_GARLAND_SWITCH         = "garland_switch";
const char* NAME_GARLAND_SET_BRIGHTNESS = "garland_set_brightness";

#define ANIMS                             8 //number of animations
#define EFFECT_UPDATE_INTERVAL_MIN     5000 // 5 sec
#define EFFECT_UPDATE_INTERVAL_MAX    10000 // 5 sec

bool _garland_enabled                   = true;
unsigned long _last_update              = 0;
unsigned long _interval_effect_update;

int paletteInd;
int animInd                             = 0; 
extern Adafruit_NeoPixel pixels;

Scene scene = Scene();

Scene::Anim* anims[] = { &anim_run, &anim_fly, &anim_pixel_dust, &anim_rand_cyc, &anim_sparkr, &anim_spread, &anim_stars, &anim_comets };

constexpr bool disableAutoChangeEffects  = false;

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
    _garland_enabled = getSetting(NAME_GARLAND_ENABLED, true);
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
    root[NAME_GARLAND_ENABLED] = garlandEnabled();
    root[NAME_GARLAND_BRIGHTNESS] = scene.getBrightness();
    root["garlandVisible"] = 1;
}

//------------------------------------------------------------------------------
bool _garlandWebSocketOnKeyCheck(const char* key, JsonVariant& value) {
    if (strncmp(key, NAME_GARLAND_ENABLED, strlen(NAME_GARLAND_ENABLED)) == 0) return true;
    if (strncmp(key, NAME_GARLAND_BRIGHTNESS, strlen(NAME_GARLAND_BRIGHTNESS)) == 0) return true;
    return false;
}

//------------------------------------------------------------------------------
void _garlandWebSocketOnAction(uint32_t client_id, const char* action, JsonObject& data) {
    if (strcmp(action, NAME_GARLAND_SWITCH) == 0) {
        if (data.containsKey("status") && data.is<int>("status")) {
            const char* payload = data["status"];
            if (strlen(payload) == 1) {
                if (payload[0] == '0') {
                    _garland_enabled = false;
                    pixels.clear();
                    pixels.show();
                } else if (payload[0] == '1')
                    _garland_enabled = true;
            }
        }
    }

    if (strcmp(action, NAME_GARLAND_SET_BRIGHTNESS) == 0) {
        if (data.containsKey("brightness")) {
            byte new_brightness = data.get<byte>("brightness");
            DEBUG_MSG_P(PSTR("[GARLAND] new brightness = %d\n"), new_brightness);
            scene.setBrightness(new_brightness);
        }
    }
}
#endif

//------------------------------------------------------------------------------
// Loop
//------------------------------------------------------------------------------
void garlandLoop(void) {
    if (!garlandEnabled())
        return;

    if (scene.run()) {
    }

    if (millis() - _last_update > _interval_effect_update) {
        _last_update = millis();
        _interval_effect_update = random(EFFECT_UPDATE_INTERVAL_MIN, EFFECT_UPDATE_INTERVAL_MAX);

        int prevAnimInd = animInd;
        while (prevAnimInd == animInd) animInd = random(ANIMS);

        scene.setAnim(anims[animInd]);

        int prevPalInd = paletteInd;
        while (prevPalInd == paletteInd) paletteInd = random(PALS);
        scene.setPalette(pals[paletteInd]);

        DEBUG_MSG_P(PSTR("[GARLAND] Anim: %-10s Pal: %d Inter: %d avg_calc: %d avg_show: %d\n"), anims[animInd]->getName().c_str(), paletteInd, _interval_effect_update, scene.getAvgCalcTime(), scene.getAvgShowTime());
        scene.setup();
    }
}

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
    randomSeed(analogRead(0) * analogRead(1));
    paletteInd = random(PALS);
    scene.setAnim(&anim_start);
    scene.setPalette(pals[0]);
    scene.setup();

    _interval_effect_update = random(EFFECT_UPDATE_INTERVAL_MIN, EFFECT_UPDATE_INTERVAL_MAX);
}

#endif  // GARLAND_SUPPORT
