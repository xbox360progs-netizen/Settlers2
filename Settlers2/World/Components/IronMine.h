#ifndef WORLD_COMPONENTS_IRONMINE_H
#define WORLD_COMPONENTS_IRONMINE_H

#include "Building.h"
#include "../Map.h"

namespace World {

class IronMine : public Building {
    bool m_hasDeposit;
    Vector2i m_depositPos;
public:
    IronMine(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::IronMine, x, y, o, m), m_hasDeposit(false) {
        m_depositPos.x = 0;
        m_depositPos.y = 0;
        m_productionInterval = 4.0f;
        outputResources.push_back(ResourceType_IronOre);
    }

    bool CanProduce() override {
        if (!m_hasDeposit)
            m_hasDeposit = map && map->FindTileTypeInRadius(pos.x, pos.y, 2, Objects, Mountain, m_depositPos.x, m_depositPos.y);
        if (!m_hasDeposit) return false;
        int food = ConsumeFood();
        return food > 0 && !IsOutputFull();
    }

    bool ProduceOne() override {
        return AddOutput(ResourceType_IronOre, 1);
    }
};

} // namespace World

#endif
