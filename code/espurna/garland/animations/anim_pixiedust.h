#if GARLAND_SUPPORT

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

#define DUST_LENGTH 20

//------------------------------------------------------------------------------
class AnimPixieDust : public Anim {
   public:
    AnimPixieDust() : Anim("PixieDust") {
    }

    void SetupImpl() override {
        phase = 0;
        curColor = palette->getRndInterpColor();
        prevColor = palette->getRndInterpColor();
        inc = secureRandom(2) * 2 - 1;
        if (inc > 0) {
            phase = -DUST_LENGTH / 2;
        } else {
            phase = numLeds + DUST_LENGTH / 2;
        }
        glowSetUp();
    }

    void Run() override {
        if (inc > 0) {
            for (int i = 0; i < numLeds; ++i) {
                leds[i] = (i > phase) ? prevColor : curColor;
                glowForEachLed(i);
            }
            phase++;
            if (phase >= 4 * numLeds) {
                phase = -DUST_LENGTH / 2;
                prevColor = curColor;
                curColor = palette->getRndInterpColor();
            }
        } else {
            for (int i = 0; i < numLeds; ++i) {
                leds[i] = (i < phase) ? prevColor : curColor;
                glowForEachLed(i);
            }
            phase--;
            if (phase <= -3 * numLeds) {
                phase = numLeds + DUST_LENGTH / 2;
                prevColor = curColor;
                curColor = palette->getContrastColor(prevColor);
            }
        }
        glowRun();

        for (int k = phase - DUST_LENGTH / 2; k < (phase + DUST_LENGTH / 2); k++) {
            if (k >= 0 && k < numLeds) {
                int mix = abs(k - phase) * 255 / DUST_LENGTH + ((int)rngb() - 125);
                if (mix < 0) {
                    mix = 0;
                } else if (mix > 255) {
                    mix = 255;
                }
                leds[k] = sparkleColor.interpolate(leds[k], (float)mix / 255);
            }
        }
    }
};

#endif  // GARLAND_SUPPORT
