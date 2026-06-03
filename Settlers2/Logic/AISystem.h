#pragma once
#include <vector>
#include "../World/Map.h"
#include "../World/Components/Building.h"
#include "../Logic/EconomyManager.h"

namespace Logic {

    struct BuildRequest
    {
        World::BuildingType type;
        int x, y;
    };

    class AISystem {
    public:
        AISystem(uint8_t playerID, World::Map* map, EconomyManager* economy);
        ~AISystem();

        void Update(float deltaTime);

        bool PlanBuild(World::BuildingType type, BuildRequest& outReq);
        void ApplyBuildRequests(const BuildRequest* requests, int numRequests);
        void ClearReservations();

    private:
        static const int MAX_BUILDING_TYPE = 22; // ToolWorkshop = 21

        struct BuildSite { short x, y; };

        uint8_t m_playerID;
        World::Map* m_map;
        EconomyManager* m_economy;

        World::Building* CreateBuilding(World::BuildingType type, int x, int y);
        void ApplyBuild(const BuildRequest& req);
        bool HasBuilding(World::BuildingType type);
        bool ReserveTile(int x, int y);

        LONG* m_reservedBits;
        int m_reservedNumWords;

        BuildSite m_siteCache[MAX_BUILDING_TYPE];
        bool m_siteCacheValid[MAX_BUILDING_TYPE];
    };

} // namespace Logic
