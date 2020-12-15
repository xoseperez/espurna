#if GARLAND_SUPPORT

#include "anims.h"
#include "scene.h"

AnimSparkr::AnimSparkr() : Scene::Anim("Sparkr") {
}

void AnimSparkr::initSeq() {
    for (int i = 0; i < numLeds; i++)
        seq[i] = i;
}

void AnimSparkr::shuffleSeq() {
    for (int i = 0; i < numLeds; i++) {
        byte ind = (unsigned int)(rngb() * numLeds / 256);
        if (ind != i) {
            std::swap(seq[ind], seq[i]);
        }
    }
}

void AnimSparkr::SetupImpl() {
    glowSetUp();
    phase = 0;
    curColor = palette->getRndInterpColor();
    prevColor = palette->getRndInterpColor();
    initSeq();
    shuffleSeq();
}

void AnimSparkr::Run() {
    for (int i = 0; i < numLeds; i++) {
        byte pos = seq[i];

        leds[pos] = (i > phase) ? prevColor
                                 : (i == phase) ? sparkleColor
                                                : curColor;
        glowForEachLed(i);
    }
    glowRun();

    if (phase > numLeds) {
        if (secureRandom(SPARK_PROB) == 0) {
            int i = (int)rngb() * numLeds / 256;
            leds[i] = sparkleColor;
        }
    }

    phase++;
    if (phase > 2 * numLeds) {
        phase = 0;
        prevColor = curColor;
        curColor = palette->getContrastColor(prevColor);
        shuffleSeq();
    }
}

#endif  // GARLAND_SUPPORT
