#pragma once

#if GARLAND_SUPPORT

#include <vector>

#include "anim.h"

#define DEFAULT_BRIGHTNESS    12    // brightness adjustment, up to 255
#define TRANSITION_MS       1000    // transition time between animations, ms

class Adafruit_NeoPixel;
class Palette;

class Scene {
public:
    Scene(Adafruit_NeoPixel* pixels);


    void setPalette(Palette* palette);
    void setBrightness(byte brightness);
    byte getBrightness();
    void setAnim(Anim* anim) { _anim = anim; }
    bool run();  //returns true if actual change has completed, or false if it's dummy call (previous call was too recent in time)
    void setup();
    unsigned long getAvgCalcTime();
    unsigned long getAvgPixlTime();
    unsigned long getAvgShowTime();

private:
    Adafruit_NeoPixel* _pixels = nullptr;
    uint16_t           _numLeds;
    //Color arrays - two for making transition
    std::vector<Color> _leds1;
    std::vector<Color> _leds2;
    // array of Colorfor anim to currently work with
    Color*             _leds = nullptr;
    Anim*              _anim = nullptr;

    //auxiliary colors array for mutual usage of anims
    std::vector<Color> _ledstmp;
    std::vector<byte>  _seq;

    Palette*           _palette = nullptr;

    // millis to transition end
    unsigned long transms;

    byte brightness = DEFAULT_BRIGHTNESS;

    //whether to call SetUp on palette change
    //(some animations require full transition with fade, otherwise the colors would change in a step, some not)
    bool setUpOnPalChange = true;

    unsigned long sum_calc_time = 0;
    unsigned long sum_pixl_time = 0;
    unsigned long sum_show_time = 0;
    unsigned int calc_num = 0;
    unsigned int show_num = 0;
    unsigned int pixl_num = 0;

    void setupImpl();
};

unsigned int rng();
byte         rngb();

#endif  // GARLAND_SUPPORT
