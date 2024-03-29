#if GARLAND_SUPPORT

#include <vector>

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimDolphins : public Anim {
   public:
    AnimDolphins() : Anim("Dolphins") {
        cycleFactor = 3;
    }

    void SetupImpl() override {
        if (dolphins.size()) {
            for (auto& d : dolphins) {
                d = {palette, numLeds};
            }
        } else {
            for (auto i = 0; i < 5; ++i) {
                dolphins.emplace_back(palette, numLeds);
            }
        }
    }

    void Run() override {
        for (auto i = 0; i < numLeds; ++i) {
            leds[i] = 0;
            seq[i] = 0;
        }

        // Run dolphins animation. Fill seq (occupied space)
        for (auto& d : dolphins)
            d.Run(leds, seq);

        // Try to recreate dolphins that have been done
        for (auto& d : dolphins) {
            if (d.done) {
                for (int i = 1; i < 5; ++i) {
                    Dolphin new_dolphin(palette, numLeds);
                    if (new_dolphin.HaveEnoughSpace(seq)) {
                        d = std::move(new_dolphin);
                        break;
                    }
                }
            }
        }
    }

   private:
    struct Dolphin {
        uint16_t numLeds;
        bool done = false;
        int len = secureRandom(10, 30);
        int speed = secureRandom(1, 3);
        int dir = randDir();
        int head = 0;
        int start;
        Color color;
        std::unique_ptr<Color[]> points;
        Dolphin(Palette* pal, uint16_t numLeds) : numLeds(numLeds), start(dir > 0 ? secureRandom(0, numLeds - len) : secureRandom(len, numLeds)), color(pal->getRndInterpColor()) {
            // DEBUG_MSG_P(PSTR("[GARLAND] Dolphin created start = %d len = %d dir = %d cr = %d cg = %d cb = %d\n"), start, len, dir, color.r, color.g, color.b);

            int halflen = len / 2;

            points.reset(new Color[len]);
            for (auto i = 0; i < len; ++i) {
                int nth = (i > halflen) ? (len - i) : i;
                points[i] = {
                    (byte)(color.r * nth / halflen),
                    (byte)(color.g * nth / halflen),
                    (byte)(color.b * nth / halflen)};
            }
        }

        bool Run(Color* leds, byte* seq) {
            if (done)
                return false;

            int p = 0;
            for (auto i = 0; i < len; ++i) {
                p = head - i;
                if (p >= 0 && p < len) {
                    int j = start + p * dir;
                    if (j > 0 && j < numLeds) {
                        leds[j] = points[i];
                    }
                }
            }

            head += speed;

            // if tail moved out of len then dolphin is done
            if (p >= len) {
                done = true;
                return false;
            }
            else {
                // dolphin occupy space for future movement
                int s = p < 0 ? 0 : p;
                for (auto i = s; i < len; ++i) {
                    int j = start + i * dir;
                    if (j > 0 && j < numLeds) {
                        seq[j] = 1;
                    }
                }
            }

            return true;
        }

        // Decide that dolphin have ehough space if seq of len before it is empty
        bool HaveEnoughSpace(byte* seq) {
            for (auto i = 0; i < len; ++i) {
                int j = start + i * dir;
                if (j > 0 && j < numLeds) {
                    if (seq[j] != 0) {
                        // DEBUG_MSG_P(PSTR("[GARLAND] Dolphin chaven't enouhg space to move.\n"));
                        return false;
                    }
                }
            }
            return true;
        }
    };

    std::vector<Dolphin> dolphins;
};

#endif  // GARLAND_SUPPORT
