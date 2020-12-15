#pragma once

#if GARLAND_SUPPORT

#include <vector>

#include "palette.h"

#define BRA_AMP_SHIFT          1    // brigthness animation amplitude shift. true BrA amplitude is calculated 
                                    // as (0..127) value shifted right by this amount
#define DEFAULT_BRIGHTNESS    12    // brightness adjustment, up to 255
#define TRANSITION_MS       1000    // transition time between animations, ms
#define BRA_OFFSET           127    //(222-64) // brigthness animation amplitude offset
#define SPARK_PROB             3    //probability of spark when in idle plase

class Adafruit_NeoPixel;

class Scene {
public:
    Scene(Adafruit_NeoPixel* pixels);

    class Anim {
    public:
        Anim(String name);
        String getName() { return _name; }
        void Setup(Palette* palette, uint16_t numLeds, Color* leds, Color* _ledstmp, byte* seq);
        virtual void Run() = 0;

    protected:
        uint16_t    numLeds     = 0;
        Palette*    palette     = nullptr;
        Color*      leds        = nullptr;
        Color*      ledstmp     = nullptr;
        byte*       seq         = nullptr;

        int         phase;
        int         pos;
        int         inc;

        //brigthness animation (BrA) current initial phase
        byte        braPhase;
        //braPhase change speed
        byte        braPhaseSpd  = 5;
        //BrA frequency (spatial)
        byte        braFreq      = 150;

        Color       curColor     = Color(0);
        Color       prevColor    = Color(0);
        const Color sparkleColor = Color(0xFFFFFF);

        virtual void SetupImpl() = 0;

        //glow animation setup
        void glowSetUp();

        //glow animation - must be called for each LED after it's BASIC color is set
        //note this overwrites the LED color, so the glow assumes that color will be stored elsewhere (not in leds[])
        //or computed each time regardless previous leds[] value
        void glowForEachLed(int i);

        //glow animation - must be called at the end of each animaton run
        void glowRun();

    private:
        const String _name;
    };

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
