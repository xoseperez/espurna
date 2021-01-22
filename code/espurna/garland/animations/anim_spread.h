#if GARLAND_SUPPORT

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimSpread : public Anim {
    const byte minWidth = 10;
    const byte maxWidth = 15;
    const byte numTries = 5;
    const byte fade_ind = 255 / minWidth;

    int GeneratePos() {
        int pos = -1;
        for (int i = 0; i < numTries; ++i) {
            pos = secureRandom(0, numLeds);
            for (int j = pos - maxWidth; j < pos + maxWidth; ++j) {
                if (j >= 0 && j < numLeds && seq[j] > 0) {
                    pos = -1;
                    break;
                }
            }
            if (pos >= 0) break;
        }
        return pos;
    }

   public:
    AnimSpread() : Anim("Spread") {
        cycleFactor = 2;
    }

    void SetupImpl() override {
        inc = secureRandom(2, 4);
        // DEBUG_MSG_P(PSTR("[GARLAND] AnimSpread inc = %d\n"), inc);
        for (int i = 0; i < numLeds; ++i)
            seq[i] = 0;
    }

    void Run() override {
        for (int i = 0; i < numLeds; ++i)
            leds[i] = 0;

        for (int i = 0; i < numLeds; ++i) {
            if (seq[i] > 0) {
                byte width = maxWidth - seq[i];
                for (int j = i - width; j <= (i + width); j++) {
                    Color c = ledstmp[i];
                    if (j >= 0 && j < numLeds) {
                        leds[j].r += c.r;
                        leds[j].g += c.g;
                        leds[j].b += c.b;
                    }
                }
                ledstmp[i].fade(fade_ind);
                seq[i]--;
            }
        }

        if (secureRandom(inc) == 0) {
            int pos = GeneratePos();
            if (pos == -1)
                return;

            ledstmp[pos] = palette->getRndInterpColor();
            seq[pos] = secureRandom(minWidth, maxWidth);
        }
    }
};

#endif  // GARLAND_SUPPORT
