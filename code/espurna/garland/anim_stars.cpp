#if GARLAND_SUPPORT

#include "anims.h"
#include "color.h"
#include "palette.h"
#include "scene.h"

//seq keeps phases: 0..127 increasing, 128..255 decreasing, ends at 255 (steady off)
//ledstmp keeps color of stars

AnimStars::AnimStars() : Scene::Anim("Stars") {
}

void AnimStars::SetupImpl() {
    //inc is (average) interval between appearance of new stars
    inc = secureRandom(2, 5);

    //reset all phases
    memset(seq, 255, LEDS);
}

void AnimStars::Run() {
    for (byte i = 0; i < LEDS; i++) {
        byte phi = seq[i];
        if (phi < 254) {
            Color col = ledstmp[i];
            if (phi <= 127) {
                _leds[i] = col.brightness(phi << 1);
            } else {
                _leds[i] = col.brightness((255 - phi) << 1);
            }
            seq[i] += 2;
        } else {
            _leds[i].r = 0;
            _leds[i].g = 0;
            _leds[i].b = 0;
        }
    }

    if (secureRandom(inc) == 0) {
        byte pos = secureRandom(LEDS);
        if (seq[pos] > 250) {
            seq[pos] = 0;
            ledstmp[pos] = _palette->getRndInterpColor();
        }
    }
}

#endif  // GARLAND_SUPPORT
