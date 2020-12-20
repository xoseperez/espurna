#if GARLAND_SUPPORT

#include "scene.h"

#include <Adafruit_NeoPixel.h>

#include "color.h"
#include "palette.h"
#include "debug.h"

#define TRANSITION_MS      1000    // transition time between animations, ms
#define SPEED_MAX          70
#define SPEED_FACTOR       10
#define DEFAULT_SPEED      50
#define DEFAULT_BRIGHTNESS 255

Scene::Scene(Adafruit_NeoPixel* pixels)
    : _pixels(pixels),
      _numLeds(pixels->numPixels()),
      _leds1(_numLeds),
      _leds2(_numLeds),
      _ledstmp(_numLeds),
      _seq(_numLeds) {
}

void Scene::setPalette(Palette* palette) {
    _palette = palette;
    if (setUpOnPalChange) {
        setupImpl();
    }
}

void Scene::setBrightness(byte brightness) {
    DEBUG_MSG_P(PSTR("[GARLAND] Scene::setBrightness = %d\n"), brightness);
    this->brightness = brightness;
}

byte Scene::getBrightness() {
    DEBUG_MSG_P(PSTR("[GARLAND] Scene::getBrightness = %d\n"), brightness);
    return brightness;
}

// Speed is reverse to cycleFactor and 10x
void Scene::setSpeed(byte speed) {
    this->speed = speed;
    cycleFactor = (float)(SPEED_MAX - speed) / SPEED_FACTOR;
    DEBUG_MSG_P(PSTR("[GARLAND] Scene::setSpeed %d cycleFactor = %d\n"), speed, (int)(cycleFactor * 1000));
}

byte Scene::getSpeed() {
    DEBUG_MSG_P(PSTR("[GARLAND] Scene::getSpeed %d cycleFactor = %d\n"), speed, (int)(cycleFactor * 1000));
    return speed;
}

void Scene::setDefault() {
    speed = DEFAULT_SPEED;
    cycleFactor = (float)(SPEED_MAX - speed) / SPEED_FACTOR;
    brightness = DEFAULT_BRIGHTNESS;
    DEBUG_MSG_P(PSTR("[GARLAND] Scene::setDefault speed = %d cycleFactor = %d brightness = %d\n"), speed, (int)(cycleFactor * 1000), brightness);
}

void Scene::run() {
    unsigned long iteration_start_time = micros();

    if (state == Calculate || cyclesRemain < 1) {
        // Calculate number of cycles for this animation iteration
        float cycleSum = cycleFactor * (_anim ? _anim->getCycleFactor() : 1.0) + cycleTail;
        cyclesRemain = cycleSum;
        if (cyclesRemain < 1) {
            cyclesRemain = 1;
            cycleSum = 0;
            cycleTail = 0;
        } else {
            cycleTail = cycleSum - cyclesRemain;
        }

        if (_anim) {
            _anim->Run();
        }

        sum_calc_time += (micros() - iteration_start_time);
        iteration_start_time = micros();
        ++calc_num;
        state = Transition;
    }
    
    if (state == Transition && cyclesRemain < 3) {
        // transition coef, if within 0..1 - transition is active
        // changes from 1 to 0 during transition, so we interpolate from current
        // color to previous
        float transc = (float)((long)transms - (long)millis()) / TRANSITION_MS;
        Color* leds_prev = (_leds == &_leds1[0]) ? &_leds2[0] : &_leds1[0];

        if (transc > 0) {
            for (int i = 0; i < _numLeds; i++) {
                // transition is in progress
                Color c = _leds[i].interpolate(leds_prev[i], transc);
                byte r = (int)(bri_lvl[c.r]) * brightness / 256;
                byte g = (int)(bri_lvl[c.g]) * brightness / 256;
                byte b = (int)(bri_lvl[c.b]) * brightness / 256;
                _pixels->setPixelColor(i, _pixels->Color(r, g, b));
            }
        } else {
            for (int i = 0; i < _numLeds; i++) {
                // regular operation
                byte r = (int)(bri_lvl[_leds[i].r]) * brightness / 256;
                byte g = (int)(bri_lvl[_leds[i].g]) * brightness / 256;
                byte b = (int)(bri_lvl[_leds[i].b]) * brightness / 256;
                _pixels->setPixelColor(i, _pixels->Color(r, g, b));
            }
        }

        sum_pixl_time += (micros() - iteration_start_time);
        iteration_start_time = micros();
        ++pixl_num;
        state = Show;
    }
    
    if (state == Show && cyclesRemain < 2) {    
        _pixels->show();
        sum_show_time += (micros() - iteration_start_time);
        ++show_num;
        state = Calculate;
        ++numShows;
    }
    --cyclesRemain;
}

void Scene::setupImpl() {
    transms = millis() + TRANSITION_MS;

    // switch operation buffers (for transition to operate)
    if (_leds == &_leds1[0]) {
        _leds = &_leds2[0];
    } else {
        _leds = &_leds1[0];
    }

    if (_anim) {
        _anim->Setup(_palette, _numLeds, _leds, &_ledstmp[0], &_seq[0]);
    }
}

void Scene::setup() {
    sum_calc_time = 0;
    sum_pixl_time = 0;
    sum_show_time = 0;
    calc_num = 0;
    pixl_num = 0;
    show_num = 0;
    numShows = 0;

    if (!setUpOnPalChange) {
        setupImpl();
    }
}

unsigned long Scene::getAvgCalcTime() { return sum_calc_time / calc_num; }
unsigned long Scene::getAvgPixlTime() { return sum_pixl_time / pixl_num; }
unsigned long Scene::getAvgShowTime() { return sum_show_time / show_num; }

#endif  // GARLAND_SUPPORT
