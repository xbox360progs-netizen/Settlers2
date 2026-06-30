#include "stdafx.h"
#include "ObjectDeletionRules.h"
#include "CarrierManager.h"
#include "TransportJobManager.h"
#include "RoadManager.h"
#include "FlagManager.h"
#include "Components/Building.h"
#include "Carrier.h"
#include "Road.h"
#include "Flag.h"
#include "../Logic/EconomyManager.h"

namespace World {

    bool CanDestroyFlag(Flag* flag, CarrierManager* cm, TransportJobManager* jm, RoadManager* rm) {
        if (!flag) return true;
        if (flag->type == FLAG_WAREHOUSE) return false;
        if (flag->state != Active) return true;
        return !cm->IsFlagInUse(flag) && !jm->IsFlagInUse(flag) && !rm->HasRoadsConnectedToFlag(flag);
    }

    bool CanDestroyRoad(Road* road, CarrierManager* cm, TransportJobManager* jm) {
        if (!road) return true;
        if (road->state != Active) return true;
        return road->carrier.IsValid() == false && !cm->IsRoadInUse(road) && !jm->IsRoadInUse(road);
    }

    bool CanDestroyCarrier(Carrier* carrier, TransportJobManager* jm) {
        (void)jm;
        if (!carrier) return true;
        return carrier->readyToRemove;
    }

    bool CanDestroyBuilding(Building* building, Logic::EconomyManager* em) {
        if (!building) return true;
        if (building->IsWarehouse()) return false;
        if (building->m_fsmState == BuildingFSM_Producing) {
            return false;
        }
        if (em && em->HasWorkers(building)) {
            return false;
        }
        return true;
    }
}
