#if GARLAND_SUPPORT

#include "anims.h"
#include "color.h"
#include "palette.h"
#include "scene.h"
#include "espurna.h"

#include <list>

struct Comet {
    float head = random(0, LEDS/2);
    int len = random(10, 20);
    float speed = ((float)random(4, 10)) / 10;
    Color color;
    int dir = 1;
    Comet(Palette* pal) : color(pal->getPalColor((float)rngb()/256)) {
        // DEBUG_MSG_P(PSTR("[GARLAND] Comet created head = %d len = %d speed = %g cr = %d cg = %d cb = %d\n"), head, len, speed, color.r, color.g, color.b);
        if (random(10) > 5) {
            head = LEDS-head;
            dir = -1;
        }
    }
};

std::list<Comet> comets;

AnimComets::AnimComets() : Scene::Anim("Comets") {
}

void AnimComets::SetupImpl() {
    comets.clear();
    for (int i=0; i<4; ++i)
        comets.emplace_back(_palette);
}

void AnimComets::Run() {
    for (int i = 0; i < LEDS; i++) _leds[i] = 0;

    for (auto& c : comets) {
        int tail = c.head + c.len * -c.dir;
        // Check if Comet out of range and generate it again
        if ((c.head < 0 && tail < 0) || (c.head >= LEDS && tail >= LEDS)) {
            Comet new_comet(_palette); 
            std::swap(c, new_comet);
        }

        for (int l = 0; l < c.len; ++l) {
            int p = c.head + l * -c.dir;
            if (p >= 0 && p < LEDS) {
                Color bpc = Color((byte)(c.color.r * (c.len - l) / c.len), (byte)(c.color.g * (c.len - l) / c.len), (byte)(c.color.b * (c.len - l) / c.len));
                if (_leds[p].empty()) {
                    _leds[p] = bpc;
                } else {
                    _leds[p] = _leds[p].interpolate(bpc, 0.5);
                }
            }
        }
        c.head = c.head + c.speed * c.dir;
    }
}

AnimComets anim_comets;

#endif // GARLAND_SUPPORT