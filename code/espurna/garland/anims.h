#ifndef anims_h
#define anims_h

#include "scene.h"

//------------------------------------------------------------------------------
class AnimRun : public Scene::Anim {
public:
    AnimRun();
    void SetupImpl() override;
    void Run() override;
};

//------------------------------------------------------------------------------
class AnimFly : public Scene::Anim {
public:
    AnimFly();
    void SetupImpl() override;
    void Run() override;
    Color curColor = Color(0);
};

//------------------------------------------------------------------------------
class AnimPixieDust : public Scene::Anim {
public:
    AnimPixieDust();
    void SetupImpl() override;
    void Run() override;
    Color curColor = Color(0);
    Color prevColor = Color(0);
};

//------------------------------------------------------------------------------
extern AnimRun          anim_run;
extern AnimFly          anim_fly;
extern AnimPixieDust    anim_pixel_dust;

#endif //anims_h