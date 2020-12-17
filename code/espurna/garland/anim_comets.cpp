#if GARLAND_SUPPORT

#include <list>

#include "anims.h"
#include "color.h"
#include "palette.h"
#include "scene.h"

AnimComets::AnimComets() : Anim("Comets") {
}

void AnimComets::SetupImpl() {
    comets.clear();
    for (int i = 0; i < 4; ++i)
        comets.emplace_back(palette, numLeds);
}

void AnimComets::Run() {
    for (int i = 0; i < numLeds; i++) leds[i] = 0;

    for (auto& c : comets) {
        int tail = c.head + c.len * -c.dir;
        // Check if Comet out of range and generate it again
        if ((c.head < 0 && tail < 0) || (c.head >= numLeds && tail >= numLeds)) {
            Comet new_comet(palette, numLeds);
            std::swap(c, new_comet);
        }

        for (int l = 0; l < c.len; ++l) {
            int p = c.head + l * -c.dir;
            if (p >= 0 && p < numLeds) {
                leds[p] = c.points[l];
            }
        }
        c.head = c.head + c.speed * c.dir;
    }
}

#endif  // GARLAND_SUPPORT
