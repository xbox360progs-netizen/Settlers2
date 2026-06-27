#include "stdafx.h"
#include "ObjectLifecycleManager.h"
#include "ObjectDeletionRules.h"
#include "FlagManager.h"
#include "RoadManager.h"
#include "CarrierManager.h"
#include "TransportJobManager.h"
#include "ConstructionManager.h"
#include "Components/Building.h"
#include "Map.h"
#include "../Logic/CoordinateSystem.h"
#include "Carrier.h"
#include "Flag.h"
#include "Road.h"
#include "../Logic/EconomyManager.h"

namespace World {

    ObjectLifecycleManager::ObjectLifecycleManager(
        FlagManager* fm, RoadManager* rm, CarrierManager* cm,
        CargoManager* cargoMgr, TransportJobManager* jm,
        ConstructionManager* con, Logic::EconomyManager* em,
        Map* map)
        : m_flagManager(fm), m_roadManager(rm), m_carrierManager(cm),
          m_cargoManager(cargoMgr), m_jobManager(jm),
          m_constructionManager(con), m_economyManager(em), m_map(map),
          m_eventBus(NULL)
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
        // Release all cargo on this flag before destroying it
        if (m_cargoManager) {
            m_cargoManager->ReleaseAllForFlag(flag->handle);
        }
        // Remove all roads connected to this flag entirely (not just PendingDelete)
        std::vector<Road*> roadsCopy = flag->roads;
        for (size_t i = 0; i < roadsCopy.size(); ++i) {
            if (roadsCopy[i]) {
                ForceDeleteRoad(roadsCopy[i]);
            }
        }
        if (m_carrierManager) m_carrierManager->RemoveCarriersForFlag(flag);
        if (m_flagManager) m_flagManager->RemoveFlag(flag);
        // Post event as consequence of deletion (not trigger).
        // Systems that react to flag removal (e.g. EconomySystem, TransportSystem)
        // receive this after the flag is gone, preserving causal order.
        if (m_eventBus) {
            m_eventBus->Post(Core::Event_FlagDeleted);
        }
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

    void ObjectLifecycleManager::ForceDeleteBuilding(Building* building, Map* map) {
        if (!building) return;
        // Clear the building's footprint on the Buildings layer before deletion
        if (map) {
            CoordinateSystem& coords = CoordinateSystem::GetInstance();
            int nodesW = coords.GetNodesWidth();
            int nodesH = coords.GetNodesHeight();
            TileLayer* buildingsLayer = map->GetLayer(LayerType::Buildings);
            if (buildingsLayer) {
                for (int dy = 0; dy < building->m_footprintH; ++dy) {
                    for (int dx = 0; dx < building->m_footprintW; ++dx) {
                        int tx = building->pos.x + building->m_footprintX + dx;
                        int ty = building->pos.y + building->m_footprintY + dy;
                        if (tx >= 0 && tx < nodesW && ty >= 0 && ty < nodesH) {
                            Tile& t = buildingsLayer->GetTile(tx, ty);
                            t.buildingType = -1;
                        }
                    }
                }
            }
        }
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

    void ObjectLifecycleManager::OnCommand(Core::CommandType type, void* data)
    {
        if (type == Core::Cmd_DeleteFlag) {
            Core::DeleteFlagCmd* cmd = static_cast<Core::DeleteFlagCmd*>(data);
            if (m_flagManager) {
                Flag* flag = m_flagManager->GetFlagById(cmd->flagId);
                if (flag) {
                    ForceDeleteFlag(flag);
                }
            }
        } else if (type == Core::Cmd_DeleteBuilding) {
            Core::DeleteBuildingCmd* cmd = static_cast<Core::DeleteBuildingCmd*>(data);
            if (m_flagManager) {
                Flag* flag = m_flagManager->GetFlagById(cmd->flagId);
                if (flag && flag->building) {
                    ForceDeleteBuilding(flag->building, m_map);
                }
            }
        }
    }
}
