#if GARLAND_SUPPORT

#include "../anim.h"
#include "../palette.h"

//------------------------------------------------------------------------------
class AnimRandRun : public Anim {
   private:
   float speed;
   float pos;
   int dir;

   public:
    AnimRandRun() : Anim("RandRun") {
    }

    void SetupImpl() override {
        pos = 0;
        speed = ((float)secureRandom(20, 90)) / 100;
        DEBUG_MSG_P(PSTR("[GARLAND] speed = %d\n"), (unsigned int)(speed*100));
        dir = secureRandom(10) > 5 ? -1 : 1;

        for (int i = 0; i < numLeds; ++i)
            ledstmp[i] = palette->getRndInterpColor();

        glowSetUp();
    }

    void Run() override {
        pos += speed * dir;
        if (pos >= numLeds) pos -= numLeds;
        if (pos < 0) pos += numLeds;
            
        for (int i = 0; i < numLeds; ++i) {
            int j = i + pos;
            if (j >= numLeds) j -= numLeds;
            leds[i] = ledstmp[j];
            glowForEachLed(i);
        }
        glowRun();
    }
};

#endif  // GARLAND_SUPPORT
