#ifndef WORLD_COMPONENTS_FISHER_H
#define WORLD_COMPONENTS_FISHER_H

#include "Building.h"
#include "../Map.h"
#include "../../Logic/ResourceRegistry.h"

namespace World {

class Fisher : public Building {
    enum FishState {
        Fish_Idle,
        Fish_Fishing
    };

    FishState m_fishState;
    float m_fishTimer;
    Vector2i m_waterTarget;
    bool m_hasWaterTarget;

    static const float IDLE_DURATION;
    static const float FISHING_DURATION;

    void FindNearestWaterWithFish() {
        if (!map) return;

        m_hasWaterTarget = false;
        int bestDist = 999999;

        for (int dy = -5; dy <= 5; ++dy) {
            for (int dx = -5; dx <= 5; ++dx) {
                int checkX = pos.x + dx;
                int checkY = pos.y + dy;

                if (checkX < 0 || checkY < 0) continue;

                BYTE weight = map->GetNodeWeight(checkX, checkY);
                if (weight == Weight_Deep || weight == Weight_Shallow) {
                    const ResourceNode& node = map->GetResourceNode(checkX, checkY);
                    if (node.type == ResourceType_Fish && node.amount > 0) {
                        int dist = dx * dx + dy * dy;
                        if (dist < bestDist) {
                            bestDist = dist;
                            m_waterTarget.x = checkX;
                            m_waterTarget.y = checkY;
                            m_hasWaterTarget = true;
                        }
                    }
                }
            }
        }
    }

public:
    Fisher(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::Fisher, x, y, o, m)
        , m_fishState(Fish_Idle)
        , m_fishTimer(0.0f)
        , m_hasWaterTarget(false)
    {
        m_productionInterval = 4.0f;
        outputResources.push_back(ResourceType_Fish);
        FindNearestWaterWithFish();
    }

    bool CanProduce() override {
        if (!m_hasWaterTarget) FindNearestWaterWithFish();
        return m_hasWaterTarget && !IsOutputFull();
    }

    bool ProduceOne() override {
        if (map && m_hasWaterTarget) {
            ResourceNode& node = map->GetResourceNode(m_waterTarget.x, m_waterTarget.y);
            if (node.type == ResourceType_Fish && node.amount > 0) {
                node.amount--;
            }
        }
        return AddOutput(ResourceType_Fish, 1);
    }

    void Update(float dt) override {
        if (state != State_Finished) return;

        switch (m_fishState) {
        case Fish_Idle:
            m_fishTimer += dt;
            if (m_fishTimer >= IDLE_DURATION) {
                if (CanProduce()) {
                    m_fishState = Fish_Fishing;
                    m_fishTimer = 0.0f;
                } else {
                    FindNearestWaterWithFish();
                    m_fishTimer = 0.0f;
                }
            }
            break;

        case Fish_Fishing:
            m_fishTimer += dt;
            if (m_fishTimer >= FISHING_DURATION) {
                ProduceOne();
                m_fishState = Fish_Idle;
                m_fishTimer = 0.0f;
            }
            break;
        }
    }

    bool GetWorkerRenderInfo(float& outX, float& outY, int& outSpriteIdx) const override {
        if (m_fishState == Fish_Fishing) {
            outX = (float)pos.x;
            outY = (float)pos.y;
            outSpriteIdx = 16;
            return true;
        }
        return false;
    }
};

} // namespace World

#endif
