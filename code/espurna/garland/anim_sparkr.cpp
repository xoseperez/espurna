#if GARLAND_SUPPORT

#include "anims.h"
#include "scene.h"

AnimSparkr::AnimSparkr() : Scene::Anim("Sparkr") {
}

void AnimSparkr::initSeq() {
    for (int i = 0; i < LEDS; i++)
        (*seq)[i] = i;
}

void AnimSparkr::shuffleSeq() {
    for (int i = 0; i < LEDS; i++) {
        byte ind = (unsigned int)(rngb() * LEDS / 256);
        if (ind != i) {
            std::swap((*seq)[ind], (*seq)[i]);
        }
    }
}

void AnimSparkr::SetupImpl() {
    glowSetUp();
    phase = 0;
    curColor = _palette->getRndInterpColor();
    prevColor = _palette->getRndInterpColor();
    initSeq();
    shuffleSeq();
}

void AnimSparkr::Run() {
    for (int i = 0; i < LEDS; i++) {
        byte pos = (*seq)[i];

        _leds[pos] = (i > phase) ? prevColor
                                 : (i == phase) ? sparkleColor
                                                : curColor;
        glowForEachLed(i);
    }
    glowRun();

    if (phase > LEDS) {
        if (secureRandom(SPARK_PROB) == 0) {
            int i = (int)rngb() * LEDS / 256;
            _leds[i] = sparkleColor;
        }
    }

    phase++;
    if (phase > 2 * LEDS) {
        phase = 0;
        prevColor = curColor;
        curColor = _palette->getContrastColor(prevColor);
        shuffleSeq();
    }
}

#endif  // GARLAND_SUPPORT
