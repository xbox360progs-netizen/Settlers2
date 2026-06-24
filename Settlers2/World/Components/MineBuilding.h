#ifndef WORLD_COMPONENTS_MINEBUILDING_H
#define WORLD_COMPONENTS_MINEBUILDING_H

#include "Building.h"
#include "../Map.h"

namespace World {

class MineBuilding : public Building {
protected:
    bool m_hasDeposit;
    Vector2i m_depositPos;

    bool FindDeposit() {
        if (m_hasDeposit) return true;
        m_hasDeposit = map && map->FindTileTypeInRadius(pos.x, pos.y, 2, Objects, Mountain, m_depositPos.x, m_depositPos.y);
        return m_hasDeposit;
    }

public:
    MineBuilding(BuildingType t, int x, int y, uint8_t o, Map* m, ResourceType outputRes)
        : Building(t, x, y, o, m), m_hasDeposit(false) {
        m_depositPos.x = 0;
        m_depositPos.y = 0;
        m_productionInterval = 4.0f;
        outputResources.push_back(outputRes);
        inputResources.push_back(ResourceType_Bread);
    }

    bool CanProduce() override {
        if (m_isDepleted) return false;
        if (!FindDeposit()) return false;
        if (map && map->GetResourceNode(m_depositPos.x, m_depositPos.y).amount <= 0) return false;
        // Check food availability (actual consumption happens in ProduceOne)
        bool hasFood = HasStorage(ResourceType_Bread)
                    || HasStorage(ResourceType_Meat)
                    || HasStorage(ResourceType_Fish);
        return hasFood && !IsOutputFull();
    }

    bool ProduceOne() override {
        if (map && m_hasDeposit) {
            World::ResourceNode& node = map->GetResourceNode(m_depositPos.x, m_depositPos.y);
            if (node.amount <= 0) {
                m_isDepleted = true;
                return false; // node exhausted — stop without producing
            }
            node.amount--;
            if (node.amount <= 0) m_isDepleted = true;
        }
        // Consume food only on successful production (moved from CanProduce)
        ConsumeFood();
        return AddOutput(outputResources[0], 1);
    }
};

} // namespace World

#endif
