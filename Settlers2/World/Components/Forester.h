#ifndef WORLD_COMPONENTS_FORESTER_H
#define WORLD_COMPONENTS_FORESTER_H

#include "WorkerBuilding.h"
#include "../../Logic/ResourceRegistry.h"

namespace World {

class Forester : public WorkerBuilding {
    bool m_needsFlagWalk;

    void GoIdle() override {
        m_needsFlagWalk = false;
        WorkerBuilding::GoIdle();
    }

    bool FindTarget() override {
        // Search for an empty land tile in radius 6
        int bestDist = 1000;
        Vector2i bestPos(0, 0);
        bool found = false;
        int centerX = (int)pos.x;
        int centerY = (int)pos.y;

        for (int dy = -6; dy <= 6; ++dy) {
            for (int dx = -6; dx <= 6; ++dx) {
                int checkX = centerX + dx;
                int checkY = centerY + dy;
                if (checkX < 0 || checkY < 0 || checkX >= map->GetWidth() || checkY >= map->GetHeight()) continue;
                if (IsPlantableTile(checkX, checkY) && !HasNearbyTrees(checkX, checkY)) {
                    int dist = dx * dx + dy * dy;
                    if (dist < bestDist) {
                        bestDist = dist;
                        bestPos.x = checkX; bestPos.y = checkY;
                        found = true;
                    }
                }
            }
        }

        if (found) {
            m_targetPos = bestPos;
            return true;
        }
        return false;
    }

    bool ValidateTarget() const override {
        if (!map) return false;
        return IsPlantableTile(m_targetPos.x, m_targetPos.y);
    }

    void Produce() override {
        if (!map) return;
        ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
        if (node.type == ResourceType_None) {
            node.type = ResourceType_Wood;
            node.amount = TreeState_Sapling;
            Logic::ResourceRegistry* registry = map->GetResourceRegistry();
            if (registry)
                registry->RegisterWorldResource(ResourceType_Wood, m_targetPos.x, m_targetPos.y);
            char dbg[128];
            _snprintf(dbg, sizeof(dbg), "[Forester] planted Sapling at (%d,%d)\n", m_targetPos.x, m_targetPos.y);
            OutputDebugStringA(dbg);
        }
    }

    bool IsPlantableTile(int tx, int ty) const {
        if (!map) return false;
        const ResourceNode& node = map->GetResourceNode(tx, ty);
        return node.type == ResourceType_None && node.weight == Weight_Land;
    }

    bool HasNearbyTrees(int tx, int ty, int radius = 2) const {
        if (!map) return false;
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int cx = tx + dx, cy = ty + dy;
                if (cx < 0 || cy < 0) continue;
                const ResourceNode& n = map->GetResourceNode(cx, cy);
                if (IsTree(n.type) && n.amount > TreeState_Empty)
                    return true;
            }
        }
        return false;
    }

public:
    Forester(int x, int y, uint8_t o, Map* m)
        : WorkerBuilding(BuildingType::Forester, x, y, o, m)
    {
        m_idleDuration = 5.0f;
        m_workDuration = 2.0f;
        m_workerSpeed = 1.0f;
        m_needsFlagWalk = false;
    }

    void Update(float dt) override {
        if (state != State_Finished) return;
        if (m_population <= 0) return;

        if (m_wState == WState_Idle) {
            m_wTimer += dt;
            if (m_wTimer >= m_idleDuration) {
                m_wTimer = 0.0f;
                if (FindTarget()) {
                    m_hasTarget = true;
                    if (connectedFlag) {
                        float dx = (float)connectedFlag->pos.x - m_wX;
                        float dy = (float)connectedFlag->pos.y - m_wY;
                        if (sqrtf(dx*dx+dy*dy) < 0.5f) {
                            m_wState = WState_Leaving;
                        } else {
                            StartWalking(dx, dy);
                            m_needsFlagWalk = true;
                            m_wState = WState_Leaving;
                        }
                    } else {
                        m_wState = WState_Leaving;
                    }
                } else {
                    m_wTimer = m_idleDuration - m_searchCooldown;
                }
            }
            return;
        }

        // Walk from building to flag before heading to target
        if (m_wState == WState_Leaving && m_needsFlagWalk && connectedFlag) {
            m_wX += m_wVx * dt;
            m_wY += m_wVy * dt;
            float dx = (float)connectedFlag->pos.x - m_wX;
            float dy = (float)connectedFlag->pos.y - m_wY;
            if (dx * dx + dy * dy <= 0.25f) {
                m_wX = (float)connectedFlag->pos.x;
                m_wY = (float)connectedFlag->pos.y;
                m_wVx = 0.0f; m_wVy = 0.0f;
                m_needsFlagWalk = false;
            } else {
                return;
            }
        }

        WorkerBuilding::Update(dt);
    }

    bool GetWorkerRenderInfo(float& outX, float& outY, int& outSpriteIdx) const override {
        if (WorkerBuilding::GetWorkerRenderInfo(outX, outY, outSpriteIdx)) {
            outSpriteIdx = (m_wDir == 0) ? 18 : 19;
            if (m_wState == WState_Working)
                outSpriteIdx += m_workFrame * 2;
            return true;
        }
        return false;
    }
};

} // namespace World
#endif
