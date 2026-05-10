#pragma once

#include "Scene.h"

namespace Scene {

class GameScene : public Scene
{
public:
    GameScene();
    virtual ~GameScene();

    virtual void Load();
    virtual void Unload();
    virtual void Update(float deltaTime);
    virtual void Render();
};

} // namespace Scene
