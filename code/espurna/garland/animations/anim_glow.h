#if GARLAND_SUPPORT

#include "../anim.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimGlow : public Anim {
   public:
    AnimGlow() : Anim("Glow") {
    }

    void SetupImpl() override {
        curColor = palette->getRndInterpColor();
        inc = secureRandom(2) * 2 - 1;
        glowSetUp();
    }

    void Run() override {
        if (inc > 0) {
            for (int i = 0; i < numLeds; ++i) {
                leds[i] = curColor;
                glowForEachLed(i);
            }
        } else {
            for (int i = 0; i < numLeds; ++i) {
                leds[i] = curColor;
                glowForEachLed(i);
            }
        }
        glowRun();
    }
};

#endif  // GARLAND_SUPPORT
