#pragma once

#include "../Entity.h"
#include <vector>

namespace World {
    class EntityManager;
}

namespace Renderer {
    class IRenderer;
}

namespace World {

class RenderSystem
{
public:
    RenderSystem();
    ~RenderSystem();

    void Update(EntityManager* entityManager, Renderer::IRenderer* renderer, float deltaTime);

private:
    void RenderSprite(EntityManager* entityManager, Renderer::IRenderer* renderer, EntityID entityID);
};

} // namespace World
