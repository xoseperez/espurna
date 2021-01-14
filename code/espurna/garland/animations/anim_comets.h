#if GARLAND_SUPPORT

#include <list>

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimComets : public Anim {
   public:
    AnimComets() : Anim("Comets") {
    }

    void SetupImpl() override {
        comets.clear();
        for (int i = 0; i < 4; ++i)
            comets.emplace_back(palette, numLeds);
    }

    void Run() override {
        for (int i = 0; i < numLeds; ++i) leds[i] = 0;

        for (auto& c : comets) {
            int tail = c.head + c.len * -c.dir;
            // Check if Comet out of range and generate it again
            if ((c.head < 0 && tail < 0) || (c.head >= numLeds && tail >= numLeds)) {
                Comet new_comet(palette, numLeds);
                std::swap(c, new_comet);
            }

            for (int l = 0; l < c.len; ++l) {
                int p = c.head + l * -c.dir;
                if (p >= 0 && p < numLeds) {
                    leds[p] = c.points[l];
                }
            }
            c.head = c.head + c.speed * c.dir;
        }
    }

   private:
    struct Comet {
        float head;
        int len = secureRandom(10, 20);
        float speed = ((float)secureRandom(4, 10)) / 10;
        Color color;
        int dir = 1;
        std::vector<Color> points;
        Comet(Palette* pal, uint16_t numLeds) : head(secureRandom(0, numLeds / 2)), color(pal->getRndInterpColor()), points(len) {
            // DEBUG_MSG_P(PSTR("[GARLAND] Comet created head = %d len = %d speed = %g cr = %d cg = %d cb = %d\n"), head, len, speed, color.r, color.g, color.b);
            if (secureRandom(10) > 5) {
                head = numLeds - head;
                dir = -1;
            }

            for (int i = 0; i < len; ++i) {
                points[i] = Color((byte)(color.r * (len - i) / len), (byte)(color.g * (len - i) / len), (byte)(color.b * (len - i) / len));
            }
        }
    };

    std::list<Comet> comets;
};

#endif  // GARLAND_SUPPORT
