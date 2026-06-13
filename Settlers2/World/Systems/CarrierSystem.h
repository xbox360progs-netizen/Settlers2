#pragma once
#include "../Component.h"
#include "../Entity.h"
#include "../ResourceNode.h"
#include "../Carrier.h"
#include "../../Core/Vector2i.h"
#include <vector>

namespace World {

class EntityManager;
class Carrier;

struct CarrierComponent : public Component {
    CarrierState state;
    Entity currentJob;
    ResourceType cargoType;
    int cargoAmount;
    CarrierComponent()
        : state(Working), currentJob(0), cargoType(ResourceType_None), cargoAmount(0) {}
};

struct PathfindingComponent : public Component {
    std::vector<Vector2i> tiles;
    float progress;
    float walkDir;
    PathfindingComponent() : progress(0.0f), walkDir(1.0f) {}
};

struct CarrierRenderComponent : public Component {
    int spriteIndex;
    float animationTimer;
    CarrierRenderComponent() : spriteIndex(-1), animationTimer(0) {}
};

class CarrierSystem {
public:
    CarrierSystem(EntityManager* entityManager);
    ~CarrierSystem();

    Entity CreateCarrier(const std::vector<Vector2i>& tiles, CarrierState initialState);
    void RemoveCarrier(Entity entity);
    void DestroyEntity(Entity entity);

    void UpdateMovement(float dt);
    void UpdatePath(Entity entity, const std::vector<Vector2i>& tiles);

    // Bridge: sync Carrier fields ↔ ECS PathfindingComponent (uses internal EntityManager)
    void SyncToCarrier(Entity entity, Carrier* carrier) const;
    void SyncFromCarrier(Entity entity, const Carrier* carrier);

private:
    EntityManager* m_entityManager;
    static const float CARRIER_SPEED;
};

}

