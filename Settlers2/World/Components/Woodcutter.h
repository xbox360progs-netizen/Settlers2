#ifndef WORLD_COMPONENTS_WOODCUTTER_H
#define WORLD_COMPONENTS_WOODCUTTER_H

#include "WorkerBuilding.h"
#include "../../Logic/ResourceRegistry.h"
#include "../CargoManager.h"

namespace World {

class Woodcutter : public WorkerBuilding {
    void GoIdle() override {
        m_carriedCargo = NULL;
        m_logSpawned = false;
        m_needsFlagWalk = false;
        WorkerBuilding::GoIdle();
    }

    bool IsOutputFull() const override {
        if (!connectedFlag) return true;
        CargoManager* cm = map ? map->GetCargoManager() : NULL;
        if (!cm) return true;
        return cm->CountCargoOnFlag(connectedFlag->handle) >= FLAG_MAX_CARGO;
    }

    bool FindTarget() override {
        if (IsOutputFull()) { OutputDebugStringA("[WOODCUTTER] FindTarget: output full\n"); return false; }
        if (m_carriedCargo) { OutputDebugStringA("[WOODCUTTER] FindTarget: already carrying\n"); return false; }
        Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
        if (!registry) { OutputDebugStringA("[WOODCUTTER] FindTarget: no registry\n"); return false; }
        if (!connectedFlag) { OutputDebugStringA("[WOODCUTTER] FindTarget: no connectedFlag\n"); return false; }

        Vector2i found;
        if (registry->FindNearestWorldResource(ResourceType_Wood, pos, found)) {
            const ResourceNode& node = map->GetResourceNode(found.x, found.y);
            if (IsTree(node.type) && IsTreeMature(node.amount)) {
                m_targetPos = found;
                m_hasTarget = true;
                return true;
            } else {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[WOODCUTTER] FindTarget: tree at (%d,%d) type=%d amount=%d not valid\n",
                    found.x, found.y, node.type, node.amount);
                OutputDebugStringA(buf);
            }
        } else {
            OutputDebugStringA("[WOODCUTTER] FindTarget: no world resource found\n");
        }
        return false;
    }

    bool ValidateTarget() const override {
        if (!map || !m_hasTarget) return false;
        const ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
        return IsTree(node.type) && IsTreeAlive(node.amount);
    }

    void Update(float dt) override {
        if (m_population <= 0) return;

        // Custom idle handler
        if (m_wState == WState_Idle) {
            m_wTimer += dt;
            if (m_wTimer >= m_idleDuration) {
                m_wTimer = 0.0f;
                if (FindTarget()) {
                    m_hasTarget = true;
                    m_wState = WState_Leaving;
                    m_needsFlagWalk = true;
                    OutputDebugStringA("[WOODCUTTER] State: Idle->Leaving (walk to flag)\n");
                } else {
                    m_wTimer = m_idleDuration - m_searchCooldown;
                }
            }
            return;
        }

        // Walk from building to flag before heading to target
        if (m_wState == WState_Leaving && m_needsFlagWalk && connectedFlag) {
            float dx = (float)connectedFlag->pos.x - m_wX;
            float dy = (float)connectedFlag->pos.y - m_wY;
            float d = sqrtf(dx * dx + dy * dy);
            if (d >= 0.5f) {
                StartWalking(dx, dy);
                m_wX += m_wVx * dt;
                m_wY += m_wVy * dt;
                m_graphLogCounter++;
                if (m_graphLogCounter % 30 == 0) {
                    char buf[256];
                    _snprintf(buf, sizeof(buf),
                        "[WOODCUTTER] WalkFlag pos=(%.1f,%.1f) dir=%d\n",
                        m_wX, m_wY, m_wDir);
                    OutputDebugStringA(buf);
                }
                return;
            }
            m_wX = (float)connectedFlag->pos.x;
            m_wY = (float)connectedFlag->pos.y;
            m_wVx = 0.0f; m_wVy = 0.0f;
            m_needsFlagWalk = false;
            OutputDebugStringA("[WOODCUTTER] Reached flag, start walking to tree\n");
        }

        // Two-phase working: fell tree → ground log → pick up → carry home
        if (m_wState == WState_Working) {
            if (!m_hasTarget || !map) { GoIdle(); return; }
            m_wTimer += dt;
            m_workFrame = ((int)(m_wTimer * 4.0f)) & 1;

            if (!m_logSpawned) {
                ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
                if (!IsTree(node.type) || !IsTreeAlive(node.amount)) {
                    GoIdle();
                    return;
                }
                if (m_wTimer >= m_workDuration) {
                    node.amount = TreeState_Stump;
                    map->SetTileAsStump(m_targetPos.x, m_targetPos.y);
                    Logic::ResourceRegistry* registry = map->GetResourceRegistry();
                    if (registry)
                        registry->UnregisterWorldResource(ResourceType_Wood, m_targetPos.x, m_targetPos.y);
                    map->SpawnGroundResource(ResourceType_Wood, 1, m_targetPos.x, m_targetPos.y);
                    OutputDebugStringA("[WOODCUTTER] Tree felled, log on ground\n");
                    m_logSpawned = true;
                    m_wTimer = 0.0f;
                }
            } else if (!m_carriedCargo && m_wTimer >= 0.5f) {
                GroundResource* gr = map->FindGroundResourceAt(m_targetPos.x, m_targetPos.y);
                if (gr) {
                    CargoManager* cm = map->GetCargoManager();
                    if (cm) {
                        m_carriedCargo = cm->Allocate(ResourceType_Wood, 1, connectedFlag ? connectedFlag->handle : FlagHandle());
                        if (m_carriedCargo) {
                            m_carriedCargo->state = Cargo_Carried;
                        }
                    }
                    map->RemoveGroundResourceAt(m_targetPos.x, m_targetPos.y);
                    OutputDebugStringA("[WOODCUTTER] Picked up log from ground\n");
                }
                StartWalkingHome();
            }
            return;
        }

        {
            bool wasLeaving = (m_wState == WState_Leaving);
            if (wasLeaving) {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg),
                    "[WOODCUTTER] Pre-StartWalkingToTarget worker=(%.1f,%.1f) target=(%d,%d)\n",
                    m_wX, m_wY, m_targetPos.x, m_targetPos.y);
                OutputDebugStringA(dbg);
            }

