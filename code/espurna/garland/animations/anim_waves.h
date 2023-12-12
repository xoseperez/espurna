#if GARLAND_SUPPORT

#include "../anim.h"
#include "../palette.h"
#include "color_wave.h"

//------------------------------------------------------------------------------
class AnimWaves : public Anim {
   public:
    AnimWaves() : Anim("Waves") {
    }

    void SetupImpl() override {
        unsigned int waveLen = secureRandom(50, 100);
        bool cleanColors = secureRandom(10) > 7;
        byte fade = palette->bright() ? secureRandom(180, 220) : 0;
        wave = ColorWave(numLeds, palette, waveLen, cleanColors, fade);
        glowSetUp();
    }

    void Run() override {
        for (auto i = 0; i < numLeds; ++i) {
            leds[i] = wave.getLedColor(i);
            glowForEachLed(i);
        }

        wave.move();

        glowRun();
    }

    ColorWave wave;
};

#endif  // GARLAND_SUPPORT
