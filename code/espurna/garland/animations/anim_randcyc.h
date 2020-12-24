#if GARLAND_SUPPORT

#include "../anim.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimRandCyc : public Anim {
   public:
    AnimRandCyc() : Anim("RandCyc") {
    }

    void SetupImpl() override {
        for (int i = 0; i < numLeds; ++i)
            seq[i] = rngb();
    }

    void Run() override {
        for (int i = 0; i < numLeds; ++i) {
            leds[i] = palette->getCachedPalColor(seq[i]);
            seq[i] += rngb() >> 6;
        }
    }
};

#endif  // GARLAND_SUPPORT
