#pragma once

class ColorWave {
    public:
    ColorWave() = default;
    /* 
        _numLeds - number of leds in garland
        _palette - palette for wave colors
        _waveLen - wave length in leds (for clean colors - length of one color), also fade length
        _cleanColors - if true - use clean colors from palette, else - use interpolated colors
        _fade - wave fade deep: 0 - no fade, 255 - fade to black
        _speed - wave moving speed (0.5 - 2.0)
        _dir - moving direction (1 or -1)
        _startEmpty - if true - start wave from empty leds, else - start from full leds
    */
    ColorWave(uint16_t _numLeds, Palette* _palette, uint16_t _waveLen, bool _cleanColors, byte _fade = 0, float _speed = secureRandom(5, 15) / 10.0, int _dir = secureRandom(10) > 5 ? 1 : -1, bool _startEmpty = false)
        : numLeds(_numLeds), palette(_palette), waveLen(_waveLen), cleanColors(_cleanColors), dir(_dir), speed(_speed), fade(_fade) {
        DEBUG_MSG_P(PSTR("[GARLAND] Wave: waveLen = %d Pal: %-8s fade = %d cleanColors = %d speed = %d\n"), waveLen, palette->name(), fade, cleanColors, (int)(speed * 10.0));
        head = _startEmpty ? 0 : numLeds - 1;
        fade_step = fade * 2 / waveLen;
    }

    Color getLedColor(uint16_t ledNum) {
        uint16_t real_led_num = dir > 0 ? ledNum : numLeds - ledNum - 1;

        if (real_led_num > head) {
            return Color();
        }

        uint16_t dist_to_head = head - real_led_num;

        Color c = cleanColors ? palette->getCleanColor(dist_to_head / waveLen) : palette->getCachedPalColor(dist_to_head * 256 * 20 / waveLen / numLeds);

        if (fade_step > 0) {
            byte fadeFactor = 255 - abs(waveLen / 2 - (dist_to_head % waveLen)) * fade_step;
            c = c.brightness(fadeFactor);              
        }

        return c;
    }

    void move() {
        head += speed;
    }

    private:
    uint16_t numLeds;
    Palette* palette;
    uint16_t waveLen;
    bool cleanColors;
    int dir;
    float speed;
    byte fade;
    float head;
    byte fade_step;
};
