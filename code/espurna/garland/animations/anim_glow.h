#if GARLAND_SUPPORT

#include "../anim.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimGlow : public Anim {
   private:
    float colorInterp;
    float colorInterpSpeed;
    uint16_t flashDeciPercent;
   public:
    AnimGlow() : Anim("Glow") {
    }

    void SetupImpl() override {
        prevColor = palette->getRndInterpColor();
        curColor = palette->getRndInterpColor();
        inc = secureRandom(2) * 2 - 1;
        
        colorInterp = 0;
        colorInterpSpeed = ((float)secureRandom(10, 20)) / 1000;
        flashDeciPercent = fiftyFifty() ? 0 : secureRandom(5, 20);
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

        for (auto i = 0; i < numLeds; ++i) {
            auto j = inc > 0 ? i : numLeds - i - 1;
            leds[j] = actualColor;
            glowForEachLed(j);
        }

        glowRun();
        if (flashDeciPercent) {
            flashRandomLeds(flashDeciPercent);
        }
    }
};

#endif  // GARLAND_SUPPORT
