#if GARLAND_SUPPORT

#include "anims.h"
#include "scene.h"

AnimRandCyc::AnimRandCyc() : Scene::Anim("RandCyc") {
}

void AnimRandCyc::SetupImpl() {
    for (auto& s : (*seq))
        s = rngb();
}

void AnimRandCyc::Run() {
    for (int i = 0; i < LEDS; i++) {
        _leds[i] = _palette->getPalColor((float)(*seq)[i] / 256);
        (*seq)[i] += rngb() >> 6;
    }
}

#endif  // GARLAND_SUPPORT
