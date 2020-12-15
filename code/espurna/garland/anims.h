#pragma once

#if GARLAND_SUPPORT

#include <list>

#include "scene.h"

//------------------------------------------------------------------------------
class AnimStart : public Scene::Anim {
public:
    AnimStart();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimRun : public Scene::Anim {
public:
    AnimRun();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimStars : public Scene::Anim {
public:
    AnimStars();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimSpread : public Scene::Anim {
public:
    AnimSpread();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimSparkr : public Scene::Anim {
public:
    AnimSparkr();
    void SetupImpl() override;
    void Run() override;
private:
    void initSeq();
    void shuffleSeq();
};

//------------------------------------------------------------------------------
class AnimRandCyc : public Scene::Anim {
public:
    AnimRandCyc();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimFly : public Scene::Anim {
public:
    AnimFly();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimPixieDust : public Scene::Anim {
public:
    AnimPixieDust();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimComets : public Scene::Anim {
    struct Comet {
        float head;
        int len = secureRandom(10, 20);
        float speed = ((float)secureRandom(4, 10)) / 10;
        Color color;
        int dir = 1;
        Comet(Palette* pal, uint16_t numLeds) : head(secureRandom(0, numLeds / 2)), color(pal->getRndInterpColor()) {
            // DEBUG_MSG_P(PSTR("[GARLAND] Comet created head = %d len = %d speed = %g cr = %d cg = %d cb = %d\n"), head, len, speed, color.r, color.g, color.b);
            if (secureRandom(10) > 5) {
                head = numLeds - head;
                dir = -1;
            }
        }
    };

    std::list<Comet> comets;
public:
    AnimComets();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimAssemble : public Scene::Anim {
public:
    AnimAssemble();
    void SetupImpl() override;
    void Run() override;
};

#endif // GARLAND_SUPPORT
