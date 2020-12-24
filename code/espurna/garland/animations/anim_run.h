#if GARLAND_SUPPORT

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimRun : public Anim {
   public:
    AnimRun() : Anim("Run") {
    }

    void SetupImpl() override {
        pos = 0;
        inc = 1 + (rngb() >> 5);
        if (secureRandom(10) > 5) {
            inc = -inc;
        }
    }

    void Run() override {
        int p = pos;
        for (int i = 0; i < numLeds; ++i) {
            Color c = palette->getCachedPalColor((byte)p);
            leds[i] = c;

            p = p + inc;
            if (p >= 256) {
                p = p - 256;
            } else if (p < 0) {
                p = p + 256;
            }
        }
        pos = pos + 1;
    }
};

#endif  // GARLAND_SUPPORT
