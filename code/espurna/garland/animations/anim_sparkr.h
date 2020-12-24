#if GARLAND_SUPPORT

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

#define SPARK_PROB 3  //probability of spark when in idle plase

//------------------------------------------------------------------------------
class AnimSparkr : public Anim {
   public:
    AnimSparkr() : Anim("Sparkr") {
    }
    void SetupImpl() override {
        glowSetUp();
        phase = 0;
        curColor = palette->getRndInterpColor();
        prevColor = palette->getRndInterpColor();
        initSeq();
        shuffleSeq();
    }

    void Run() override {
        for (int i = 0; i < numLeds; ++i) {
            byte pos = seq[i];

            leds[pos] = (i > phase) ? prevColor
                                    : (i == phase) ? sparkleColor
                                                   : curColor;
            glowForEachLed(i);
        }
        glowRun();

        if (phase > numLeds) {
            if (secureRandom(SPARK_PROB) == 0) {
                int i = (int)rngb() * numLeds / 256;
                leds[i] = sparkleColor;
            }
        }

        phase++;
        if (phase > 2 * numLeds) {
            phase = 0;
            prevColor = curColor;
            curColor = palette->getContrastColor(prevColor);
            shuffleSeq();
        }
    }
};

#endif  // GARLAND_SUPPORT
