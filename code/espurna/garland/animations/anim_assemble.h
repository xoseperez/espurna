#if GARLAND_SUPPORT

#include <list>

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimAssemble : public Anim {
    enum class Phases {
        Assemmble,
        Glow,
        Fade
    };

    Phases phase = Phases::Assemmble;
   public:
    AnimAssemble() : Anim("Assemble") {
        cycleFactor = 2;
    }
    void SetupImpl() override {
        inc = 1 + (rngb() >> 5);
        if (secureRandom(10) > 5) {
            inc = -inc;
        }

        int p = 0;
        for (int i = 0; i < numLeds; ++i) {
            leds[i] = 0;
            Color c = palette->getCachedPalColor((byte)p);
            ledstmp[i] = c;

            p = p + inc;
            if (p >= 256) {
                p = p - 256;
            } else if (p < 0) {
                p = p + 256;
            }
        }
        initSeq();
        shuffleSeq();
        glowSetUp();
        pos = 0;
    }

    void Run() override {
        if (phase == Phases::Assemmble) {
            if (pos < numLeds) {
                byte cur_point = seq[pos];
                leds[cur_point] = ledstmp[cur_point];
                ++pos;
            } else {
                pos = 0;
                phase = Phases::Glow;
            }
        } else if (phase == Phases::Glow) {
            if (pos < numLeds/2) {
                for (int i = 0; i < numLeds; ++i) {
                    leds[i] = ledstmp[i];
                    glowForEachLed(i);
                }
                glowRun();
                ++pos;
            } else {
                pos = 0;
                phase = Phases::Fade;
            }
        } else {
            if (pos < numLeds) {
                byte cur_point = seq[pos];
                leds[cur_point] = 0;
                ++pos;
            } else {
                pos = 0;
                phase = Phases::Assemmble;
            }
        }
    }
};

#endif  // GARLAND_SUPPORT
