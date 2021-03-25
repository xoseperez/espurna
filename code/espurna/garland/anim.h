/*
Part of the GARLAND MODULE
Copyright (C) 2020 by Dmitry Blinov <dblinov76 at gmail dot com>

Inspired by https://github.com/Vasil-Pahomov/ArWs2812 (currently https://github.com/Vasil-Pahomov/Liana)
*/

#pragma once

#include "color.h"

#define BRA_AMP_SHIFT          1    // brigthness animation amplitude shift. true BrA amplitude is calculated
                                    // as (0..127) value shifted right by this amount
#define BRA_OFFSET           127    //(222-64) // brigthness animation amplitude offset

class Palette;

class Anim {
public:
    Anim(const char* name);
    const char* name() { return _name; }
    void Setup(Palette* palette, uint16_t numLeds, Color* leds, Color* _ledstmp, byte* seq);
    virtual bool finishedycle() const { return true; };
    virtual void Run() = 0;
    virtual void setCycleFactor(float new_cycle_factor) { cycleFactor = new_cycle_factor; }
    virtual float getCycleFactor() { return cycleFactor; }

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
    byte        braPhase = 0;
    //braPhase change speed
    byte        braPhaseSpd  = 8;
    //BrA frequency (spatial)
    byte        braFreq      = 40;

    Color       curColor     = Color(0);
    Color       prevColor    = Color(0);
    const Color sparkleColor = Color(0xFFFFFF);

    // Reversed analog of speed. To control speed for particular animation (fine tuning for one).
    // Lower - faster. Set cycleFactor < 1 speed up animation, while cycleFactor > 1 slow it down.
    float       cycleFactor = 1.0;

    virtual void SetupImpl() = 0;

    // helper functions for animations
    void initSeq();
    void shuffleSeq();

    //glow animation setup
    void glowSetUp();

    //glow animation - must be called for each LED after it's BASIC color is set
    //note this overwrites the LED color, so the glow assumes that color will be stored elsewhere (not in leds[])
    //or computed each time regardless previous leds[] value
    void glowForEachLed(int i);

    //glow animation - must be called at the end of each animaton run
    void glowRun();

private:
    const char* _name;
};

unsigned int rng();
byte         rngb();
