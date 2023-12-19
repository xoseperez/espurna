#pragma once

enum WaveType {
    Wave,
    Comet
};

class ColorWave {
   public:
    ColorWave() = default;
    /*
        _numLeds - number of leds in garland
        _palette - palette for wave colors
        _waveLen - wave length in leds (for clean colors - length of one color), also fade length
        _cleanColors - if true - use clean colors from palette, else - use interpolated colors
        _fade - wave fade deep: 0 - no fade, 255 - fade to black
        _pixelCache - pointer to array of Color with numLeds size, if not null - use it for caching colors
        _speed - wave moving speed (0.5 - 2.0)
        _dir - moving direction (1 or -1)
        _startEmpty - if true - start wave from empty leds, else - start from full leds
    */
    ColorWave(
        uint16_t _numLeds,
        Palette *_palette,
        uint16_t _waveLen,
        bool _cleanColors,
        byte _fade = 0,
        Color* _pixelCache = nullptr,
        WaveType _type = WaveType::Wave,
        float _speed = secureRandom(5, 15) / 10.0,
        int _dir = randDir(),
        bool _startEmpty = false)
        : numLeds(_numLeds), palette(_palette), waveLen(_waveLen), cleanColors(_cleanColors), fade(_fade), pixelCache(_pixelCache), type(_type), speed(_speed), dir(_dir) {
        if (speed < 0) {
            speed = -speed;
            dir = -dir;
        }

        head = _startEmpty ? 0 : numLeds - 1;

        if (type == WaveType::Wave) {
            fade_step = fade * 2 / waveLen;
        } else {
            fade_step = 255 / waveLen;
        }

        DEBUG_MSG_P(PSTR("[GARLAND] Wave: waveLen = %d Pal: %-8s fade = %d cleanColors = %d speed = %d, type = %d\n"),
                    waveLen, palette->name(), fade, cleanColors, (int)(speed * 10.0), type);

        if (pixelCache) {
            for (auto i = 0; i < numLeds; ++i) {
                pixelCache[i] = Color();
            }
        }
    }

    Color getLedColor(uint16_t ledNum) {
        uint16_t real_led_num = dir > 0 ? ledNum : numLeds - ledNum - 1;

        if (real_led_num > head) {
            return Color();
        }

        if (pixelCache && !(pixelCache[real_led_num] == Color())) {
            return pixelCache[real_led_num];
        }

        uint16_t dist_to_head = head - real_led_num;
        Color c = cleanColors ? palette->getCleanColor(dist_to_head / waveLen) : palette->getCachedPalColor(dist_to_head * 256 * 20 / waveLen / numLeds);

        if (fade_step > 0) {
            byte bright = 255;

            if (type == WaveType::Wave) {
                bright -= fade_step * abs(waveLen / 2 - (dist_to_head % waveLen));
            } else {
                bright -= fade_step * (dist_to_head % waveLen);
            }

            c = c.brightness(bright);
        }

        if (pixelCache) {
            pixelCache[real_led_num] = c;
        }

        return c;
    }

    void move() {
        uint16_t prevHead = head;
        head += speed;
        uint16_t headDelta = head - prevHead;

        if (pixelCache && headDelta > 0) {
            for (auto i = numLeds - 1; i >= 0; --i) {
                if (i >= headDelta) {
                    pixelCache[i] = pixelCache[i - headDelta];
                } else {
                    pixelCache[i] = Color();
                }
            }
        }
    }

   private:
    uint16_t numLeds;
    Palette *palette;
    uint16_t waveLen;
    bool cleanColors;
    byte fade;
    Color* pixelCache;
    WaveType type;
    float speed;
    int dir;
    float head;
    byte fade_step;
};
