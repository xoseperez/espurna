#if GARLAND_SUPPORT

#include "../anim.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimWaves : public Anim {
    private:
     int bra_phase;
     int bra_phase_speed;
     byte bra_speed;
     byte bra_min;
     int sign;

   public:
    AnimWaves() : Anim("Waves") {
    }

    void SetupImpl() override {
        curColor = palette->getRndInterpColor().max_bright();
        glowSetUp();
        sign = braPhaseSpd > 128 ? -1 : 1;
        bra_phase = secureRandom(255);
        bra_phase_speed = secureRandom(2, 5);
        bra_speed = secureRandom(4, 12);
        bra_min = secureRandom(20, 30);
    }

    void Run() override {
        int bra = bra_phase;
        int8_t bra_inc = -sign * bra_speed;

        for (int i = 0; i < numLeds; ++i) {
            leds[i] = curColor;
            glowForEachLed(i);
            leds[i] = leds[i].brightness((byte)bra);
            bra += bra_inc;
            if (bra > 255 || bra < bra_min) {
                bra_inc = -bra_inc;
                bra = bra > 255 ? 255 : bra_min;
            }            
        }

        glowRun();

        bra_phase += bra_phase_speed;
        if (bra_phase > 255 || bra_phase < bra_min) {
            bra_phase_speed = -bra_phase_speed;
            sign = -sign;
            bra_phase = bra_phase > 255 ? 255 : bra_min;
        }
    }
};

#endif  // GARLAND_SUPPORT
