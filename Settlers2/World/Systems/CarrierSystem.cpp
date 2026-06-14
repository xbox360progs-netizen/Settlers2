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

Entity CarrierSystem::CreateCarrier(const CarrierInit& init) {
    Entity entity = m_entityManager->CreateEntity();

    CarrierComponent& cc = m_entityManager->AddComponent<CarrierComponent>(entity);
    cc.state = init.initialState;
    cc.hasPickedUp = false;
    cc.cargoDelivered = false;

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

void CarrierSystem::UpdateMovement(float dt) {
    m_entityManager->ForEach<CarrierComponent, PathfindingComponent>(
        [&](Entity entity, CarrierComponent& carrier, PathfindingComponent& path)
    {
		char dbg[256];
        _snprintf(dbg, sizeof(dbg),
            "[ECS Move] e=%u state=%d prog=%.2f tiles=%d pickEp=%.2f destEp=%.2f ver=%u\n",
            entity, carrier.state, path.progress, (int)path.tiles.size(),
            path.pickupEp, path.destEp, path.pathVersion);
        OutputDebugStringA(dbg);

        if (!IsTransitState(carrier.state) && carrier.state != Working)
            return;

        if (!IsValidPath(path)) return;

        if (IsTransitState(carrier.state)) {
            // Transit states: simple progress with state transitions
            path.progress += path.walkDir * CARRIER_SPEED * dt;
            float pathLen = (float)(path.tiles.size() - 1);
            if (path.progress < 0.0f) {
                path.progress = 0.0f;
                path.walkDir = 1.0f;
            } else if (path.progress >= pathLen) {
                path.progress = pathLen;
                if (carrier.state == WalkingToPost) {
                    carrier.state = Working;
                } else if (carrier.state == ReturningHome) {
                    carrier.readyToRemove = true;
                }
            }
        } else {
            UpdateWorking(entity, path, carrier, dt);
        }
    });
}

void CarrierSystem::DestroyEntity(Entity entity) {
    RemoveCarrier(entity);
}

void CarrierSystem::UpdateWorking(Entity entity, PathfindingComponent& path, CarrierComponent& carrier, float dt) {
    if (carrier.currentJob != 0 && !carrier.cargoDelivered) {
        MoveToTarget(path, carrier, dt);
    } else {
        MoveToCenter(path, carrier, dt);
    }
}

void CarrierSystem::MoveToTarget(PathfindingComponent& path, CarrierComponent& carrier, float dt) {
    float targetEp = carrier.hasPickedUp ? path.destEp : path.pickupEp;
//    if (targetEp <= 0.0f && !carrier.hasPickedUp) return;

    if (fabsf(path.progress - targetEp) > 0.01f) {
        path.walkDir = (targetEp >= path.progress) ? 1.0f : -1.0f;
        path.progress += path.walkDir * CARRIER_SPEED * dt;

        if ((path.walkDir > 0.0f && path.progress >= targetEp) || 
            (path.walkDir < 0.0f && path.progress <= targetEp)) {
            path.progress = targetEp;
        }
    }
}

void CarrierSystem::MoveToCenter(PathfindingComponent& path, CarrierComponent& carrier, float dt) {
     if (!IsValidPath(path)) return;

    float pathLen = (float)(path.tiles.size() - 1);
    float centerEp = pathLen * 0.5f;

    if (fabsf(path.progress - centerEp) > 0.1f) {
        path.walkDir = (centerEp >= path.progress) ? 1.0f : -1.0f;
        path.progress += path.walkDir * CARRIER_SPEED * dt;

        if ((path.walkDir > 0.0f && path.progress >= centerEp) || 
            (path.walkDir < 0.0f && path.progress <= centerEp)) {
            path.progress = centerEp;
        }
    }
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
    cc->hasPickedUp = carrier->hasPickedUp;
    cc->cargoDelivered = carrier->cargoDelivered;
}

void CarrierSystem::SyncToCarrier(Entity entity, Carrier* carrier) const {
    if (!carrier) return;
    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    CarrierComponent* cc = m_entityManager->GetComponent<CarrierComponent>(entity);
    if (!pc || !cc) return;

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

    carrier->hasPickedUp = cc->hasPickedUp;
    carrier->cargoDelivered = cc->cargoDelivered;
    carrier->readyToRemove = cc->readyToRemove;
}

void CarrierSystem::SyncLegTargets(Entity entity, const Carrier* carrier) {


    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    CarrierComponent* cc = m_entityManager->GetComponent<CarrierComponent>(entity);

	char dbg[256];
_snprintf(dbg, sizeof(dbg),
    "[SyncLeg] e=%u carrier.ver=%u ecs.ver=%u pickEp=%.2f destEp=%.2f\n",
    entity, carrier->pathVersion, pc->pathVersion,
    carrier->m_resolvedLegFrom ? 1.0f : 0.0f,
    carrier->m_resolvedLegTo ? 1.0f : 0.0f);
OutputDebugStringA(dbg);

    if (!pc || !cc) return;

    cc->state = carrier->state;
    cc->hasPickedUp = carrier->hasPickedUp;
    cc->cargoDelivered = carrier->cargoDelivered;
    cc->currentJob = carrier->HasJob() ? 1 : 0;
    cc->readyToRemove = carrier->readyToRemove;
    pc->pathVersion = carrier->pathVersion;

    if (carrier->state == Working) {
        if (carrier->road && carrier->road->tiles.size() >= 2) {
            pc->tiles = carrier->road->tiles;
        }
        pc->progress = carrier->ep;
        pc->walkDir = carrier->walkDir;
        if (carrier->m_resolvedLegFrom && carrier->m_resolvedLegTo) {
            pc->pickupEp = carrier->GetFlagEp(carrier->m_resolvedLegFrom);
            pc->destEp = carrier->GetFlagEp(carrier->m_resolvedLegTo);
        }
    } else if (IsTransitState(carrier->state) &&
               IsValidPath(carrier->transitTiles)) {
        pc->tiles = carrier->transitTiles;
        pc->progress = carrier->transitProgress;
    }
}

void CarrierSystem::DebugECSInvariants(Entity entity, const Carrier* carrier) const {
#ifdef _DEBUG
    PathfindingComponent* pc = m_entityManager->GetComponent<PathfindingComponent>(entity);
    CarrierComponent* cc = m_entityManager->GetComponent<CarrierComponent>(entity);
    if (!pc || !cc) return;

    if (carrier->state == Working || IsTransitState(carrier->state)) {
        ASSERT(!pc->tiles.empty());
        ASSERT(pc->progress >= -0.01f);
        ASSERT(pc->walkDir == 1.0f || pc->walkDir == -1.0f);
        ASSERT(pc->pathVersion == carrier->pathVersion);

        float pathLen = (float)(pc->tiles.size() - 1);
        if (pc->progress > pathLen + 1.0f) {
            char buf[128];
            _snprintf(buf, sizeof(buf), "[ECS] Warning: pc->progress=%.2f > pathLen=%.2f+1 (state=%d)\n",
                pc->progress, pathLen, (int)carrier->state);
            OutputDebugStringA(buf);
        }

        // Tile origin: transit paths must not contain road tiles
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
                ASSERT(err < 0.25f);
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
