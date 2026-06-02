#pragma once
#include <vector>
#include "../World/Components/Building.h"
#include "../World/Warehouse.h"
#include "../World/TransportJob.h"
#include "../World/CarrierManager.h"
#include "EconomySettings.h"

namespace Logic {

    class EconomyManager {
    public:
        EconomyManager();
        void Update(World::CarrierManager* carrierManager);
        void AddBuilding(World::Building* building);
        void SetWarehouse(World::Warehouse* warehouse) { m_warehouse = warehouse; }

    private:
        std::vector<World::Building*> m_buildings;
        World::Warehouse* m_warehouse;
        EconomySettings m_settings;
    };
}
