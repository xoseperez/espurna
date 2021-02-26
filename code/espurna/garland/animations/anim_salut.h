#if GARLAND_SUPPORT

#include <vector>

#include "../anim.h"
#include "../color.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimSalut : public Anim {
   public:
    AnimSalut() : Anim("Salut") {
    }

    void SetupImpl() override {
        shots.clear();
        // There can be more then one shot at the moment
        // but looks like one is enough
        // for (int i = 0; i < 3; ++i)
            shots.emplace_back(palette, numLeds);
    }

    void Run() override {
        for (int i = 0; i < numLeds; ++i) leds[i] = 0;

        for (auto& c : shots) {
            if (!c.Run(leds)) {
                Shot new_shot(palette, numLeds);
                std::swap(c, new_shot);
            }
        }
    }

   private:
    struct Shot {
       private:
        struct Spark {
            bool done = false;
            float speed = ((float)secureRandom(1, 25)) / 10;
            float speed_dec = ((float)secureRandom(1, 3)) / 10;
            float pos;
            int dir;
            Color color;
            uint16_t numLeds;
            Spark(int pos, Palette* pal, uint16_t numLeds) : pos(pos), dir(secureRandom(10) > 5 ? -1 : 1), color(pal->getRndInterpColor()), numLeds(numLeds) {}
            void Run(Color* leds) {
                if (pos >= 0 && pos < numLeds) {
                    leds[(int)pos] = color;
                    if (speed > 0) {
                        pos += speed * dir;
                        speed -= speed_dec;
                    } else {
                        color.fade(5);
                        if (color.empty()) {
                            if (secureRandom(10) > 8)
                                leds[(int)pos] = 0xFFFFFF;
                            done = true;
                        }
                    }
                } else {
                    done = true;
                }
            }
        };

       public:
        int spark_num = secureRandom(30, 40);
        int center;
        std::vector<Spark> sparks;
        Shot(Palette* pal, uint16_t numLeds) : center(secureRandom(15, numLeds - 15)) {
            // DEBUG_MSG_P(PSTR("[GARLAND] Shot created center = %d spark_num = %d\n"), center, spark_num);
            sparks.reserve(spark_num);
            for (int i = 0; i < spark_num; ++i) {
                sparks.emplace_back(center, pal, numLeds);
            }
        }
        bool Run(Color* leds) {
            bool done = true;
            for (auto& s : sparks) {
                if (!s.done) {
                    done = false;
                    s.Run(leds);
                }
            }
            return !done;
        }
    };

    std::vector<Shot> shots;
};

#endif  // GARLAND_SUPPORT
