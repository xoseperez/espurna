#include "anim_run2.h"

AnimRun::AnimRun(Scene& scene) : Scene::Anim("Run", scene) {
}

void AnimRun::Setup() {
    pos = 0;
    inc = 1 + (rngb() >> 5);
    if (random(10) > 5) {
        inc = -inc;
    }
}

void AnimRun::Run() {
    int p = pos;
    for (int i=0;i<LEDS;i++) {
        Color c = _scene.palette->getPalColor((float)p/256);
        _scene.leds[i] = c;
   
        p = p + inc;
        if (p >= 256) {
            p = p - 256;
        } else if (p < 0) {
            p = p + 256;
        }
    }
    pos = pos + 1;
}