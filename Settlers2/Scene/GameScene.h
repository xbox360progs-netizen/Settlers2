#pragma once

#include "Scene.h"
#include "../Graphics/RenderQueue.h"

namespace Scene {

class GameScene : public Scene
{
public:
    GameScene();
    virtual ~GameScene();

    virtual void Load();
    virtual void Unload();
    virtual void Update(float deltaTime);
    virtual void Render(class RenderQueue* renderQueue) override;
};

} // namespace Scene
