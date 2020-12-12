#if GARLAND_SUPPORT

#include <Adafruit_NeoPixel.h>
#include "color.h"
#include "palette.h"
#include "scene.h"
#include "brightness.h"

//Adafruit's class to operate strip
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LEDS, PIN, NEO_GRB + NEO_KHZ800); 


// byte skip_count = 0;
// bool calc_or_show = false;

//=============================================================================
// Scene
//=============================================================================

void Scene::setPeriod(byte period) {
    this->period = period;
}

void Scene::setPalette(Palette * pal) {
    this->palette = pal;
    if (setUpOnPalChange) {
        setupImpl();
    }
}

void Scene::setBrightness(byte brightness) {
    this->brightness = brightness;
}

byte Scene::getBrightness() {
    return brightness;
}

bool Scene::run()
{    
    if (sum_num != 0 && (millis() - start_time) / sum_num < period) {
        return false;
    }

    ++sum_num;

    unsigned long iteration_start_time = millis();
    // if (calc_or_show) {

        if (_anim) {
            _anim->Run();
        }

        //transition coef, if within 0..1 - transition is active
        //changes from 1 to 0 during transition, so we interpolate from current color to previous
        float transc = (float)((long)transms - (long)millis()) / TRANSITION_MS;
        Color * leds_prev = (leds == leds1) ? leds2 : leds1;
        
        if (transc > 0) {
            for(int i=0; i<LEDS; i++) {
                //transition is in progress
                Color c = leds[i].interpolate(leds_prev[i], transc);
                byte r = (int)pgm_read_byte_near(BRI + c.r) * brightness / 256;
                byte g = (int)pgm_read_byte_near(BRI + c.g) * brightness / 256;
                byte b = (int)pgm_read_byte_near(BRI + c.b) * brightness / 256;
                pixels.setPixelColor(i, pixels.Color(r, g, b));
            }
        } else {
            for(int i=0; i<LEDS; i++) {
                //regular operation
                byte r = (int)pgm_read_byte_near(BRI + leds[i].r) * brightness / 256;
                byte g = (int)pgm_read_byte_near(BRI + leds[i].g) * brightness / 256;
                byte b = (int)pgm_read_byte_near(BRI + leds[i].b) * brightness / 256;
                pixels.setPixelColor(i, pixels.Color(r, g, b));
            }
        }

        ++calc_num;
        sum_calc_time += (millis() - iteration_start_time);
    // } else {
        pixels.show();
        ++show_num;
        sum_show_time += (millis() - iteration_start_time);
    // }

    // calc_or_show = !calc_or_show;

    return true;
}

void Scene::setupImpl()
{
    transms = millis() + TRANSITION_MS;

    //switch operation buffers (for transition to operate)
    
    if (leds == leds1) {
        leds = leds2;
    } else {
        leds = leds1;
    }

    if (_anim) {
        _anim->Setup(paletteInd, leds);
    }

}

void Scene::setup()
{
    start_time = millis();
    sum_calc_time = 0;
    sum_show_time = 0;
    sum_num = 0;
    calc_num = 0;
    show_num = 0;

    if (!setUpOnPalChange) {
        setupImpl();
    }
}

unsigned long Scene::getAvgCalcTime() {
    return sum_calc_time / calc_num;
}

unsigned long Scene::getAvgShowTime() {
    return sum_show_time / show_num;
}

unsigned int rng() {
    static unsigned int y = 0;
    y += micros(); // seeded with changing number
    y ^= y << 2; y ^= y >> 7; y ^= y << 7;
    return (y);
}

byte rngb() {
    return (byte)rng();
}


Color Scene::leds1[LEDS];
Color Scene::leds2[LEDS];
Color Scene::ledstmp[LEDS];
byte  Scene::Anim::seq[LEDS];
Color Scene::Anim::ledstmp[LEDS];

//=============================================================================
// Scene::Anim
//=============================================================================
Scene::Anim::Anim(String name)
    : _name(name) {
} 

void Scene::Anim::Setup(int paletteInd , Color* leds) {
    _palette = pals[paletteInd];
    _leds = leds;
    SetupImpl();
}

void Scene::Anim::glowSetUp()
{
    braPhaseSpd = random(4,13);
    if (braPhaseSpd > 8) {
        braPhaseSpd = braPhaseSpd - 17;
    }
    braFreq = random(20,60);
}

void Scene::Anim::glowForEachLed(int i)
{
    int8 bra = braPhase + i * braFreq;
    bra = BRA_OFFSET + (abs(bra) >> BRA_AMP_SHIFT);
    _leds[i] = _leds[i].brightness(bra);
}

void Scene::Anim::glowRun()
{
    braPhase += braPhaseSpd;
}

#endif // GARLAND_SUPPORT
