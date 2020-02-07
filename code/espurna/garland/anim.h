#ifndef anim_h
#define anim_h
#include <Adafruit_NeoPixel.h>
#include "palette.h"
#include "..\config\general.h"

#define PIN 4 // WS2812 pin number
const int   LEDS                     = GARLAND_LEDS;
#define DEFAULT_BRIGHTNESS 12//256// brightness adjustment, up to 256

#define TRANSITION_MS 1000 // transition time between animations, ms

// brigthness animation amplitude shift. true BrA amplitude is calculated as (0..127) value shifted right by this amount
#define BRA_AMP_SHIFT 1
// brigthness animation amplitude offset
#define BRA_OFFSET 127//(222-64)

//probability of spark when in idle plase
#define SPARK_PROB 3

class Anim {
    
private:
    //Color arrays - two for making transition
    static Color leds1[LEDS];
    static Color leds2[LEDS];
    //auxiliary colors array
    static Color ledstmp[LEDS];

    void animStart();
    
    // length of animation timeslot (period)
    byte period;
    // array of Color to work with
    Color *leds;
    Palette *palette;

    // millis for next timeslot 
    unsigned long nextms;
    // millis to transition end
    unsigned long transms;

    int phase;
    int pos;
    int inc;

    byte brightness = DEFAULT_BRIGHTNESS;

    //whether to call SetUp on palette change
    //(some animations require full transition with fade, otherwise the colors would change in a step, some not)
    bool setUpOnPalChange;

    Color curColor = Color(0);
    Color prevColor = Color(0);

    Color sparkleColor = Color(0xFFFFFF);

    static byte seq[LEDS];

    //brigthness animation (BrA) current initial phase
    byte braPhase;
    //braPhase change speed 
    byte braPhaseSpd=5;
    //BrA frequency (spatial)
    byte braFreq=150;

    unsigned long start_time = 0;
    unsigned long sum_calc_time = 0;
    unsigned long sum_show_time = 0;
    unsigned int sum_num = 0;
    unsigned int calc_num = 0;
    unsigned int show_num = 0;

    //glow animation setup
    void glowSetUp();

    //glow animation - must be called for each LED after it's BASIC color is set
    //note this overwrites the LED color, so the glow assumes that color will be stored elsewhere (not in leds[])
    //or computed each time regardless previous leds[] value
    void glowForEachLed(int i);
    
    //glow animation - must be called at the end of each animaton run
    void glowRun();

    void setUp();

    //run and setup handlers
    void (Anim::*runImpl)();
    void (Anim::*setUpImpl)();


    //animation implementations
    void animStart_SetUp();
    void animStart_Run();

    void animRun_SetUp();
    void animRun_Run();
    
    void animPixieDust_SetUp();
    void animPixieDust_Run();
    
    void animSparkr_SetUp();
    void animSparkr_Run();

    void animRandCyc_SetUp();
    void animRandCyc_Run();

    void animStars_SetUp();
    void animStars_Run();

    void animSpread_SetUp();
    void animSpread_Run();

    void animFly_SetUp();
    void animFly_Run();

    // void animBT_SetUp();
    // void animBT_Run();

public:
    Anim();
    void setPeriod(byte period);
    void setPalette(Palette * pal);
    void setBrightness(byte brightness);
    byte getBrightness();
    void setAnim(byte animInd);
    bool run();//returns true if actual change has completed, or false if it's dummy call (previous call was too recent in time)
    void doSetUp();
    unsigned long getAvgCalcTime();
    unsigned long getAvgShowTime();

};

unsigned int rng();

byte rngb();

#endif
