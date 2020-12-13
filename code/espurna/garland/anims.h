#if GARLAND_SUPPORT

#ifndef anims_h
#define anims_h

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
    void initSeq(byte * seq);
    void shuffleSeq(byte * seq);
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
public:
    AnimComets();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
extern AnimStart        anim_start;
extern AnimRun          anim_run;
extern AnimStars        anim_stars;
extern AnimSpread       anim_spread;
extern AnimSparkr       anim_sparkr;
extern AnimRandCyc      anim_rand_cyc;
extern AnimFly          anim_fly;
extern AnimPixieDust    anim_pixel_dust;
extern AnimComets       anim_comets;

#endif //anims_h

#endif // GARLAND_SUPPORT
