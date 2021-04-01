/*
Part of the GARLAND MODULE
Copyright (C) 2020 by Dmitry Blinov <dblinov76 at gmail dot com>

Inspired by https://github.com/Vasil-Pahomov/ArWs2812 (currently https://github.com/Vasil-Pahomov/Liana)
*/

#pragma once

#if GARLAND_SUPPORT

#include <array>
#include <vector>

#include "anim.h"
#include "animations/anim_assemble.h"
#include "animations/anim_comets.h"
#include "animations/anim_dolphins.h"
#include "animations/anim_fountain.h"
#include "animations/anim_fly.h"
#include "animations/anim_glow.h"
#include "animations/anim_pixiedust.h"
#include "animations/anim_randcyc.h"
#include "animations/anim_run.h"
#include "animations/anim_salut.h"
#include "animations/anim_sparkr.h"
#include "animations/anim_spread.h"
#include "animations/anim_stars.h"
#include "animations/anim_start.h"
#include "animations/anim_waves.h"

class Adafruit_NeoPixel;
class Palette;

class Scene {
public:
    Scene(Adafruit_NeoPixel* pixels);


    void setPalette(Palette* palette);
    void setBrightness(byte brightness);
    byte getBrightness();
    void setSpeed(byte speed);
    byte getSpeed();
    void setDefault();
    void setAnim(Anim* anim) { _anim = anim; }
    void run();
    void setup();
    bool finishedAnimCycle() { return _anim ? _anim->finishedycle() : true; }
    unsigned long getAvgCalcTime();
    unsigned long getAvgPixlTime();
    unsigned long getAvgShowTime();
    int getNumShows() { return numShows; }

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
    unsigned long      transms;

    byte               brightness = 0;

    // cycleFactor is actually number of cycles to calculate and draw one animation step
    // if cycleFactor is 2 or more, than calculation and drawing made in different cycles
    // cycleFactor is float. For example cycleFactor=2.5 gives one step 2 than next 3 cycles per anim step
    // Recommended values: 1 < cycleFactor < 4
    float              cycleFactor = 2.0;
    // speed is reverse to cycleFactor. For forward direction control of animation speed.
    // Recommended values: 30 < speed < 60.
    // Correspondence: 
    //   speed=60, cycleFactor=1
    //   speed=30, cycleFactor=4
    byte               speed = 50;
    float              cycleTail = 0;
    int                cyclesRemain = 0;

    enum State {
        Calculate,
        Transition,
        Show
    }                  state = Calculate;

    int                numShows = 0;

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

#endif  // GARLAND_SUPPORT
