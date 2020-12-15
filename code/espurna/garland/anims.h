#pragma once

#if GARLAND_SUPPORT

#include <list>

#include "anim.h"
#include "palette.h"

//------------------------------------------------------------------------------
class AnimStart : public Anim {
public:
    AnimStart();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimRun : public Anim {
public:
    AnimRun();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimStars : public Anim {
public:
    AnimStars();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimSpread : public Anim {
public:
    AnimSpread();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimSparkr : public Anim {
public:
    AnimSparkr();
    void SetupImpl() override;
    void Run() override;
private:
    void initSeq();
    void shuffleSeq();
};

//------------------------------------------------------------------------------
class AnimRandCyc : public Anim {
public:
    AnimRandCyc();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimFly : public Anim {
public:
    AnimFly();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimPixieDust : public Anim {
public:
    AnimPixieDust();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimComets : public Anim {
public:
    AnimComets();
    void SetupImpl() override;
    void Run() override;
private:
    struct Comet {
        float head;
        int len = secureRandom(10, 20);
        float speed = ((float)secureRandom(4, 10)) / 10;
        Color color;
        int dir = 1;
        std::vector<Color> points;
        Comet(Palette* pal, uint16_t numLeds) : head(secureRandom(0, numLeds / 2)), color(pal->getRndInterpColor()), points(len) {
            // DEBUG_MSG_P(PSTR("[GARLAND] Comet created head = %d len = %d speed = %g cr = %d cg = %d cb = %d\n"), head, len, speed, color.r, color.g, color.b);
            if (secureRandom(10) > 5) {
                head = numLeds - head;
                dir = -1;
            }

            for (int i = 0; i < len; ++i) {
                points[i] = Color((byte)(color.r * (len - i) / len), (byte)(color.g * (len - i) / len), (byte)(color.b * (len - i) / len));
            }
        }
    };

    std::list<Comet> comets;
};

//------------------------------------------------------------------------------
class AnimAssemble : public Anim {
public:
    AnimAssemble();
    void SetupImpl() override;
    void Run() override;
};

#endif // GARLAND_SUPPORT
