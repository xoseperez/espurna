#ifndef anim_h
#define anim_h
#include <Adafruit_NeoPixel.h>
#include "palette.h"
#include "..\config\general.h"

const int   PIN  = GARLAND_D_PIN; // WS2812 pin number
const int   LEDS = GARLAND_LEDS;
#define DEFAULT_BRIGHTNESS 12//256// brightness adjustment, up to 256

#define TRANSITION_MS 1000 // transition time between animations, ms

// brigthness animation amplitude shift. true BrA amplitude is calculated as (0..127) value shifted right by this amount
#define BRA_AMP_SHIFT 1
// brigthness animation amplitude offset
#define BRA_OFFSET 127//(222-64)

//probability of spark when in idle plase
#define SPARK_PROB 3

class Scene {
public:
    Scene();

    class Anim {
    public:
        String getName();
        Anim(String name);
        void Setup(int paletteInd , Color* leds);
        virtual void Run() = 0;

    protected:
        int phase;
        int pos;
        int inc;
        Palette*        _palette;
        Color*          _leds;

        //brigthness animation (BrA) current initial phase
        byte braPhase;
        //braPhase change speed 
        byte braPhaseSpd=5;
        //BrA frequency (spatial)
        byte braFreq=150;

        static byte seq[LEDS];

        Color curColor = Color(0);
        Color prevColor = Color(0);
        const Color sparkleColor = Color(0xFFFFFF);
        
        //auxiliary colors array
        static Color ledstmp[LEDS];
        
        virtual void SetupImpl(){};

        //glow animation setup
        void glowSetUp();

        //glow animation - must be called for each LED after it's BASIC color is set
        //note this overwrites the LED color, so the glow assumes that color will be stored elsewhere (not in leds[])
        //or computed each time regardless previous leds[] value
        void glowForEachLed(int i);
        
        //glow animation - must be called at the end of each animaton run
        void glowRun();

    private:
        const String    _name;
    };

    void setPeriod(byte period);
    void setPalette(Palette * pal);
    void setBrightness(byte brightness);
    byte getBrightness();
    void setAnim(Anim* anim) { _anim = anim; }
    bool run();//returns true if actual change has completed, or false if it's dummy call (previous call was too recent in time)
    void doSetUp();
    unsigned long getAvgCalcTime();
    unsigned long getAvgShowTime();
    
private:
    //Color arrays - two for making transition
    static Color leds1[LEDS];
    static Color leds2[LEDS];
    //auxiliary colors array
    static Color ledstmp[LEDS];

    // length of animation timeslot (period)
    byte period;
    // array of Color to work with
    Color *leds;
    int paletteInd = 0;
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

    Anim* _anim = nullptr;

    //glow animation setup
    void glowSetUp();

    //glow animation - must be called for each LED after it's BASIC color is set
    //note this overwrites the LED color, so the glow assumes that color will be stored elsewhere (not in leds[])
    //or computed each time regardless previous leds[] value
    void glowForEachLed(int i);
    
    //glow animation - must be called at the end of each animaton run
    void glowRun();

    void setUp();
};

unsigned int rng();

byte rngb();

#endif
