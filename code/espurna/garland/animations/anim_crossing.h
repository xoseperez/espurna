#if GARLAND_SUPPORT

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

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
        for (int i = 0; i < numLeds; ++i) {
            leds[i] = wave1.getLedColor(i);
            leds[i] = leds[i].interpolate(wave2.getLedColor(i), 0.5);
        }

        wave1.move();
        wave2.move();
    }

    private:
    class ColorWave {
        public:
        uint16_t numLeds;
        Palette* palette;
        unsigned int waveLen; // wave length in leds (for clean colors - length of one color), also fade length
        bool cleanColors; // if true - use clean colors from palette, else - use interpolated colors       
        int dir; // moving direction (1 or -1)
        float speed; // wave moving speed
        byte fade; // wave fade deep - 0 - no fade, 255 - fade to black

        ColorWave() = default;
        ColorWave(uint16_t _numLeds, Palette* _palette, unsigned int _waveLen, bool _cleanColors, bool _startEmpty, int _dir, float _speed, byte _fade)
            : numLeds(_numLeds), palette(_palette), waveLen(_waveLen), cleanColors(_cleanColors), dir(_dir), speed(_speed), fade(_fade) {
            if (_startEmpty) {
                head = 0;
            } else {
                head = numLeds - 1;
            }
            fade_step = fade * 2 / waveLen;
            DEBUG_MSG_P(PSTR("[GARLAND] fade_step = %d\n"), fade_step);
        }

        Color getLedColor(unsigned int ledNum) {
            unsigned int real_led_num = dir > 0 ? ledNum : numLeds - ledNum - 1;
            // DEBUG_MSG_P(PSTR("[GARLAND] real_led_num = %d, head = %d\n"), real_led_num, (unsigned int)head);

            if (real_led_num > head) {
                return Color();
            }

            if (cleanColors) {
                unsigned int color_num = (head - real_led_num) / waveLen;
                Color c = palette->getCleanColor(color_num);
                // DEBUG_MSG_P(PSTR("[GARLAND] color_num = %d, color = %s\n"), color_num, c.to_str().c_str());
                return c;
            } else {
                byte interp_color = (byte)((float)((uint16_t)(head - real_led_num) % numLeds) / (float)numLeds * (float)256);
                return palette->getCachedPalColor(interp_color);
            }

            return Color();
        }

        void move() {
            head = head + speed;
        }

        private:
        float head;
        byte fade_step;
    };

    ColorWave generateWave(int dir) {
        unsigned int _waveLen = secureRandom(10, 30);
        bool _cleanColors = secureRandom(10) > 5;
        bool _startEmpty = secureRandom(10) > 5;
        float _speed = secureRandom(5, 20) / 10.0;
        byte _fade = secureRandom(0, 256);
        Palette* wavePal = &pals[secureRandom(palsNum)];
        DEBUG_MSG_P(PSTR("[GARLAND] Wave created waveLen = %d Pal: %-8s cleanColors = %d startEmpty = %d dir = %d speed = %d fade = %d\n"), _waveLen, wavePal->name(), _cleanColors, _startEmpty, dir, (int)(_speed * 10.0), _fade);
        return ColorWave(numLeds, wavePal, _waveLen, _cleanColors, _startEmpty, dir, _speed, _fade);
    }

    ColorWave wave1;
    ColorWave wave2;
};

#endif  // GARLAND_SUPPORT
