#if GARLAND_SUPPORT

#include "../anim.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimGlow : public Anim {
   private:
    float colorInterp;
    float colorInterpSpeed;
   public:
    AnimGlow() : Anim("Glow") {
    }

    void SetupImpl() override {
        prevColor = palette->getRndInterpColor();
        curColor = palette->getRndInterpColor();
        inc = secureRandom(2) * 2 - 1;
        
        colorInterp = 0;
        colorInterpSpeed = ((float)secureRandom(10, 20)) / 1000;
        glowSetUp();
    }

    void Run() override {
        Color actualColor = prevColor.interpolate(curColor, colorInterp);
        colorInterp += colorInterpSpeed;
        if (colorInterp >= 1.0) {
            colorInterp = 0;
            prevColor = curColor;
            curColor = palette->getContrastColor(prevColor);
        }

        if (inc > 0) {
            for (int i = 0; i < numLeds; ++i) {
                leds[i] = actualColor;
                glowForEachLed(i);
            }
        } else {
            for (int i = numLeds - 1 ; i >= 0; --i) {
                leds[i] = actualColor;
                glowForEachLed(i);
            }
        }
        glowRun();
    }
};

#endif  // GARLAND_SUPPORT
