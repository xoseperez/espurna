#pragma once

#if GARLAND_SUPPORT

#include <array>
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

    std::array<byte, 256> bri_lvl = {{0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
                                      4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 9, 9, 9, 9, 10,
                                      10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 17, 17,
                                      17, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 22, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29,
                                      29, 30, 30, 31, 31, 32, 32, 33, 34, 34, 35, 35, 36, 37, 37, 38, 39, 39, 40, 41, 42, 42, 43, 44, 45, 45, 46,
                                      47, 48, 49, 49, 50, 51, 52, 53, 54, 55, 56, 57, 57, 58, 59, 60, 61, 63, 64, 65, 66, 67, 68, 69, 70, 71, 73,
                                      74, 75, 76, 78, 79, 80, 82, 83, 84, 86, 87, 89, 90, 91, 93, 94, 96, 98, 99, 101, 102, 104, 106, 108, 109,
                                      111, 113, 115, 117, 119, 121, 122, 124, 126, 129, 131, 133, 135, 137, 139, 142, 144, 146, 149, 151, 153, 156,
                                      158, 161, 163, 166, 169, 171, 174, 177, 180, 183, 186, 189, 192, 195, 198, 201, 204, 208, 211, 214, 218, 221,
                                      225, 228, 232, 236, 239, 243, 247, 251, 255}};

    void setupImpl();
};

unsigned int rng();
byte         rngb();

#endif  // GARLAND_SUPPORT
