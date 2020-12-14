#if GARLAND_SUPPORT

#include "anims.h"
#include "color.h"
#include "palette.h"
#include "scene.h"


AnimFly::AnimFly() : Scene::Anim("Fly") {
}

void AnimFly::SetupImpl() {
    //length of particle tail
    pos = random(2, 15);
    //probability of the tail
    inc = random(5, 15);
    if (random(10) > 5) {
        inc = -inc;
    }
    phase = 0;
}

void AnimFly::Run() {
    byte launchpos;
    if (inc > 0) {
        launchpos = LEDS-1;
        for (int i=1;i<LEDS;i++) {
            _leds[i-1] = _leds[i];
        }
    } else {
        launchpos = 0;
        for (int i=LEDS-2;i>=0;i--) {
            _leds[i+1] = _leds[i];
        }
    }

    if (random(abs(inc)) == 0) {
        curColor = _palette->getRndNeighborInterpColor();
        phase = pos;
    }

    _leds[launchpos] = Color( (int)curColor.r * phase / pos, (int)curColor.g * phase / pos, (int)curColor.b * phase / pos) ;
    if (phase > 0) phase--; 
}

#endif // GARLAND_SUPPORT
