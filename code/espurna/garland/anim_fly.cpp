#if GARLAND_SUPPORT

#include "anims.h"
#include "color.h"
#include "palette.h"
#include "scene.h"

AnimFly::AnimFly() : Anim("Fly") {
}

void AnimFly::SetupImpl() {
    //length of particle tail
    pos = secureRandom(2, 15);
    //probability of the tail
    inc = secureRandom(5, 15);
    if (secureRandom(10) > 5) {
        inc = -inc;
    }
    phase = 0;
}

void AnimFly::Run() {
    byte launchpos;
    if (inc > 0) {
        launchpos = numLeds - 1;
        for (int i = 1; i < numLeds; i++) {
            leds[i - 1] = leds[i];
        }
    } else {
        launchpos = 0;
        for (int i = numLeds - 2; i >= 0; i--) {
            leds[i + 1] = leds[i];
        }
    }

    if (secureRandom(abs(inc)) == 0) {
        curColor = palette->getRndInterpColor();
        phase = pos;
    }

    leds[launchpos] = Color((int)curColor.r * phase / pos, (int)curColor.g * phase / pos, (int)curColor.b * phase / pos);
    if (phase > 0) phase--;
}

#endif  // GARLAND_SUPPORT
