#include "anim.h"

void Anim::glowSetUp()
{
    braPhaseSpd = random(4,13);
    if (braPhaseSpd > 8) {
        braPhaseSpd = braPhaseSpd - 17;
    }
    braFreq = random(20,60);
}

void Anim::glowForEachLed(int i)
{
    int8 bra = braPhase + i * braFreq;
    bra = BRA_OFFSET + (abs(bra) >> BRA_AMP_SHIFT);
    leds[i] = leds[i].brightness(bra);
}

void Anim::glowRun()
{
    braPhase += braPhaseSpd;
}
