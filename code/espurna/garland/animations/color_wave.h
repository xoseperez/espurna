#pragma once

enum WaveType {
    Wave,
    Comet
};

class ColorWave {
   public:
    struct Params {
        uint16_t numLeds;   // number of leds in garland
        Palette *palette;   // palette for wave colors
        uint16_t waveLen;   // wave length in leds (for clean colors - length of one color), also fade length
        bool cleanColors;   // if true - use clean colors from palette, else - use interpolated colors
        byte fade;          // wave fade deep: 0 - no fade, 255 - fade to black
        Color* pixelCache;  // pointer to array of Color with numLeds size, if not null - use it for caching colors
        WaveType type;      // Wave or Comet
        float speed;        // wave moving speed (0.5 - 2.0)
        int dir;            // moving direction (1 or -1)
        bool startEmpty;    // if true - start wave from empty leds, else - start from full leds
    };

    ColorWave() = default;
    ColorWave(Params params) : params(params) {
        if (params.speed < 0) {
            params.speed = -params.speed;
            params.dir = -params.dir;
        }

        head = params.startEmpty ? 0 : params.numLeds - 1;

        if (params.type == WaveType::Wave) {
            fade_step = params.fade * 2 / params.waveLen;
        } else {
            fade_step = 255 / params.waveLen;
        }

        DEBUG_MSG_P(PSTR("[GARLAND] Wave: waveLen = %d Pal: %-8s fade = %d cleanColors = %d speed = %d, type = %d\n"),
                    params.waveLen, params.palette->name(), params.fade, params.cleanColors, (int)(params.speed * 10.0), params.type);

        if (params.pixelCache) {
            for (auto i = 0; i < params.numLeds; ++i) {
                params.pixelCache[i] = Color::empty();
            }
        }
    }

    Color getLedColor(uint16_t ledNum) {
        uint16_t real_led_num = params.dir > 0 ? ledNum : params.numLeds - ledNum - 1;

        if (real_led_num > head) {
            return Color::empty();
        }

        if (params.pixelCache && !(params.pixelCache[real_led_num].is_empty())) {
            return params.pixelCache[real_led_num];
        }

        uint16_t dist_to_head = head - real_led_num;
        Color c = params.cleanColors ? params.palette->getCleanColor(dist_to_head / params.waveLen)
         : params.palette->getCachedPalColor(dist_to_head * 256 * 20 / params.waveLen / params.numLeds);

        if (fade_step > 0) {
            byte bright = 255;

            if (params.type == WaveType::Wave) {
                bright -= fade_step * abs(params.waveLen / 2 - (dist_to_head % params.waveLen));
            } else {
                bright -= fade_step * (dist_to_head % params.waveLen);
            }

            c = c.brightness(bright);
        }

        if (params.pixelCache) {
            params.pixelCache[real_led_num] = c;
        }

        return c;
    }

    void move() {
        uint16_t prevHead = head;
        head += params.speed;
        uint16_t headDelta = head - prevHead;

        if (params.pixelCache && headDelta > 0) {
            for (auto i = params.numLeds - 1; i >= 0; --i) {
                if (i >= headDelta) {
                    params.pixelCache[i] = params.pixelCache[i - headDelta];
                } else {
                    params.pixelCache[i] = Color::empty();
                }
            }
        }
    }

   private:
    Params params;
    float head;
    byte fade_step;
};
