#if GARLAND_SUPPORT

#include "anims.h"
#include "scene.h"

AnimStart::AnimStart() : Scene::Anim("Start") {
}

void AnimStart::SetupImpl() {
    phase = 0;
}

void AnimStart::Run() {
    if (phase < LEDS) {
        _leds[phase].r = 255;
        _leds[phase].g = 255;
        _leds[phase].b = 255;
        for(int i=0; i<LEDS; i++) {
            _leds[i].fade(50);
        }        
    } else if (phase >= LEDS) 
    {
        for(int i=0; i<LEDS; i++) {
            short r = LEDS + 255 - phase + rngb();
            r = min(r,(short)255); _leds[i].r = (byte)max(r,(short)0);
            short g = LEDS + 255 - phase + rngb();
            g = min(g,(short)255); _leds[i].g = (byte)max(g,(short)0);
            short b = LEDS + 255 - phase + rngb();
            b = min(b,(short)255); _leds[i].b = (byte)max(b,(short)0);
        }
        phase++;
    }

    phase++;
}

AnimStart anim_start;

#endif // GARLAND_SUPPORT
