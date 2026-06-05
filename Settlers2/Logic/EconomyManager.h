#pragma once
#include <vector>
#include "../World/Components/Building.h"
#include "../World/Warehouse.h"
#include "../World/TransportJob.h"
#include "../World/CarrierManager.h"
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
        World::Flag* destFlag;
        World::ResourceType type;
        int amount;
        int priority;
        bool active;

        ConstructionResourceRequest() : destFlag(NULL), type(World::ResourceType_None), amount(0), priority(0), active(false) {}
    };

    class EconomyManager {
    public:
        EconomyManager();
        void Update(World::CarrierManager* carrierManager);
        void AddBuilding(World::Building* building);
        void SetWarehouse(World::Warehouse* warehouse) { m_warehouse = warehouse; }
        bool HasBuilding(World::BuildingType type) const;
        int GetBuildingCount() const { return (int)m_buildings.size(); }
        World::Building* GetBuilding(int index) const { return m_buildings[index]; }
        void ValidateEconomy();

        ResourceRegistry& GetRegistry() { return m_registry; }
        const ResourceRegistry& GetRegistry() const { return m_registry; }

        void RequestResource(World::Building* requester, World::ResourceType type, int amount, int priority);
        void RequestConstructionResource(World::Flag* destFlag, World::ResourceType type, int amount, int priority);

        static const int MAX_REQUESTS = 256;
        static const int MAX_CONSTRUCTION_REQUESTS = 64;

    private:
        void ComputeDeliveryReserved();
        World::Building* FindBestSupplier(World::ResourceType type, int& outAmount,
                                          World::Building* exclude, const Vector2i& requesterPos);

        std::vector<World::Building*> m_buildings;
        ResourceRegistry m_registry;
        int m_deliveryReserved[World::ResourceType_Count];
        World::Warehouse* m_warehouse;
        EconomySettings m_settings;
        ResourceRequest m_requests[MAX_REQUESTS];
        ConstructionResourceRequest m_constructionRequests[MAX_CONSTRUCTION_REQUESTS];
        int m_validateCounter;
    };
}
