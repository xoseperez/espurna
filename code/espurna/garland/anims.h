#pragma once

#if GARLAND_SUPPORT

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
