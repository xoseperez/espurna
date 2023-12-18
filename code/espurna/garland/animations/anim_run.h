#if GARLAND_SUPPORT

#include "../anim.h"
#include "../color.h"
#include "../palette.h"
#include "color_wave.h"

//------------------------------------------------------------------------------
class AnimRun : public Anim {
   public:
    AnimRun() : Anim("Run") {
    }

    void SetupImpl() override {
        unsigned int waveLen = secureRandom(10, 30);
        bool cleanColors = secureRandom(10) > 7;
        byte fade = palette->bright() ? secureRandom(180, 220) : 0;
        wave = ColorWave(numLeds, palette, waveLen, cleanColors, fade, ledstmp);
    }

    void Run() override {
        for (auto i = 0; i < numLeds; ++i) {
            leds[i] = wave.getLedColor(i);
        }

        wave.move();
    }

    ColorWave wave;
};

#endif  // GARLAND_SUPPORT
