#ifndef WORLD_COMPONENTS_COALMINE_H
#define WORLD_COMPONENTS_COALMINE_H

#include "Building.h"
#include "../Map.h"

namespace World {

class CoalMine : public Building {
    bool m_hasDeposit;
    Vector2i m_depositPos;
public:
    CoalMine(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::CoalMine, x, y, o, m), m_hasDeposit(false) {
        m_depositPos.x = 0;
        m_depositPos.y = 0;
        outputResources.push_back(ResourceType_Coal);
    }

    void Update() override {
        if (!m_hasDeposit) {
            m_hasDeposit = map && map->FindTileTypeInRadius(pos.x, pos.y, 2, Objects, Mountain, m_depositPos.x, m_depositPos.y);
        }
        if (m_hasDeposit) {
            int foodBonus = ConsumeFood();
            if (foodBonus > 0) {
                m_storage[ResourceType_Coal] += foodBonus;
            }
        }
    }
};

} // namespace World

#endif
