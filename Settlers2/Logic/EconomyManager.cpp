#include "stdafx.h"
#include "EconomyManager.h"
#include "../World/Components/Building.h"
#include "../World/Flag.h"
#include "../World/Warehouse.h"

namespace Logic {

    EconomyManager::EconomyManager()
        : m_warehouse(NULL)
    {
    }

    void EconomyManager::Update(World::CarrierManager* carrierManager) {
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            World::Building* b = m_buildings[i];
            b->Update();

            for (std::map<World::ResourceType, int>::iterator it = b->inventory.begin(); it != b->inventory.end(); ++it) {
                World::ResourceType type = it->first;
                int& amount = it->second;

                if (amount > 0 && b->connectedFlag && m_warehouse && m_warehouse->connectedFlag) {
                    World::TransportJob job;
                    job.resource = type;
                    job.source = b->connectedFlag;
                    job.destination = m_warehouse->connectedFlag;
                    job.priority = m_settings.priorities[type];

                    carrierManager->AssignJob(job);

                    amount--;
                }
            }
        }
    }

    void EconomyManager::AddBuilding(World::Building* building) {
        m_buildings.push_back(building);
    }
}
