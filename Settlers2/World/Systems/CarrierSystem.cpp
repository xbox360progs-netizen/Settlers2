#include "stdafx.h"
#include <cassert>
#include "CarrierSystem.h"
#include "../EntityManager.h"

namespace World {

CarrierSystem::CarrierSystem(EntityManager* entityManager)
    : m_entityManager(entityManager)
{
}

CarrierSystem::~CarrierSystem() {
}

Entity CarrierSystem::CreateCarrier(const CarrierInit& init) {
    Entity entity = m_entityManager->CreateEntity();

    CarrierComponent& cc = m_entityManager->AddComponent<CarrierComponent>(entity);
    cc.state = init.initialState;

    PathfindingComponent& pc = m_entityManager->AddComponent<PathfindingComponent>(entity);
    pc.tiles = init.tiles;
    pc.progress = 0.0f;
    pc.walkDir = 1.0f;
    pc.state = init.initialState;
    pc.pickupEp = 0.0f;
    pc.destEp = 0.0f;
    pc.pathVersion = init.pathVersion;

    CarrierRenderComponent& rc = m_entityManager->AddComponent<CarrierRenderComponent>(entity);
    rc.spriteIndex = -1;
    rc.animationTimer = 0.0f;

    return entity;
}

void CarrierSystem::RemoveCarrier(Entity entity) {
    m_entityManager->DestroyEntity(entity);
}

void CarrierSystem::DestroyEntity(Entity entity) {
    RemoveCarrier(entity);
}

void CarrierSystem::UpdatePath(Entity entity, const std::vector<Vector2i>& tiles) {
    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    if (pc) {
        pc->tiles = tiles;
        pc->progress = 0.0f;
        pc->walkDir = 1.0f;
        pc->pickupEp = 0.0f;
        pc->destEp = 0.0f;
    }
}

void CarrierSystem::SyncFromCarrier(Entity entity, const Carrier* carrier) {
    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    CarrierComponent* cc = m_entityManager->GetComponent<CarrierComponent>(entity);
    if (!pc || !cc) return;

    if (IsTransitState(carrier->state)) {
        pc->progress = carrier->transitProgress;
    } else {
        pc->progress = carrier->ep;
    }
    pc->walkDir = carrier->walkDir;
    pc->state = carrier->state;
    pc->pickupEp = 0.0f;
    pc->destEp = 0.0f;

    cc->state = carrier->state;
}

void CarrierSystem::SyncToCarrier(Entity entity, Carrier* carrier) const {
    if (!carrier) return;
    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    CarrierComponent* cc = m_entityManager->GetComponent<CarrierComponent>(entity);
    if (!pc || !cc) return;

    // Legacy carriers manage their own movement — ECS is a render-side copy only
    if (carrier->m_authority == MovementAuthority::Legacy)
        return;

    CarrierState prevState = carrier->state;
    bool transitCompleted = (prevState == WalkingToPost && cc->state == Working);

    if (transitCompleted) {
        if (carrier->m_roadEndpointA && carrier->m_roadEndpointB && pc->tiles.size() >= 2) {
            const Vector2i& lastTile = pc->tiles.back();
            if (carrier->m_roadEndpointA->pos.x == lastTile.x && carrier->m_roadEndpointA->pos.y == lastTile.y) {
                carrier->ep = 0.0f;
                carrier->walkDir = 1.0f;
            } else {
                carrier->ep = carrier->GetPathLen();
                carrier->walkDir = -1.0f;
            }
        }
        carrier->state = cc->state;
    } else {
        if (IsTransitState(cc->state)) {
            carrier->transitProgress = pc->progress;
        } else {
            carrier->ep = pc->progress;
        }
        carrier->state = cc->state;
        carrier->walkDir = pc->walkDir;
    }

    carrier->readyToRemove = cc->readyToRemove;
}

void CarrierSystem::SyncLegTargets(Entity entity, const Carrier* carrier) {
    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    CarrierComponent* cc = m_entityManager->GetComponent<CarrierComponent>(entity);

    if (!pc || !cc) return;

    cc->state = carrier->state;
    cc->readyToRemove = carrier->readyToRemove;

    if (carrier->state == Working) {
        if (carrier->road && carrier->road->tiles.size() >= 2) {
            pc->tiles = carrier->road->tiles;
        }
        pc->progress = carrier->ep;
        pc->walkDir = carrier->walkDir;
        pc->pathVersion = carrier->pathVersion;
    } else if (IsTransitState(carrier->state) &&
               IsValidPath(carrier->transitTiles)) {
        pc->tiles = carrier->transitTiles;
        if (pc->pathVersion != carrier->pathVersion) {
            pc->progress = carrier->transitProgress;
            pc->pathVersion = carrier->pathVersion;
        }
    }
}

void CarrierSystem::DebugECSInvariants(Entity entity, const Carrier* carrier) const {
#ifdef _DEBUG
    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    CarrierComponent* cc = m_entityManager->GetComponent<CarrierComponent>(entity);
    if (!pc || !cc) return;

    if (carrier->state == Working || IsTransitState(carrier->state)) {
        assert(!pc->tiles.empty());
        assert(pc->progress >= -0.01f);
        assert(pc->walkDir == 1.0f || pc->walkDir == -1.0f);
        assert(pc->pathVersion == carrier->pathVersion);

        float pathLen = (float)(pc->tiles.size() - 1);
        if (pc->progress > pathLen + 1.0f) {
            char buf[128];
            _snprintf(buf, sizeof(buf), "[ECS] Warning: pc->progress=%.2f > pathLen=%.2f+1 (state=%d)\n",
                pc->progress, pathLen, (int)carrier->state);
            OutputDebugStringA(buf);
        }

        if (IsTransitState(carrier->state)) {
            if (carrier->road && carrier->road->tiles.size() >= 2 &&
                pc->tiles.size() == carrier->road->tiles.size() &&
                pc->tiles[0].x == carrier->road->tiles[0].x)
            {
                OutputDebugStringA("[ECS] Warning: transit carrier has road tiles (should be transit path)\n");
            }
        } else if (carrier->state == Working && carrier->road && carrier->road->tiles.size() >= 2) {
            if (pc->tiles.size() != carrier->road->tiles.size()) {
                OutputDebugStringA("[ECS] Warning: Working carrier lacks road tiles\n");
            }
        }
    }

    // EP invariant: carrier->ep must match ECS progress (normal Working frames)
    if (carrier->state == Working) {
        float roadPathLen = carrier->GetPathLen();
        bool onTransitionFrame = (roadPathLen > 0.0f && pc && pc->progress > roadPathLen + 0.5f);
        if (!onTransitionFrame && roadPathLen > 0.0f && pc) {
            float err = fabsf(carrier->ep - ComputeCarrierEP(*pc));
            if (err >= 0.25f) {
                char buf[128];
                _snprintf(buf, sizeof(buf), "[ECS] ERROR: carrier->ep=%.2f != ComputeCarrierEP=%.2f (err=%.2f)\n",
                    carrier->ep, ComputeCarrierEP(*pc), err);
                OutputDebugStringA(buf);
                assert(err < 0.25f);
            }
        }
    }
#else
    (void)entity;
    (void)carrier;
#endif
}

float CarrierSystem::ComputeCarrierEP(const PathfindingComponent& pc) {
    return pc.progress;
}

}
