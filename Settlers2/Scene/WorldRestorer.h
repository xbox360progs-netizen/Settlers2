#pragma once

#include "../World/Components/Building.h"

namespace World {
    class CarrierManager;
    class ConstructionManager;
    class DemandManager;
    class Flag;
    class FlagManager;
    class Map;
    class RoadNetworkRelinker;
    class StorehouseManager;
}

namespace Logic {
    class EconomyManager;
}

namespace Scene {

struct WorldRestorerContext {
    World::Map* map;
    World::FlagManager* flags;
    Logic::EconomyManager* economy;
    World::StorehouseManager* storehouse;
    World::ConstructionManager* construction;
    World::CarrierManager* carriers;
    World::DemandManager* demand;
    World::RoadNetworkRelinker* relinker;
};

class WorldRestorer {
public:
    WorldRestorer();

    void SetContext(const WorldRestorerContext& ctx) { m_ctx = ctx; }
    const WorldRestorerContext& GetContext() const { return m_ctx; }

    void RestoreBuildingsFromLayer();
    void AssignOreDepositsToMountains();

private:
    World::BuildingType GetBuildingTypeFromSpriteName(const std::string& name) const;
    static void AdjustEntranceForParity(bool buildingEvenY, int& entranceX, int entranceY);

    WorldRestorerContext m_ctx;
};

} // namespace Scene
