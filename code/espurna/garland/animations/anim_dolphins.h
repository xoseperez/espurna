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
        dolphins.clear();
        for (int i = 0; i < 4; ++i)
            dolphins.emplace_back(palette, numLeds);
    }

    void Run() override {
        for (int i = 0; i < numLeds; ++i) {
            leds[i] = 0;
            seq[i] = 0;
        }

        // Run dolphins animation. Fill seq (accupied space)
        for (auto& d : dolphins)
            d.Run(leds, seq);

        // Try to recreate dolphins that have been done
        for (auto& d : dolphins) {
            if (d.done) {
                for (int i = 1; i < 5; ++i) {
                    Dolphin new_dolphin(palette, numLeds);
                    if (new_dolphin.HaveEnoughSpace(seq)) {
                        std::swap(d, new_dolphin);
                        break;
                    }
                }
            }
        }
    }

   private:
    struct Dolphin {
        bool done = false;
        int len = secureRandom(10, 20);
        int speed = secureRandom(1, 3);
        int dir = 1;
        int head = 0;
        int start;
        Color color;
        std::vector<Color> points;
        Dolphin(Palette* pal, uint16_t numLeds) : start(secureRandom(0, numLeds - len)), color(pal->getRndInterpColor()), points(len) {
            // DEBUG_MSG_P(PSTR("[GARLAND] Dolphin created start = %d len = %d dir = %d cr = %d cg = %d cb = %d\n"), start, len, dir, color.r, color.g, color.b);
            if (secureRandom(10) > 5) {
                start = numLeds - start;
                dir = -1;
            }

            int halflen = len / 2;
            for (int i = 0; i < halflen; ++i) {
                points[i] = points[len-i-1] = Color((byte)(color.r * i / halflen), (byte)(color.g * i / halflen), (byte)(color.b * i / halflen));
                // DEBUG_MSG_P(PSTR("[GARLAND] Dolphin i=%d cr = %d cg = %d cb = %d\n"), i, points[i].r, points[i].g, points[i].b);
            }
            if (len > halflen * 2) {
                points[halflen] = color;
            }
        }

        bool Run(Color* leds, byte* seq) {
            if (done)
                return false;

            int p = 0;
            for (int i = 0; i < len; ++i) {
                p = head - i;
                if (p >= 0 && p < len) {
                    leds[start + p * dir] = points[i];
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
                for (int i = s; i < len; ++i) {
                    seq[start + i * dir] = 1;
                }
            }

            return true;
        }

        // Decide that dolphin have ehough space if seq of len before it is empty
        bool HaveEnoughSpace(byte* seq) {
            for (int i = 0; i < len; ++i) {
                if (seq[start + i * dir] != 0) {
                    // DEBUG_MSG_P(PSTR("[GARLAND] Dolphin chaven't enouhg space to move.\n"));
                    return false;
                }
            }
            return true;
        }
    };

    std::vector<Dolphin> dolphins;
};

#endif  // GARLAND_SUPPORT