            WorkerBuilding::Update(dt);

            if (wasLeaving) {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg),
                    "[WOODCUTTER] Post-StartWalkingToTarget worker=(%.1f,%.1f) state=%d\n",
                    m_wX, m_wY, m_wState);
                OutputDebugStringA(dbg);
            }
        }

        // Rate-limited position log when walking
        m_graphLogCounter++;
        if (m_wState == WState_WalkingToTarget || m_wState == WState_Returning) {
            if (m_graphLogCounter % 10 == 0) {
                int spriteIdx = (m_wDir == 0) ? 14 : 15;
                if (m_carriedCargo) spriteIdx += 2;
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[WOODCUTTER] Walk pos=(%.1f,%.1f) dir=%d state=%d sprite=%d cargo=%d\n",
                    m_wX, m_wY, m_wDir, m_wState, spriteIdx, m_carriedCargo ? 1 : 0);
                OutputDebugStringA(buf);
            }
        }
    }

    void Produce() override {} // handled entirely in custom Update

    void OnArriveHome() override {
        if (m_carriedCargo && connectedFlag) {
            CargoManager* cm = map ? map->GetCargoManager() : NULL;
            if (cm) {
                if (cm->CountCargoOnFlag(connectedFlag->handle) < FLAG_MAX_CARGO) {
                    connectedFlag->AcceptCargo(m_carriedCargo);
                    m_carriedCargo = NULL;
                } else {
                    return; // flag full — keep cargo and retry next OnArriveHome
                }
            } else {
                connectedFlag->AcceptCargo(m_carriedCargo);
                m_carriedCargo = NULL;
            }
        }
        GoIdle();
    }

public:
    Woodcutter(int x, int y, uint8_t o, Map* m)
        : WorkerBuilding(BuildingType::Woodcutter, x, y, o, m)
        , m_carriedCargo(NULL)
        , m_logSpawned(false)
        , m_needsFlagWalk(false)
        , m_graphLogCounter(0)
    {
        m_idleDuration = 0.5f;
        m_workDuration = 2.0f;
        m_workerSpeed = 1.0f;
        m_searchCooldown = 0.5f;
    }

    bool GetWorkerRenderInfo(float& outX, float& outY, int& outSpriteIdx) const override {
        if (WorkerBuilding::GetWorkerRenderInfo(outX, outY, outSpriteIdx)) {
            outSpriteIdx = (m_wDir == 0) ? 14 : 15;
            if (m_wState == WState_Working)
                outSpriteIdx += m_workFrame * 2;
            if (m_carriedCargo)
                outSpriteIdx += 2;
            return true;
        }
        return false;
    }

private:
    Cargo* m_carriedCargo;
    bool   m_logSpawned;
    bool   m_needsFlagWalk;
    int    m_graphLogCounter;
};

} // namespace World
#endif
