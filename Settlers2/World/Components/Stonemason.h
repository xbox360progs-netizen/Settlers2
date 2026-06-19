#ifndef WORLD_COMPONENTS_STONEMASON_H
#define WORLD_COMPONENTS_STONEMASON_H

#include "WorkerBuilding.h"
#include "../Map.h"
#include "../../Logic/ResourceRegistry.h"
#include <algorithm>

namespace World {

struct LocalResource {
    Vector2i pos;
    int distSq;

    bool operator<(const LocalResource& other) const {
        return distSq < other.distSq;
    }
};

class Stonemason : public WorkerBuilding {
private:
    std::vector<LocalResource> m_localNodes;
    bool m_isCacheInitialized;
    ResourceType m_targetResourceType;

    void GoIdle() override {
        WorkerBuilding::GoIdle();
    }

    bool FindTarget() override {
        if (IsOutputFull() || !map) return false;

        if (!m_isCacheInitialized || m_localNodes.empty()) {
            InitLocalResources();
            if (m_localNodes.empty()) {
                m_isCacheInitialized = false;
                return false;
            }
            m_isCacheInitialized = true;
        }

        for (size_t i = 0; i < m_localNodes.size(); ++i) {
            const Vector2i& nodePos = m_localNodes[i].pos;
            const ResourceNode& node = map->GetResourceNode(nodePos.x, nodePos.y);

            if (node.type == m_targetResourceType && node.amount > 0) {
                m_targetPos = nodePos;
                return true;
            }
        }
        return false;
    }

    bool ValidateTarget() const override {
        if (!map) return false;
        const ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
        return node.type == m_targetResourceType && node.amount > 0;
    }

    void Produce() override {
        if (map) {
            ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
            if (node.type == m_targetResourceType && node.amount > 0) {
                node.amount--;
            }
        }
    }

    void OnArriveHome() override {
        bool ok = AddOutput(m_targetResourceType, 1);
        if (!ok) {
            m_wTimer = m_idleDuration - 1.0f;
        }
        GoIdle();
    }

public:
    Stonemason(int x, int y, uint8_t o, Map* m)
        : WorkerBuilding(BuildingType::Stonemason, x, y, o, m)
        , m_isCacheInitialized(false)
        , m_targetResourceType(ResourceType_Stone)
    {
        outputResources.push_back(ResourceType_Stone);
        outputResources.push_back(ResourceType_Marble);
        outputResources.push_back(ResourceType_Granite);

        m_idleDuration = 5.0f;
        m_workDuration = 3.0f;
        m_workerSpeed = 1.0f;
        m_searchCooldown = 5.0f;
    }

    void ChangeTargetResource(ResourceType newType) {
        if (m_targetResourceType == newType) return;

        m_targetResourceType = newType;
        m_localNodes.clear();
        m_isCacheInitialized = false;

        // Interrupt any in-progress work — worker picks up new type on next idle cycle
        GoIdle();
    }

    ResourceType GetCurrentTargetResource() const { return m_targetResourceType; }

    void SetActiveResourceMode(ResourceType type) override { ChangeTargetResource(type); }
    ResourceType GetActiveResourceMode() const override { return m_targetResourceType; }

    void InitLocalResources() {
        if (!map) return;
        Logic::ResourceRegistry* registry = map->GetResourceRegistry();
        if (!registry) return;

        m_localNodes.clear();

        const std::vector<Vector2i>& allResources = registry->GetWorldResources(m_targetResourceType);
        const int maxRadiusSq = 400;

        for (size_t i = 0; i < allResources.size(); ++i) {
            int dx = allResources[i].x - pos.x;
            int dy = allResources[i].y - pos.y;
            int distSq = dx * dx + dy * dy;

            if (distSq <= maxRadiusSq) {
                LocalResource lr;
                lr.pos = allResources[i];
                lr.distSq = distSq;
                m_localNodes.push_back(lr);
            }
        }

        std::sort(m_localNodes.begin(), m_localNodes.end());
    }

    bool GetWorkerRenderInfo(float& outX, float& outY, int& outSpriteIdx) const override {
        if (WorkerBuilding::GetWorkerRenderInfo(outX, outY, outSpriteIdx)) {
            outSpriteIdx = (m_wDir == 0) ? 20 : 21;
            return true;
        }
        return false;
    }
};

} // namespace World

#endif
