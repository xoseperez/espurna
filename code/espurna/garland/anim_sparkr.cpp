#if GARLAND_SUPPORT

#include "anims.h"
#include "scene.h"

AnimSparkr::AnimSparkr() : Scene::Anim("Sparkr") {
}

void AnimSparkr::initSeq(byte * seq) {
    for (int i=0; i<LEDS; i++) {
        seq[i] = i;
    }
}

void AnimSparkr::shuffleSeq(byte * seq) {
    for (int i=0; i<LEDS; i++) {
        byte ind = (unsigned int) ( rngb() * LEDS / 256);
        if (ind != i) {
            byte tmp = seq[ind];
            seq[ind] = seq[i];
            seq[i] = tmp;
        }
    }
}

void AnimSparkr::SetupImpl() {
    glowSetUp();
    phase = 0;
    curColor = _palette->getRndNeighborInterpColor();
    prevColor = _palette->getRndNeighborInterpColor();
    initSeq(seq);
    shuffleSeq(seq);


}

void AnimSparkr::Run() {
    for (int i=0;i<LEDS;i++) {
        byte pos = seq[i];

        _leds[pos] = (i > phase)
            ? prevColor 
            : (i == phase) ? sparkleColor : curColor;
        glowForEachLed(i);
    }
    glowRun();

    if (phase > LEDS) {
        if (random(SPARK_PROB) == 0) {
            int i = (int)rngb() * LEDS / 256;
            _leds[i] = sparkleColor;
        }    
    }

    phase++;
    if (phase > 2*LEDS) {
        phase = 0;
        prevColor = curColor;
        while (prevColor.isCloseTo(curColor)) {
            curColor = _palette->getRndNeighborInterpColor();     
        }
        shuffleSeq(seq);
    }
}

#endif // GARLAND_SUPPORT
