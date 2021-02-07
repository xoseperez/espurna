#if GARLAND_SUPPORT

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

//seq keeps phases: 0..127 increasing, 128..255 decreasing, ends at 255 (steady off)
//ledstmp keeps color of stars

//------------------------------------------------------------------------------
class AnimStars : public Anim {
   public:
    AnimStars() : Anim("Stars") {
    }

    void SetupImpl() override {
        //inc is (average) interval between appearance of new stars
        inc = secureRandom(2, 5);

        //reset all phases
        for (int i = 0; i < numLeds; ++i)
            seq[i] = 255;
    }

    void Run() override {
        for (int i = 0; i < numLeds; i++) {
            byte phi = seq[i];
            if (phi < 254) {
                Color col = ledstmp[i];
                if (phi <= 127) {
                    leds[i] = col.brightness(phi << 1);
                } else {
                    leds[i] = col.brightness((255 - phi) << 1);
                }
                seq[i] += 2;
            } else {
                leds[i].r = 0;
                leds[i].g = 0;
                leds[i].b = 0;
            }
        }

        if (secureRandom(inc) == 0) {
            byte pos = secureRandom(numLeds);
            if (seq[pos] > 250) {
                seq[pos] = 0;
                ledstmp[pos] = palette->getRndInterpColor();
            }
        }
    }
};

#endif  // GARLAND_SUPPORT
