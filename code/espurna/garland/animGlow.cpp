#if GARLAND_SUPPORT

#include "scene.h"

void Scene::glowSetUp() {
    braPhaseSpd = secureRandom(4, 13);
    if (braPhaseSpd > 8) {
        braPhaseSpd = braPhaseSpd - 17;
    }
    braFreq = secureRandom(20, 60);
}

void Scene::glowForEachLed(int i) {
    int8 bra = braPhase + i * braFreq;
    bra = BRA_OFFSET + (abs(bra) >> BRA_AMP_SHIFT);
    leds[i] = leds[i].brightness(bra);
}

void Scene::glowRun() {
    braPhase += braPhaseSpd;
}

#endif  // GARLAND_SUPPORT
