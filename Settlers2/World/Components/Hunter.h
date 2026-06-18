#ifndef WORLD_COMPONENTS_HUNTER_H
#define WORLD_COMPONENTS_HUNTER_H

#include "WorkerBuilding.h"
#include "../WildlifeSystem.h"
#include "../Entity.h"
#include "../../Logic/ResourceRegistry.h"

namespace World {

class Hunter : public WorkerBuilding {
    void GoIdle() override {
        WorkerBuilding::GoIdle();
    }

    bool FindTarget() override {
        if (IsOutputFull()) return false;
        Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
        if (!registry || !map || !connectedFlag) return false;

        const std::vector<Vector2i>& spawners = registry->GetWorldResources(ResourceType_WildlifeSpawner_Deer);
        Vector2i bestPos;
        int bestDist = 999999;
        bool found = false;
        for (size_t i = 0; i < spawners.size(); ++i) {
            const ResourceNode& node = map->GetResourceNode(spawners[i].x, spawners[i].y);
            if ((node.type != ResourceType_WildlifeSpawner_Deer && node.type != ResourceType_Meat) || node.amount <= 0) continue;
            int dx = spawners[i].x - pos.x;
            int dy = spawners[i].y - pos.y;
            int dist = dx * dx + dy * dy;
            if (dist < bestDist) {
                bestDist = dist;
                bestPos = spawners[i];
                found = true;
            }
        }

        if (found) {
            m_targetPos = bestPos;
            return true;
        }
        // Retry after cooldown signalled via return false
        return false;
    }

    bool ValidateTarget() const override {
        if (!map) return false;
        const ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
        return (node.type == ResourceType_WildlifeSpawner_Deer || node.type == ResourceType_Meat) && node.amount > 0;
    }

    void Produce() override {
        if (map) {
            ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
            if ((node.type == ResourceType_WildlifeSpawner_Deer || node.type == ResourceType_Meat) && node.amount > 0) {
                node.amount--;
            }
            WildlifeSystem* ws = map->GetWildlifeSystem();
            if (ws) {
                Entity entity = ws->FindAliveAnimal(m_targetPos.x, m_targetPos.y, 12, AnimalType_Deer);
                if (entity != INVALID_ENTITY) ws->RemoveAnimal(entity);
            }
        }
    }

    void OnArriveHome() override {
        bool ok = AddOutput(ResourceType_Meat, 1);
        if (!ok) {
            m_wTimer = m_idleDuration - 1.0f;
        }
        GoIdle();
    }

public:
    Hunter(int x, int y, uint8_t o, Map* m)
        : WorkerBuilding(BuildingType::Hunter, x, y, o, m)
    {
        outputResources.push_back(ResourceType_Meat);
        m_idleDuration = 5.0f;
        m_workDuration = 3.0f;
        m_workerSpeed = 1.0f;
        m_searchCooldown = 5.0f;
    }

    bool GetWorkerRenderInfo(float& outX, float& outY, int& outSpriteIdx) const override {
        if (WorkerBuilding::GetWorkerRenderInfo(outX, outY, outSpriteIdx)) {
            outSpriteIdx = (m_wDir == 0) ? 12 : 13;
            return true;
        }
        return false;
    }
};

} // namespace World

#endif
