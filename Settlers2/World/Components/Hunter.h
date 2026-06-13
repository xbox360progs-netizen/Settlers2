#ifndef WORLD_COMPONENTS_HUNTER_H
#define WORLD_COMPONENTS_HUNTER_H

#include "Building.h"
#include "../Flag.h"
#include "../Map.h"
#include "../WildlifeSystem.h"
#include "../../Logic/ResourceRegistry.h"

namespace World {

class Hunter : public Building {
    enum WorkerState {
        WState_Idle,
        WState_WalkingToTarget,
        WState_Hunting,
        WState_WalkingHome
    };

    WorkerState m_wState;
    float m_wX, m_wY;       // worker float pos (node coords)
    float m_wVx, m_wVy;     // worker velocity
    float m_wTimer;          // general-purpose timer
    int   m_wDir;            // 0=SE (sprite 12), 1=SW (sprite 13)

    static const float IDLE_DURATION;
    static const float HUNT_DURATION;
    static const float WORKER_SPEED;

    void StartWalking(float dx, float dy) {
        float d = sqrtf(dx * dx + dy * dy);
        if (d < 0.5f) return;
        m_wVx = dx / d * WORKER_SPEED;
        m_wVy = dy / d * WORKER_SPEED;
        m_wDir = (m_wVx >= 0.0f) ? 0 : 1;
    }

    void StartWalkingHome() {
        if (!connectedFlag) { GoIdle(); return; }
        float dx = (float)connectedFlag->pos.x - m_wX;
        float dy = (float)connectedFlag->pos.y - m_wY;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < 0.5f) {
            // already at flag – deposit immediately
            DepositMeat();
            return;
        }
        m_wVx = dx / d * WORKER_SPEED;
        m_wVy = dy / d * WORKER_SPEED;
        m_wDir = (m_wVx >= 0.0f) ? 0 : 1;
        m_wState = WState_WalkingHome;
    }

    void FindTargetAndStart() {
        if (IsOutputFull()) return;
        Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
        if (!registry || !map) return;
        if (!connectedFlag) return;
        const std::vector<Vector2i>& spawners = registry->GetWorldResources(ResourceType_WildlifeSpawner_Deer);
        Vector2i bestPos;
        int bestDist = 999999;
        bool found = false;
        for (size_t i = 0; i < spawners.size(); ++i) {
            const ResourceNode& node = map->GetResourceNode(spawners[i].x, spawners[i].y);
            if (node.type != ResourceType_WildlifeSpawner_Deer || node.amount <= 0) continue;
            int dx = spawners[i].x - pos.x;
            int dy = spawners[i].y - pos.y;
            int dist = dx * dx + dy * dy;
            if (dist < bestDist) {
                bestDist = dist;
                bestPos = spawners[i];
                found = true;
            }
        }
        if (!found) return;
        m_target = bestPos;
        m_wX = (float)connectedFlag->pos.x;
        m_wY = (float)connectedFlag->pos.y;
        float dx = (float)m_target.x - m_wX;
        float dy = (float)m_target.y - m_wY;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < 0.5f) {
            m_wState = WState_Hunting;
            m_wTimer = 0.0f;
            return;
        }
        StartWalking(dx, dy);
        m_wState = WState_WalkingToTarget;
        m_wTimer = 0.0f;
    }

    void DepositMeat() {
        if (AddOutput(ResourceType_Meat, 1)) {
            GoIdle();
        }
        // if output full, stay at flag and retry next frame (no velocity)
    }

    void GoIdle() {
        m_wState = WState_Idle;
        m_wTimer = 0.0f;
        m_wVx = 0.0f;
        m_wVy = 0.0f;
    }

    void UpdateIdle(float dt) {
        m_wTimer += dt;
        if (m_wTimer >= IDLE_DURATION) {
            FindTargetAndStart();
        }
    }

    void UpdateWalkingToTarget(float dt) {
        m_wX += m_wVx * dt;
        m_wY += m_wVy * dt;
        float dx = (float)m_target.x - m_wX;
        float dy = (float)m_target.y - m_wY;
        if (dx * dx + dy * dy <= 0.25f) {
            m_wX = (float)m_target.x;
            m_wY = (float)m_target.y;
            m_wVx = 0.0f; m_wVy = 0.0f;
            m_wState = WState_Hunting;
            m_wTimer = 0.0f;
        }
    }

    void UpdateHunting(float dt) {
        m_wTimer += dt;
        if (m_wTimer >= HUNT_DURATION) {
            if (map) {
                ResourceNode& node = map->GetResourceNode(m_target.x, m_target.y);
                if (node.type == ResourceType_WildlifeSpawner_Deer && node.amount > 0) {
                    node.amount--;
                }
                WildlifeSystem* ws = map->GetWildlifeSystem();
                if (ws) {
                    int idx = ws->FindAliveAnimal(m_target.x, m_target.y, 12, AnimalType_Deer);
                    if (idx >= 0) ws->RemoveAnimal(idx);
                }
            }
            StartWalkingHome();
        }
    }

    void UpdateWalkingHome(float dt) {
        if (!connectedFlag) { GoIdle(); return; }
        // if output was full last frame, standing still at flag
        if (m_wVx == 0.0f && m_wVy == 0.0f) {
            DepositMeat();
            return;
        }
        m_wX += m_wVx * dt;
        m_wY += m_wVy * dt;
        float dx = (float)connectedFlag->pos.x - m_wX;
        float dy = (float)connectedFlag->pos.y - m_wY;
        if (dx * dx + dy * dy <= 0.25f) {
            m_wX = (float)connectedFlag->pos.x;
            m_wY = (float)connectedFlag->pos.y;
            m_wVx = 0.0f; m_wVy = 0.0f;
            DepositMeat();
        }
    }

public:
    Hunter(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::Hunter, x, y, o, m)
        , m_wState(WState_Idle)
        , m_wX(0.0f), m_wY(0.0f)
        , m_wVx(0.0f), m_wVy(0.0f)
        , m_wTimer(0.0f)
        , m_wDir(0)
    {
        outputResources.push_back(ResourceType_Meat);
    }

    void Update(float dt) override {
        if (state != State_Finished) return;

        switch (m_wState) {
        case WState_Idle:            UpdateIdle(dt);           break;
        case WState_WalkingToTarget: UpdateWalkingToTarget(dt); break;
        case WState_Hunting:         UpdateHunting(dt);        break;
        case WState_WalkingHome:     UpdateWalkingHome(dt);    break;
        }
    }

    bool GetWorkerRenderInfo(float& outX, float& outY, int& outSpriteIdx) const override {
        if (m_wState == WState_Idle) return false;
        outX = m_wX;
        outY = m_wY;
        outSpriteIdx = (m_wDir == 0) ? 12 : 13;  // hunter_SE / hunter_SW
        return true;
    }
};

} // namespace World

#endif
