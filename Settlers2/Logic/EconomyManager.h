#pragma once
#include <vector>
#include "../World/Components/Building.h"
#include "../World/Warehouse.h"
#include "../World/FlagManager.h"
#include "../World/RoadManager.h"
#include "EconomySettings.h"
#include "ResourceRegistry.h"

namespace World {
    class Flag;
    class ConstructionSite;
}

namespace Logic {

    struct ResourceRequest {
        World::Building* requester;
        World::ResourceType type;
        int amount;
        int priority;
        bool active;

        ResourceRequest() : requester(NULL), type(World::ResourceType_None), amount(0), priority(0), active(false) {}
    };

    struct ConstructionResourceRequest {
        uint32_t destFlagId;    // flag ID (safe across flag deletion)
        World::Flag* destFlag;  // cached pointer, may be dangling if flag deleted
        World::ResourceType type;
        int amount;
        int priority;
        bool active;

        ConstructionResourceRequest() : destFlagId(0), destFlag(NULL), type(World::ResourceType_None), amount(0), priority(0), active(false) {}
    };

    class EconomyManager {
    public:
        EconomyManager();
        void Update(float dt);
        void CollectWarehouse();
        void AddBuilding(World::Building* building);
        void RemoveBuilding(World::Building* building);
        void SetWarehouse(World::Warehouse* warehouse) { m_warehouse = warehouse; }
        World::Warehouse* GetWarehouse() const { return m_warehouse; }
        void SetFlagManager(World::FlagManager* fm) { m_flagManager = fm; }
        void SetRoadManager(World::RoadManager* rm) { m_roadManager = rm; }
        bool HasBuilding(World::BuildingType type) const;
        int GetBuildingCount() const { return (int)m_buildings.size(); }
        World::Building* GetBuilding(int index) const { return m_buildings[index]; }
        void ValidateEconomy();

        ResourceRegistry& GetRegistry() { return m_registry; }
        const ResourceRegistry& GetRegistry() const { return m_registry; }

        bool HasWorkers(World::Building* building) const;
        void RequestResource(World::Building* requester, World::ResourceType type, int amount, int priority);
        void RequestConstructionResource(World::Flag* destFlag, World::ResourceType type, int amount, int priority);
        bool HasActiveConstructionRequest(World::Flag* destFlag) const;

        static const int MAX_REQUESTS = 256;
        static const int MAX_CONSTRUCTION_REQUESTS = 64;

    private:
        World::Building* FindBestSupplier(World::ResourceType type, int& outAmount,
                                          World::Building* exclude, const Vector2i& requesterPos);

        std::vector<World::Building*> m_buildings;
        ResourceRegistry m_registry;
        World::Warehouse* m_warehouse;
        World::FlagManager* m_flagManager;
        World::RoadManager* m_roadManager;
        EconomySettings m_settings;
        ResourceRequest m_requests[MAX_REQUESTS];
        ConstructionResourceRequest m_constructionRequests[MAX_CONSTRUCTION_REQUESTS];
        int m_validateCounter;
    };
}
