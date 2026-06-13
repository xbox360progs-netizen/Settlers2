#include "stdafx.h"
#include "CarrierSystem.h"
#include "../EntityManager.h"

namespace World {

const float CarrierSystem::CARRIER_SPEED = 3.0f;

CarrierSystem::CarrierSystem(EntityManager* entityManager)
    : m_entityManager(entityManager)
{
}

CarrierSystem::~CarrierSystem() {
}

Entity CarrierSystem::CreateCarrier(const std::vector<Vector2i>& tiles, CarrierState initialState) {
    Entity entity = m_entityManager->CreateEntity();

    CarrierComponent& cc = m_entityManager->AddComponent<CarrierComponent>(entity);
    cc.state = initialState;

    PathfindingComponent& pc = m_entityManager->AddComponent<PathfindingComponent>(entity);
    pc.tiles = tiles;
    pc.progress = 0.0f;
    pc.walkDir = 1.0f;

    CarrierRenderComponent& rc = m_entityManager->AddComponent<CarrierRenderComponent>(entity);
    rc.spriteIndex = -1;
    rc.animationTimer = 0.0f;

    return entity;
}

void CarrierSystem::RemoveCarrier(Entity entity) {
    m_entityManager->DestroyEntity(entity);
}

void CarrierSystem::UpdateMovement(float dt) {
    m_entityManager->ForEach<CarrierComponent, PathfindingComponent>(
        [&](Entity entity, CarrierComponent& carrier, PathfindingComponent& path)
    {
        if (carrier.state != WalkingToPost && carrier.state != Working && carrier.state != ReturningHome)
            return;

        if (path.tiles.size() < 2) return;
        float pathLen = (float)(path.tiles.size() - 1);

        path.progress += path.walkDir * CARRIER_SPEED * dt;

        if (path.progress < 0.0f) {
            path.progress = 0.0f;
            path.walkDir = 1.0f;
        } else if (path.progress > pathLen) {
            path.progress = pathLen;
            path.walkDir = 1.0f;
        }
    });
}

void CarrierSystem::UpdatePath(Entity entity, const std::vector<Vector2i>& tiles) {
    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    if (pc) {
        pc->tiles = tiles;
        pc->progress = 0.0f;
        pc->walkDir = 1.0f;
    }
}

void CarrierSystem::SyncFromCarrier(Entity entity, const Carrier* carrier) {
    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    CarrierComponent* cc = m_entityManager->GetComponent<CarrierComponent>(entity);
    if (!pc || !cc) return;

    if (carrier->state == WalkingToPost || carrier->state == ReturningHome) {
        pc->progress = carrier->transitProgress;
    } else {
        pc->progress = carrier->ep;
    }
    pc->walkDir = carrier->walkDir;
    cc->state = carrier->state;
}

void CarrierSystem::SyncToCarrier(Entity entity, Carrier* carrier) const {
    if (!carrier) return;
    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    if (!pc) return;

    if (carrier->state == WalkingToPost || carrier->state == ReturningHome) {
        carrier->transitProgress = pc->progress;
    } else {
        carrier->ep = pc->progress;
    }
    carrier->walkDir = pc->walkDir;
}

}
