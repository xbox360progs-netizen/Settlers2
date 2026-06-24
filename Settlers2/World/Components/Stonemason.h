#ifndef WORLD_COMPONENTS_STONEMASON_H
#define WORLD_COMPONENTS_STONEMASON_H

#include "WorkerBuilding.h"
#include "../Map.h"
#include "../../Logic/ResourceRegistry.h"

namespace World {

struct LocalResource {
    Vector2i pos;
    int distSq;
};

class Stonemason : public WorkerBuilding {
private:
    static const int MAX_LOCAL_NODES = 64;
    LocalResource m_localNodes[MAX_LOCAL_NODES];
    int m_localNodesCount;
    bool m_isCacheInitialized;
    ResourceType m_targetResourceType;

    bool m_hasClaimedSpot;
    bool m_hasWorkSite;

    void UnclaimSpot() {
        if (!m_hasClaimedSpot || !map) return;
        ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
        if (node.type == m_targetResourceType)
            node.amount++;
        m_hasClaimedSpot = false;
    }

    void GoIdle() override {
        UnclaimSpot();
        WorkerBuilding::GoIdle();
    }

    bool FindTarget() override {
        if (IsOutputFull() || !map) return false;

        if (!m_isCacheInitialized || m_localNodesCount == 0) {
            InitLocalResources();
            if (m_localNodesCount == 0) {
                m_isCacheInitialized = false;
                return false;
            }
            m_isCacheInitialized = true;
        }

        for (int i = 0; i < m_localNodesCount; ++i) {
            const Vector2i& nodePos = m_localNodes[i].pos;
            ResourceNode& node = map->GetResourceNode(nodePos.x, nodePos.y);

            if (node.type == m_targetResourceType && node.amount > 0 && node.surveyed) {
                m_targetPos = nodePos;
                node.amount--;
                m_hasClaimedSpot = true;
                return true;
            }
        }

        // All local nodes depleted
        m_isDepleted = true;
        return false;
    }

    bool ValidateTarget() const override {
        if (!map || !m_hasClaimedSpot) return false;
        const ResourceNode& node = map->GetResourceNode(m_targetPos.x, m_targetPos.y);
        return node.type == m_targetResourceType;
    }

    void Produce() override {
        m_hasClaimedSpot = false;
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
        , m_localNodesCount(0)
        , m_isCacheInitialized(false)
        , m_targetResourceType(ResourceType_Stone)
        , m_hasClaimedSpot(false)
        , m_hasWorkSite(false)
    {
        outputResources.push_back(ResourceType_Stone);
        outputResources.push_back(ResourceType_Marble);
        outputResources.push_back(ResourceType_Granite);

        m_idleDuration = 5.0f;
        m_workDuration = 3.0f;
        m_workerSpeed = 1.0f;
        m_searchCooldown = 5.0f;
    }

    void Update(float dt) override {
        WorkerState prevState = m_wState;
        WorkerBuilding::Update(dt);
        // When first entering Working state, place work-site at the resource node
        if (m_wState == WState_Working && prevState != WState_Working && m_hasTarget) {
            m_hasWorkSite = true;
        }
    }

    void ChangeTargetResource(ResourceType newType) {
        if (m_targetResourceType == newType) return;

        m_targetResourceType = newType;
        m_localNodesCount = 0;
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

        m_localNodesCount = 0;

        const std::vector<Vector2i>& allResources = registry->GetWorldResources(m_targetResourceType);
        const int maxRadiusSq = 400;

        for (size_t i = 0; i < allResources.size() && m_localNodesCount < MAX_LOCAL_NODES; ++i) {
            int dx = allResources[i].x - pos.x;
            int dy = allResources[i].y - pos.y;
            int distSq = dx * dx + dy * dy;

            if (distSq <= maxRadiusSq) {
                LocalResource& lr = m_localNodes[m_localNodesCount++];
                lr.pos = allResources[i];
                lr.distSq = distSq;
            }
        }

        // Insertion sort — fast on small arrays, no indirection
        for (int i = 1; i < m_localNodesCount; ++i) {
            LocalResource key = m_localNodes[i];
            int j = i - 1;
            while (j >= 0 && m_localNodes[j].distSq > key.distSq) {
                m_localNodes[j + 1] = m_localNodes[j];
                --j;
            }
            m_localNodes[j + 1] = key;
        }
    }

    bool GetWorkerRenderInfo(float& outX, float& outY, int& outSpriteIdx) const override {
        if (WorkerBuilding::GetWorkerRenderInfo(outX, outY, outSpriteIdx)) {
            // Base sprite pair 20/21 = stone miner, 22/23 = marble, 24/25 = granite
            int resOffset = 0;
            if (m_targetResourceType == ResourceType_Marble) resOffset = 2;
            else if (m_targetResourceType == ResourceType_Granite) resOffset = 4;
            outSpriteIdx = (m_wDir == 0) ? (20 + resOffset) : (21 + resOffset);
            return true;
        }
        return false;
    }

    bool GetWorkSiteRenderInfo(Vector2i& outPosition, const char*& outSpriteName) const override {
        if (m_hasWorkSite && m_hasTarget) {
            outPosition = m_targetPos;
            outSpriteName = m_isDepleted ? "mine_ruin_stone_marble" : "mine_stone_framework";
            return true;
        }
        return false;
    }
};

} // namespace World

#endif
