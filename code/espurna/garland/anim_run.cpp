#if GARLAND_SUPPORT

#include "anims.h"
#include "color.h"
#include "palette.h"
#include "scene.h"

AnimRun::AnimRun() : Scene::Anim("Run") {
}

void AnimRun::SetupImpl() {
    pos = 0;
    inc = 1 + (rngb() >> 5);
    if (random(10) > 5) {
        inc = -inc;
    }
}

void AnimRun::Run() {
    int p = pos;
    for (int i=0;i<LEDS;i++) {
        Color c = _palette->getPalColor((float)p/256);
        _leds[i] = c;
   
        p = p + inc;
        if (p >= 256) {
            p = p - 256;
        } else if (p < 0) {
            p = p + 256;
        }
    }
    pos = pos + 1;
}

AnimRun anim_run;

#endif // GARLAND_SUPPORT
