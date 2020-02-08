#ifndef anim_run2_h
#define anim_run2_h

#include "scene.h"

class AnimRun : public Scene::Anim {
public:
    AnimRun(Scene& scene);
    void Setup() override;
    void Run() override;
};

#endif //anim_run2_h