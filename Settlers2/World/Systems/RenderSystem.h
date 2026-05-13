#pragma once

#include "../../Graphics/RenderTypes.h"
#include "../Entity.h"
#include <vector>

namespace World {
    class EntityManager;
}

namespace World {

class RenderSystem
{
public:
    RenderSystem();
    ~RenderSystem();

    void Update(EntityManager* entityManager, Renderer* renderer, float deltaTime);

private:
    void RenderSprite(EntityManager* entityManager, Renderer* renderer, EntityID entityID);
};

} // namespace World
