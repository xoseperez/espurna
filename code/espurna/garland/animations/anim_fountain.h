#if GARLAND_SUPPORT

#include <vector>

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimFountain : public Anim {
   public:
    AnimFountain() : Anim("Fountain") {
        cycleFactor = 4;
    }

    void SetupImpl() override {
        fountains.clear();
        for (int i = 0; i < 3; ++i)
            fountains.emplace_back(palette, numLeds);
    }

    void Run() override {
        for (int i = 0; i < numLeds; ++i) {
            leds[i] = 0;
            seq[i] = 0;
        }

        // Run fountains animation. Fill seq (occupied space)
        for (auto& d : fountains)
            d.Run(leds, seq);

        // Try to recreate finished fountains
        for (auto& d : fountains) {
            if (d.done) {
                for (int i = 1; i < 5; ++i) {
                    Fountain new_fountain(palette, numLeds);
                    if (new_fountain.HaveEnoughSpace(seq)) {
                        std::swap(d, new_fountain);
                        break;
                    }
                }
            }
        }
    }

   private:
    struct Fountain {
        bool done = false;
        int len = secureRandom(5, 10);
        int speed = secureRandom(1, 3);
        int dir = 1;
        int head = 0;
        int start;
        // Color color;
        std::vector<Color> points;
        Fountain(Palette* pal, uint16_t numLeds) : start(secureRandom(len, numLeds - len)), /*color(pal->getRndInterpColor()),*/ points(len) {
            // DEBUG_MSG_P(PSTR("[GARLAND] Fountain created start = %d len = %d dir = %d cr = %d cg = %d cb = %d\n"), start, len, dir, color.r, color.g, color.b);
            if (secureRandom(10) > 5) {
                start = numLeds - start;
                dir = -1;
            }

            // int halflen = len / 2;
            for (int i = 0; i < len; ++i) {
                points[i] = pal->getRndInterpColor();
                // DEBUG_MSG_P(PSTR("[GARLAND] Fountain i=%d cr = %d cg = %d cb = %d\n"), i, points[i].r, points[i].g, points[i].b);
            }
        }

        bool Run(Color* leds, byte* seq) {
            if (done)
                return false;

            int p = 0;
            for (int i = 0; i < len; ++i) {
                p = head - i;
                if (p >= 0 && p < len) {
                    if (dir == 1) {
                        leds[start + p] = points[i];
                        leds[start - p] = points[i];
                    } else {
                        leds[start + len - p] = points[i];
                        leds[start - len + p] = points[i];
                    }
                }
            }

            head += speed;

            // if tail moved out of len then fountain is done
            if (p >= len) {
                done = true;
                return false;
            }
            else {
                // fountain occupy space for future movement
                int s = p < 0 ? 0 : p;
                for (int i = s; i < len; ++i) {
                    seq[start + i] = 1;
                    seq[start - i] = 1;
                }
            }

            return true;
        }

        // Decide that fountain have ehough space if seq of len before it is empty
        bool HaveEnoughSpace(byte* seq) {
            for (int i = 0; i < len; ++i) {
                if (seq[start + i] != 0 && seq[start - i] != 0) {
                    // DEBUG_MSG_P(PSTR("[GARLAND] Fountain chaven't enouhg space to move.\n"));
                    return false;
                }
            }
            return true;
        }
    };

    std::vector<Fountain> fountains;
};

#endif  // GARLAND_SUPPORT
