#if GARLAND_SUPPORT

#include "../anim.h"
#include "../palette.h"
#include "color_wave.h"

//------------------------------------------------------------------------------
class AnimWaves : public Anim {
   public:
    enum Type {
        LongWaves,
        ShortWaves,
        Comets,
        CrossWaves,
    };

    AnimWaves(Type _type) : Anim("Waves"), type(_type) {
    }

    void SetupImpl() override {
        switch (type) {
            case LongWaves:
                wave1 = generateLongWave();
                break;
            case ShortWaves:
                wave1 = generateShortWave();
                break;
            case Comets:
                wave1 = generateCometWave();
                break;
            case CrossWaves:
                WaveType wave1Type = secureRandom(100) > 70 ? WaveType::Comet : WaveType::Wave;
                wave1 = generateCrossingWave(1, wave1Type, ledstmp);

                WaveType wave2Type = secureRandom(100) > 60 ? WaveType::Comet : WaveType::Wave;

                // if first wave is comet - inverse second wave probability to be comer to 60%
                if (wave1Type == WaveType::Comet) {
                    wave2Type = wave2Type == WaveType::Comet ? WaveType::Wave : WaveType::Comet; 
                }

                // we have only one pixel array for cache
                wave2 = generateCrossingWave(-1, wave2Type, nullptr);
                break;
        }
        glowSetUp();
    }

    void Run() override {
        for (auto i = 0; i < numLeds; ++i) {
            leds[i] = wave1.getLedColor(i);

            if (type == LongWaves) {
                glowForEachLed(i);
            }
        }

        wave1.move();

        if (type == LongWaves) {
            glowRun();
        } else if (type == CrossWaves) {
            for (auto i = 0; i < numLeds; ++i) {
                leds[i] = leds[i].interpolate(wave2.getLedColor(i), 0.5);
            }

            wave2.move();
        }
    }

    private:
    ColorWave generateLongWave() {
        unsigned int waveLen = secureRandom(50, 100);
        bool cleanColors = secureRandom(10) > 7;
        return ColorWave(numLeds, palette, waveLen, cleanColors, 0, ledstmp);
    }

    ColorWave generateShortWave() {
        unsigned int waveLen = secureRandom(10, 30);
        bool cleanColors = secureRandom(10) > 7;
        byte fade = palette->bright() ? secureRandom(180, 220) : 0;
        return ColorWave(numLeds, palette, waveLen, cleanColors, fade, ledstmp);
    }

    ColorWave generateCometWave() {
        unsigned int waveLen = secureRandom(10, 40);
        bool cleanColors = secureRandom(10) > 5;
        return ColorWave(numLeds, palette, waveLen, cleanColors, 0, ledstmp, WaveType::Comet);
    }

    ColorWave generateCrossingWave(int dir,  WaveType waveType, Color* pixelCache) {
        unsigned int waveLen = secureRandom(10, 50);
        bool cleanColors = fiftyFifty();
        bool startEmpty = fiftyFifty();
        float speed = secureRandom(5, 20) / 10.0;
        byte fade = fiftyFifty() ? 0 : palette->bright() ? secureRandom(180, 220) : 120;
        Palette* wavePal = &pals[secureRandom(palsNum)];

        return ColorWave(numLeds, wavePal, waveLen, cleanColors, fade, pixelCache, waveType, speed, dir, startEmpty);
    }

    Type type;
    ColorWave wave1;
    ColorWave wave2;
};

#endif  // GARLAND_SUPPORT
