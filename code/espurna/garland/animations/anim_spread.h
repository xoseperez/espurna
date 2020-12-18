#if GARLAND_SUPPORT

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

#define SPREAD_MAX_WIDTH 20

//------------------------------------------------------------------------------
class AnimSpread : public Anim {
   public:
    AnimSpread() : Anim("Spread") {
    }

    void SetupImpl() override {
        inc = secureRandom(2, 8);
        for (int i = 0; i < numLeds; i++)
            seq[i] = 0;
    }

    void Run() override {
        for (int i = 0; i < numLeds; i++)
            leds[i] = 0;

        for (int i = 0; i < numLeds; i++) {
            if (seq[i] > 0) {
                byte width = SPREAD_MAX_WIDTH - seq[i];
                for (int j = i - width; j <= (i + width); j++) {
                    Color c = ledstmp[i];
                    if (j >= 0 && j < numLeds) {
                        leds[j].r += c.r;
                        leds[j].g += c.g;
                        leds[j].b += c.b;
                    }
                }
                ledstmp[i].fade(255 / SPREAD_MAX_WIDTH);
                seq[i]--;
            }
        }

        if (secureRandom(inc) == 0) {
            byte pos = secureRandom(0, numLeds);
            ledstmp[pos] = palette->getRndInterpColor();
            seq[pos] = SPREAD_MAX_WIDTH;
        }
    }
};

#endif  // GARLAND_SUPPORT
