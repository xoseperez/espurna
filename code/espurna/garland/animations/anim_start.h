#if GARLAND_SUPPORT

#include "../anim.h"

//------------------------------------------------------------------------------
class AnimStart : public Anim {
   public:
    AnimStart() : Anim("Start") {
    }

    void SetupImpl() override {
        phase = 0;
    }

    void Run() override {
        if (phase < numLeds) {
            leds[phase].r = 255;
            leds[phase].g = 255;
            leds[phase].b = 255;
            for (int i = 0; i < numLeds; ++i) {
                leds[i].fade(50);
            }
        } else if (phase >= numLeds) {
            for (int i = 0; i < numLeds; ++i) {
                short r = numLeds + 255 - phase + rngb();
                r = min(r, (short)255);
                leds[i].r = (byte)max(r, (short)0);
                short g = numLeds + 255 - phase + rngb();
                g = min(g, (short)255);
                leds[i].g = (byte)max(g, (short)0);
                short b = numLeds + 255 - phase + rngb();
                b = min(b, (short)255);
                leds[i].b = (byte)max(b, (short)0);
            }
            phase++;
        }

        phase++;
    }
};
#endif  // GARLAND_SUPPORT
