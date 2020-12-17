/*
GARLAND MODULE
Copyright (C) 2020 by Dmitry Blinov <dblinov76 at gmail dot com>

Tested on 60 led strip. 
!!! For more leds can cause WDT rebot. Need to be carefully tested for more than 60 leds !!!
The most time consuming operation is actually showing leds by Adafruit Neopixel. It take about 1870 mcs.
More long strip can take more time to show. 
Currently animation calculation, brightness calculation/transition and showing makes in one loop cycle.
Debug output shows timings. Overal timing should be not more that 3000 ms.

For longer strips have sense to separate show operation from animation/brightness calculation. Or even
divide showing for parts and process them in different cycles.
*/

#include "garland.h"

#if GARLAND_SUPPORT

#include <Adafruit_NeoPixel.h>

#include <vector>

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

constexpr int LEDS                      = GARLAND_LEDS;
constexpr int PIN                       = GARLAND_D_PIN; // WS2812 pin number

#define EFFECT_UPDATE_INTERVAL_MIN      5000  // 5 sec
#define EFFECT_UPDATE_INTERVAL_MAX      10000 // 10 sec

bool _garland_enabled                   = true;
unsigned long _last_update              = 0;
unsigned long _interval_effect_update;

int paletteInd;
int animInd                             = 0; 

// Palette should
Palette pals[] = {
    // palettes below are taken from http://www.color-hex.com/color-palettes/ (and modified)
    // RGB: Red,Green,Blue sequence
    Palette("RGB", {0xFF0000, 0x00FF00, 0x0000FF}),

    // Rainbow: Rainbow colors
    Palette("Rainbow", {0xFF0000, 0xAB5500, 0xABAB00, 0x00FF00, 0x00AB55, 0x0000FF, 0x5500AB, 0xAB0055}),

    // RainbowStripe: Rainbow colors with alternating stripes of black
    Palette("Stripe", {0xFF0000, 0x000000, 0xAB5500, 0x000000, 0xABAB00, 0x000000, 0x00FF00, 0x000000,
                       0x00AB55, 0x000000, 0x0000FF, 0x000000, 0x5500AB, 0x000000, 0xAB0055, 0x000000}),

    // Party: Blue purple ping red orange yellow (and back). Basically, everything but the greens.
    // This palette is good for lighting at a club or party.
    Palette("Party", {0x5500AB, 0x84007C, 0xB5004B, 0xE5001B, 0xE81700, 0xB84700, 0xAB7700, 0xABAB00,
                      0xAB5500, 0xDD2200, 0xF2000E, 0xC2003E, 0x8F0071, 0x5F00A1, 0x2F00D0, 0x0007F9}),

    // Heat: Approximate "black body radiation" palette, akin to the FastLED 'HeatColor' function.
    // Recommend that you use values 0-240 rather than the usual 0-255, as the last 15 colors will be
    // 'wrapping around' from the hot end to the cold end, which looks wrong.
    Palette("Heat", {0x700070, 0xFF0000, 0xFFFF00, 0xFFFFCC}),

    // Fire:
    Palette("Fire", {0x000000, 0x220000, 0x880000, 0xFF0000, 0xFF6600, 0xFFCC00}),

    // Blue:
    Palette("Blue", {0xffffff, 0x0000ff, 0x00ffff}),

    // Sun: Slice Of The Sun
    Palette("Sun", {0xfff95b, 0xffe048, 0xffc635, 0xffad22, 0xff930f}),

    // Lime: yellow green mix
    Palette("Lime", {0x51f000, 0x6fff00, 0x96ff00, 0xc9ff00, 0xf0ff00}),

    // Pastel: Pastel Fruity Mixture
    Palette("Pastel", {0x75aa68, 0x5960ae, 0xe4be6c, 0xca5959, 0x8366ac}),

    // Green: Vibrant greens
    Palette("Green", {0x89ff01, 0x42c501, 0x349404, 0x0f6902, 0x004208})};

constexpr size_t palsSize() { return sizeof(pals)/sizeof(pals[0]); }

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LEDS, PIN, NEO_GRB + NEO_KHZ800);
Scene scene(&pixels);

Anim* anims[] = {new AnimStart(), new AnimPixieDust(), new AnimSparkr(), new AnimRun(), new AnimStars(),
               new AnimSpread(), new AnimRandCyc(), new AnimFly(), new AnimComets(), new AnimAssemble()};

constexpr size_t animsSize() { return sizeof(anims)/sizeof(anims[0]); }

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
            _garland_enabled = (1 == data["status"].as<int>());
            if (!_garland_enabled) {
                pixels.clear();
                pixels.show();
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

    scene.run();

    if (millis() - _last_update > _interval_effect_update && scene.finishedAnimCycle()) {
        _last_update = millis();
        _interval_effect_update = secureRandom(EFFECT_UPDATE_INTERVAL_MIN, EFFECT_UPDATE_INTERVAL_MAX);

        int prevAnimInd = animInd;
        while (prevAnimInd == animInd) animInd = secureRandom(1, animsSize());

        int prevPalInd = paletteInd;
        while (prevPalInd == paletteInd) paletteInd = secureRandom(palsSize());

        DEBUG_MSG_P(PSTR("[GARLAND] Anim: %-10s Pal: %-8s timings: calc: %4d pixl: %4d show: %4d\n"),
                    anims[prevAnimInd]->getName().c_str(), pals[prevPalInd].name(),
                    scene.getAvgCalcTime(), scene.getAvgPixlTime(), scene.getAvgShowTime());
        DEBUG_MSG_P(PSTR("[GARLAND] Anim: %-10s Pal: %-8s Inter: %d\n"),
                    anims[animInd]->getName().c_str(), pals[paletteInd].name(), _interval_effect_update);

        scene.setAnim(anims[animInd]);
        scene.setPalette(&pals[paletteInd]);
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
    paletteInd = secureRandom(palsSize());
    scene.setAnim(anims[0]);
    scene.setPalette(&pals[0]);
    scene.setup();

    _interval_effect_update = secureRandom(EFFECT_UPDATE_INTERVAL_MIN, EFFECT_UPDATE_INTERVAL_MAX);
}

#endif  // GARLAND_SUPPORT
