#ifndef WORLD_COMPONENTS_FISHER_H
#define WORLD_COMPONENTS_FISHER_H

#include "WorkerBuilding.h"

namespace World {

class Fisher : public WorkerBuilding {
    bool m_hasFishingSpot;

    void UnreserveSpot() {
        if (!m_hasFishingSpot || !map) return;
        ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
        if (node.type == ResourceType_Fish)
            node.amount++;
        m_hasFishingSpot = false;
    }

    void GoIdle() override {
        UnreserveSpot();
        WorkerBuilding::GoIdle();
        if (connectedFlag) {
            m_wX = (float)connectedFlag->pos.x;
            m_wY = (float)connectedFlag->pos.y;
        }
    }

    bool FindTarget() override {
        if (IsOutputFull()) return false;
        if (!map || !connectedFlag) return false;

        int bestDist = 999999;
        Vector2i bestPos(0, 0);
        bool found = false;

        for (int dy = -8; dy <= 8; ++dy) {
            for (int dx = -8; dx <= 8; ++dx) {
                int checkX = pos.x + dx;
                int checkY = pos.y + dy;
                if (checkX < 0 || checkY < 0) continue;
                uint8_t weight = map->GetNodeWeight(checkX, checkY);
                if (weight == Weight_Deep || weight == Weight_Shallow) {
                    ResourceNode& node = map->GetResourceNode(checkX, checkY);
                    if (node.type == ResourceType_Fish && node.amount > 0) {
                        int dist = dx * dx + dy * dy;
                        if (dist < bestDist) {
                            bestDist = dist;
                            bestPos.x = checkX; bestPos.y = checkY;
                            found = true;
                        }
                    }
                }
            }
        }
        if (found) {
            m_targetPos = bestPos;
            // Reserve: decrement so other fishers avoid it
            map->GetResourceNode(m_targetPos.x, m_targetPos.y).amount--;
            m_hasFishingSpot = true;
            return true;
        }
        return false;
    }

    bool ValidateTarget() const override {
        if (!map || !m_hasFishingSpot) return false;
        const ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
        return node.type == ResourceType_Fish;
    }

    void Produce() override {
        // Fish already decremented in FindTarget — spot consumed
        m_hasFishingSpot = false;
    }

    void OnArriveHome() override {
        bool ok = AddOutput(ResourceType_Fish, 1);
        if (!ok) {
            // Output full — try again later (re-enter idle with shorter timer)
            m_wTimer = WorkerBuilding::m_idleDuration - 1.0f;
        }
        GoIdle();
    }

public:
    Fisher(int x, int y, uint8_t o, Map* m)
        : WorkerBuilding(BuildingType::Fisher, x, y, o, m)
        , m_hasFishingSpot(false)
    {
        outputResources.push_back(ResourceType_Fish);
        m_idleDuration = 5.0f;
        m_workDuration = 3.0f;
        m_workerSpeed = 1.0f;
        m_searchCooldown = 5.0f;
    }

    void AddFish(int amount) {
        if (!map || !m_hasFishingSpot) return;
        ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
        if (node.type == ResourceType_Fish)
            node.amount += amount;
    }

    bool GetWorkerRenderInfo(float& outX, float& outY, int& outSpriteIdx) const override {
        if (WorkerBuilding::GetWorkerRenderInfo(outX, outY, outSpriteIdx)) {
            outSpriteIdx = (m_wDir == 0) ? 16 : 17;
            if (m_wState == WState_Working)
                outSpriteIdx += m_workFrame * 2;
            return true;
        }
        return false;
    }
};

} // namespace World

#endif