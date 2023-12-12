#if GARLAND_SUPPORT

#include "../anim.h"
#include "../color.h"
#include "../palette.h"
#include "color_wave.h"

//------------------------------------------------------------------------------
class AnimCrossing : public Anim {
   public:
    AnimCrossing() : Anim("Crossing") {
    }

    void SetupImpl() override {

        wave1 = generateWave(1);
        wave2 = generateWave(-1);
    }

    void Run() override {
        for (auto i = 0; i < numLeds; ++i) {
            leds[i] = wave1.getLedColor(i);
            leds[i] = leds[i].interpolate(wave2.getLedColor(i), 0.5);
        }

        wave1.move();
        wave2.move();
    }

    private:


    ColorWave generateWave(int dir) {
        unsigned int waveLen = secureRandom(10, 50);
        bool cleanColors = secureRandom(10) > 5;
        bool startEmpty = secureRandom(10) > 5;
        float speed = secureRandom(5, 20) / 10.0;
        byte fade = secureRandom(10) > 5 ? 0 : palette->bright() ? secureRandom(180, 220) : 120;
        Palette* wavePal = &pals[secureRandom(palsNum)];

        return ColorWave(numLeds, wavePal, waveLen, cleanColors, fade, speed, dir, startEmpty);
    }

    ColorWave wave1;
    ColorWave wave2;
};

#endif  // GARLAND_SUPPORT
