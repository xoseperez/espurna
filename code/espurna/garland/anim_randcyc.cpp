#include "scene.h"

void Scene::animRandCyc_SetUp() {
    for (int i=0;i<LEDS;i++) {
        seq[i] = rngb();
    }
}

void Scene::animRandCyc_Run() {
    for (int i=0;i<LEDS;i++) {
        leds[i] = palette->getPalColor((float)seq[i] / 256);
        seq[i]+=rngb() >> 6;
    }
}
