#ifndef WORLD_COMPONENTS_SAWMILL_H
#define WORLD_COMPONENTS_SAWMILL_H

#include "ProductionBuilding.h"
#include "../Map.h"
#include "../DemandManager.h"

namespace World {

class Sawmill : public ProductionBuilding {
    int m_lastWoodDemand;

public:
    Sawmill(int x, int y, uint8_t o, Map* m)
        : ProductionBuilding(BuildingType::Sawmill, x, y, o, m)
        , m_lastWoodDemand(0)
    {
        m_numRules = 1;
        m_rules[0].AddInput(ResourceType_Wood);
        m_rules[0].AddOutput(ResourceType_Planks);
        m_rules[0].outputCap = 9;
        SyncIOFromRules();
    }

    int MaxStoragePerType(ResourceType type) const override {
        if (type == ResourceType_Wood) return 9;
        if (type == ResourceType_Planks) return 9;
        return 5;
    }

    void Update(float dt) override {
        DemandManager* dm = map ? map->GetDemandManager() : NULL;
        if (dm && connectedFlag) {
            int woodInStorage = m_storage[ResourceType_Wood];
            int woodNeeded = MaxStoragePerType(ResourceType_Wood) - woodInStorage;
            if (woodNeeded > 0) {
                // Always set demand: external ClearDemand (e.g. RemoveSite) may
                // have removed it since last frame.
                dm->SetDemand(ResourceType_Wood, (uint32_t)woodNeeded,
                    connectedFlag->handle, 20);
                m_lastWoodDemand = woodNeeded;
            } else if (m_lastWoodDemand > 0) {
                dm->ClearDemand(ResourceType_Wood, connectedFlag->handle);
                m_lastWoodDemand = 0;
            }
        }

        ProductionBuilding::Update(dt);
    }
};

} // namespace World

#endif
