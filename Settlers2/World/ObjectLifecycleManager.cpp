#include "stdafx.h"
#include "ObjectLifecycleManager.h"
#include "ObjectDeletionRules.h"
#include "FlagManager.h"
#include "RoadManager.h"
#include "CarrierManager.h"
#include "TransportJobManager.h"
#include "ConstructionManager.h"
#include "Components/Building.h"
#include "Carrier.h"
#include "Flag.h"
#include "Road.h"
#include "../Logic/EconomyManager.h"

namespace World {

    ObjectLifecycleManager::ObjectLifecycleManager(
        FlagManager* fm, RoadManager* rm, CarrierManager* cm,
        TransportJobManager* jm, ConstructionManager* con, Logic::EconomyManager* em)
        : m_flagManager(fm), m_roadManager(rm), m_carrierManager(cm),
          m_jobManager(jm), m_constructionManager(con), m_economyManager(em)
    {}

    bool ObjectLifecycleManager::SafeDeleteFlag(Flag* flag) {
        return CanDestroyFlag(flag, m_carrierManager, m_jobManager, m_roadManager);
    }

    bool ObjectLifecycleManager::SafeDeleteRoad(Road* road) {
        return CanDestroyRoad(road, m_carrierManager, m_jobManager);
    }

    bool ObjectLifecycleManager::SafeDeleteCarrier(Carrier* carrier) {
        return CanDestroyCarrier(carrier, m_jobManager);
    }

    bool ObjectLifecycleManager::SafeDeleteBuilding(Building* building) {
        return CanDestroyBuilding(building, m_economyManager);
    }

    void ObjectLifecycleManager::ForceDeleteFlag(Flag* flag) {
        if (!flag) return;
        if (m_jobManager) m_jobManager->CancelJobsForFlag(flag);
        // Remove all roads connected to this flag entirely (not just PendingDelete)
        std::vector<Road*> roadsCopy = flag->roads;
        for (size_t i = 0; i < roadsCopy.size(); ++i) {
            if (roadsCopy[i]) {
                ForceDeleteRoad(roadsCopy[i]);
            }
        }
        if (m_carrierManager) m_carrierManager->RemoveCarriersForFlag(flag);
        if (m_flagManager) m_flagManager->RemoveFlag(flag);
    }

    void ObjectLifecycleManager::ForceDeleteRoad(Road* road) {
        if (!road) return;
        if (m_carrierManager) m_carrierManager->RemoveCarriersForRoad(road);
        if (m_constructionManager) m_constructionManager->OnRoadRemoved(road);
        if (m_roadManager) m_roadManager->RemoveRoad(road);
    }

    void ObjectLifecycleManager::ForceDeleteCarrier(Carrier* carrier) {
        if (!carrier || !m_carrierManager) return;
        CarrierHandle h = m_carrierManager->GetCarrierHandle(carrier);
        m_carrierManager->UnregisterCarrier(h);
        delete carrier;
    }

    void ObjectLifecycleManager::ForceDeleteBuilding(Building* building) {
        if (!building) return;
        if (m_economyManager) m_economyManager->RemoveBuilding(building);
        if (building->connectedFlag) {
            building->connectedFlag->building = NULL;
            building->connectedFlag->hasBuilding = false;
        }
        if (m_constructionManager && building->state != State_Finished) {
            m_constructionManager->RemoveSiteAt(building->pos.x, building->pos.y);
        }
        delete building;
    }
}
