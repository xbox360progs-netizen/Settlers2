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
    bool hasPickedUp;
    bool cargoDelivered;
    bool readyToRemove;
    CarrierComponent()
        : state(Working), currentJob(INVALID_ENTITY), cargoType(ResourceType_None), cargoAmount(0),
          hasPickedUp(false), cargoDelivered(false), readyToRemove(false) {}
};

struct PathfindingComponent : public Component {
    std::vector<Vector2i> tiles;
    float progress;
    float walkDir;
    CarrierState state;
    float pickupEp;
    float destEp;
    uint32_t pathVersion;
    
    PathfindingComponent() 
        : progress(0.0f), walkDir(1.0f), state(Working), pickupEp(0.0f), destEp(0.0f), pathVersion(0) {}
};

struct CarrierRenderComponent : public Component {
    int spriteIndex;
    float animationTimer;
    CarrierRenderComponent() : spriteIndex(-1), animationTimer(0) {}
};

struct CarrierInit {
    const std::vector<Vector2i>& tiles;
    CarrierState initialState;
    uint32_t pathVersion;

    CarrierInit(const std::vector<Vector2i>& tiles_, CarrierState state_, uint32_t pv = 0)
        : tiles(tiles_), initialState(state_), pathVersion(pv) {}
    };

    inline bool IsValidPath(const PathfindingComponent& pc) {
        return pc.tiles.size() >= 2 && (pc.walkDir == 1.0f || pc.walkDir == -1.0f);
    }

    inline bool IsValidPath(const std::vector<Vector2i>& tiles) {
        return tiles.size() >= 2;
    }

    class CarrierSystem {

public:
    CarrierSystem(EntityManager* entityManager);
    ~CarrierSystem();

    Entity CreateCarrier(const CarrierInit& init);
    void RemoveCarrier(Entity entity);
    void DestroyEntity(Entity entity);

    void UpdateMovement(float dt);
    void UpdatePath(Entity entity, const std::vector<Vector2i>& tiles);

    // Bridge: sync Carrier fields ↔ ECS PathfindingComponent (uses internal EntityManager)
    void SyncToCarrier(Entity entity, Carrier* carrier) const;
    void SyncFromCarrier(Entity entity, const Carrier* carrier);

    // Sync job-related leg targets from Carrier to PathfindingComponent
    void SyncLegTargets(Entity entity, const Carrier* carrier);

    // Debug: assert ECS invariants after SyncToCarrier
    void DebugECSInvariants(Entity entity, const Carrier* carrier) const;

    // Convert ECS pathfinding state → carrier ep coordinate space
    static float ComputeCarrierEP(const PathfindingComponent& pc);

private:
    EntityManager* m_entityManager;
    static const float CARRIER_SPEED;

    void UpdateWorking(Entity entity, PathfindingComponent& path, CarrierComponent& carrier, float dt);
    void MoveToTarget(PathfindingComponent& path, CarrierComponent& carrier, float dt);
    void MoveToCenter(PathfindingComponent& path, CarrierComponent& carrier, float dt);
};

}