#if GARLAND_SUPPORT

#include "anims.h"
#include "color.h"
#include "palette.h"
#include "scene.h"
#include "espurna.h"

#include <list>

// struct Dot {
//     float point = random(0, LEDS/2);
//     int len = random(10, 20);
//     float speed = ((float)random(4, 10)) / 10;
//     Color color;
//     int dir = 1;
//     Dot(Palette* pal) : color(pal->getRndInterpColor()) {
//         // DEBUG_MSG_P(PSTR("[GARLAND] Dot created head = %d len = %d speed = %g cr = %d cg = %d cb = %d\n"), head, len, speed, color.r, color.g, color.b);
//         if (random(10) > 5) {
//             head = LEDS-head;
//             dir = -1;
//         }
//     }
// };

AnimAssemble::AnimAssemble() : Scene::Anim("Assemble") {

}

void AnimAssemble::SetupImpl() {
    inc = 1 + (rngb() >> 5);
    if (random(10) > 5) {
        inc = -inc;
    }
    
    int p = 0;
    for (int i = 0; i < LEDS; i++) {
        Color c = _palette->getPalColor((float)p / 256);
        _leds[i] = c;

        p = p + inc;
        if (p >= 256) {
            p = p - 256;
        } else if (p < 0) {
            p = p + 256;
        }
    }    
}

void AnimAssemble::Run() {

}

AnimAssemble anim_assemble;

#endif // GARLAND_SUPPORT