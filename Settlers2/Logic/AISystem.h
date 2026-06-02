#pragma once
#include <vector>
#include "../World/Map.h"
#include "../World/Components/Building.h"
#include "../Logic/EconomyManager.h"

namespace Logic {

    class AISystem {
    public:
        AISystem(uint8_t playerID, World::Map* map, EconomyManager* economy)
            : m_playerID(playerID), m_map(map), m_economy(economy) {}

        void Update(float deltaTime);

    private:
        uint8_t m_playerID;
        World::Map* m_map;
        EconomyManager* m_economy;
        
        void BuildIfMissing(World::BuildingType type);
        bool HasBuilding(World::BuildingType type);
    };
}
