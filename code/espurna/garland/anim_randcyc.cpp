#if GARLAND_SUPPORT

#include "anims.h"
#include "scene.h"

AnimRandCyc::AnimRandCyc() : Anim("RandCyc") {
}

void AnimRandCyc::SetupImpl() {
    for (int i = 0; i < numLeds; i++)
        seq[i] = rngb();
}

void AnimRandCyc::Run() {
    for (int i = 0; i < numLeds; i++) {
        leds[i] = palette->getCachedPalColor(seq[i]);
        seq[i] += rngb() >> 6;
    }
}

#endif  // GARLAND_SUPPORT
