#if GARLAND_SUPPORT

#include "anims.h"
#include "color.h"
#include "palette.h"
#include "scene.h"

#define SPREAD_MAX_WIDTH 20

AnimSpread::AnimSpread() : Scene::Anim("Spread") {
}

void AnimSpread::SetupImpl() {
    inc = secureRandom(2, 8);
    for (auto& s : (*seq))
        s = 0;
}

void AnimSpread::Run() {
    for (int i = 0; i < LEDS; i++) {
        _leds[i] = 0;
        if ((*seq)[i] > 0) {
            byte width = SPREAD_MAX_WIDTH - (*seq)[i];
            for (int j = i - width; j <= (i + width); j++) {
                Color c = ledstmp[i];
                if (j >= 0 && j < LEDS) {
                    _leds[j].r += c.r;
                    _leds[j].g += c.g;
                    _leds[j].b += c.b;
                }
            }
            ledstmp[i].fade(255 / SPREAD_MAX_WIDTH);
            (*seq)[i]--;
        }
    }

    if (secureRandom(inc) == 0) {
        byte pos = secureRandom(0, LEDS);
        ledstmp[pos] = _palette->getRndInterpColor();
        (*seq)[pos] = SPREAD_MAX_WIDTH;
    }
}

#endif  // GARLAND_SUPPORT
