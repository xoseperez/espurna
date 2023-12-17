#if GARLAND_SUPPORT

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimSpread : public Anim {
    const byte minWidth = 10;
    const byte maxWidth = 15;
    const byte numTries = 5;
    byte fade_ind = 10;

    int GeneratePos() {
        int pos = -1;
        for (auto i = 0; i < numTries; ++i) {
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
        fade_ind = secureRandom(1, 20);
        inc = secureRandom(2, 4);
        // DEBUG_MSG_P(PSTR("[GARLAND] AnimSpread inc = %d, fade_inc = %d\n"), inc, fade_ind);
        for (auto i = 0; i < numLeds; ++i)
            seq[i] = 0;
    }

    void Run() override {
        for (auto i = 0; i < numLeds; ++i)
            leds[i] = 0;

        for (auto i = 0; i < numLeds; ++i) {
            if (!ledstmp[i].empty()) {
                byte width = maxWidth - seq[i];
                for (int j = i - width; j <= (i + width); j++) {
                    Color c = ledstmp[i];
                    if (j >= 0 && j < numLeds) {
                        if (leds[j].empty()) {
                            leds[j] = c;
                        } else {
                            leds[j] = leds[j].interpolate(c, 0.5);
                        }
                    }
                }
                ledstmp[i].fade(fade_ind);
                seq[i]--;
            } else {
                seq[i] = 0;
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
