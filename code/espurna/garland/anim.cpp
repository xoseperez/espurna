#if GARLAND_SUPPORT

#include "anim.h"
#include "palette.h"

Anim::Anim(const char* name) : _name(name) {}

void Anim::Setup(Palette* palette, uint16_t numLeds, Color* leds, Color* ledstmp, byte* seq) {
    this->palette = palette;
    this->numLeds = numLeds;
    this->leds = leds;
    this->ledstmp = ledstmp;
    this->seq = seq;
    SetupImpl();
}

void Anim::glowSetUp() {
    braPhaseSpd = secureRandom(4, 13);
    if (braPhaseSpd > 8) {
        braPhaseSpd = braPhaseSpd - 17;
    }
    braFreq = secureRandom(20, 60);
}

void Anim::glowForEachLed(int i) {
    int8 bra = braPhase + i * braFreq;
    bra = BRA_OFFSET + (abs(bra) >> BRA_AMP_SHIFT);
    leds[i] = leds[i].brightness(bra);
}

void Anim::glowRun() { braPhase += braPhaseSpd; }

bool operator== (const Color &c1, const Color &c2)
{
    return (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b);
}

unsigned int rng() {
    static unsigned int y = 0;
    y += micros();  // seeded with changing number
    y ^= y << 2;
    y ^= y >> 7;
    y ^= y << 7;
    return (y);
}

// Ranom numbers generator in byte range (256) much faster than secureRandom.
// For usage in time-critical places.
byte rngb() { return (byte)rng(); }

#endif
