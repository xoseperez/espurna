#if GARLAND_SUPPORT

#include "anims.h"
#include "scene.h"

AnimRandCyc::AnimRandCyc() : Scene::Anim("RandCyc") {
}

void AnimRandCyc::SetupImpl() {
    for (int i=0;i<LEDS;i++) {
        seq[i] = rngb();
    }
}

void AnimRandCyc::Run() {
    for (int i=0;i<LEDS;i++) {
        _leds[i] = _palette->getPalColor((float)seq[i] / 256);
        seq[i]+=rngb() >> 6;
    }
}

AnimRandCyc anim_rand_cyc;

#endif // GARLAND_SUPPORT
