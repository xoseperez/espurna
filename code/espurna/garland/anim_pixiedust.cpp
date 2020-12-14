#if GARLAND_SUPPORT

#include "anims.h"
#include "color.h"
#include "palette.h"
#include "scene.h"

#define DUST_LENGTH 20

AnimPixieDust::AnimPixieDust() : Scene::Anim("PixieDust") {
}

void AnimPixieDust::SetupImpl() {
    phase = 0;
    curColor = _palette->getRndInterpColor();
    prevColor = _palette->getRndInterpColor();
    inc = random(2)*2-1;
    if (inc > 0) {
        phase = -DUST_LENGTH/2;
    } else {
        phase = LEDS + DUST_LENGTH/2;
    }
    glowSetUp();
}

void AnimPixieDust::Run() {
    if (inc > 0) {
        for (int i=0;i<LEDS;i++) {
            _leds[i] = (i > phase) ? prevColor : curColor;
            glowForEachLed(i);
        }
        phase++;
        if (phase >= 4*LEDS) {
            phase = -DUST_LENGTH/2;
            prevColor = curColor;
            curColor = _palette->getRndInterpColor();     
        }
    } else {
        for (int i=0;i<LEDS;i++) {
            _leds[i] = (i < phase) ? prevColor : curColor;
            glowForEachLed(i);
        }
        phase--;
        if (phase <= -3*LEDS) {
            phase = LEDS + DUST_LENGTH/2;
            prevColor = curColor;
            curColor = _palette->getContrastColor(prevColor);
        }
    }
    glowRun();
    
    for (int k = phase-DUST_LENGTH/2; k < (phase + DUST_LENGTH/2); k++ ) {
        if (k >= 0 && k < LEDS) {
            int mix = abs(k-phase) * 255 / DUST_LENGTH + random(-100, 100);
            if (mix < 0) { 
                mix = 0;
            } else if (mix > 255) {
                mix = 255;
            }
            _leds[k] = sparkleColor.interpolate(_leds[k], (float)mix/255);
        }
    }
}

#endif // GARLAND_SUPPORT
