#include "stdafx.h"
#include "SimulationCoreBuildingSource.h"
#include "../../../SimulationCore/World/WorldModel.h"
#include "BuildingView.h"

// Temporary — reads SimulationCore WorldModel for migrated building data.
// Remove after WorldModel is the sole source and LegacyBuildingSource is deleted.
//
// Mapping notes:
//   SimulationCore::BuildingType and legacy World::BuildingType use different enum values.
//   Legacy enum is renderer-compatible. MapBuildingType() converts from SimulationCore
//   enum values to legacy enum values.

namespace Scene {

void SimulationCoreBuildingSource::SetWorldModel(const World::WorldModel* world)
{
    m_world = world;
}


// ─── IBuildingSource (completed buildings) ─────────────────────────────

uint32_t SimulationCoreBuildingSource::GetBuildingCount() const
{
    if (!m_world) return 0;
    return static_cast<uint32_t>(m_world->productionBuildingCount);
}

bool SimulationCoreBuildingSource::GetBuilding(uint32_t index, BuildingView& out) const
{
    if (!m_world || index >= static_cast<uint32_t>(m_world->productionBuildingCount))
        return false;

    const World::ProductionBuilding& pb = m_world->productionBuildings[index];

    out.flagX = pb.position.x;
    out.flagY = pb.position.y;
    out.kind = 1; // completed building
    out.buildingType = MapBuildingType(static_cast<uint8_t>(pb.type));
    out.buildingId = index + 1;
    out.depleted = false;
    out.fsmState = pb.active ? 1 : 0; // 0=Idle, 1=Producing
    out.hasWorker = pb.active;
    out.workerVisualState = pb.active ? WVS_Working : WVS_Idle;
    out.color = 0xFFFFFFFF;
    return true;
}


// ─── IConstructionSiteSource ─────────────────────────────────────────────

uint32_t SimulationCoreBuildingSource::GetConstructionSiteCount() const
{
    if (!m_world) return 0;
    return static_cast<uint32_t>(m_world->activeSiteCount);
}

bool SimulationCoreBuildingSource::GetConstructionSite(uint32_t index, BuildingView& out) const
{
    if (!m_world || index >= static_cast<uint32_t>(m_world->activeSiteCount))
        return false;

    const World::ConstructionSite& site = m_world->activeSites[index];

    out.flagX = site.position.x;
    out.flagY = site.position.y;
    out.kind = 2; // construction site
    out.buildingType = MapBuildingType(static_cast<uint8_t>(site.type));
    out.buildingId = index + 1;
    out.depleted = false;
    out.fsmState = 0;
    out.hasWorker = site.builderAssigned;
    out.workerVisualState = site.builderAssigned ? WVS_Working : WVS_None;
    out.color = 0xFFFFFFFF;
    return true;
}


// ─── Mapping: SimulationCore → Legacy ─────────────────────────────────────

uint8_t SimulationCoreBuildingSource::MapBuildingType(uint8_t simType)
{
    // SimulationCore::BuildingType values:
    //   BuildingType_None=0, Woodcutter=1, Forester=2, Sawmill=3, Stonemason=4,
    //   Fisher=5, Hunter=6, Farm=7, Mill=8, Bakery=9, CoalMine=10, IronMine=11,
    //   IronSmelter=12, Toolmaker=13, GoldMine=14, WeaponSmith=15, Barracks=16,
    //   Storehouse=17, Residence=18, Well=19
    //
    // Legacy World::BuildingType values (renderer-compatible):
    //   Building_None=0, Hut=1, Tower=2, Fortress=3, Castle=4, Forester=5,
    //   Woodcutter=6, Sawmill=7, Stonemason=8, CoalMine=9, IronMine=10,
    //   GoldMine=11, IronSmelter=12, GoldSmelter=13, Farm=14, Mill=15, Bakery=16,
    //   Fisher=17, Hunter=18, Baker=19, Brewer=20, ToolWorkshop=21, Storehouse=22,
    //   Residence=23, Stronghold=24, Well=25, BronzeMine=26, ToolMaker=27,
    //   Barracks=28, BronzeSmelter=29

    static const uint8_t legacyValues[20] = {
        0,  // BuildingType_None      → Building_None
        6,  // Woodcutter             → Woodcutter
        5,  // Forester               → Forester
        7,  // Sawmill                → Sawmill
        8,  // Stonemason             → Stonemason
        17, // Fisher                 → Fisher
        18, // Hunter                 → Hunter
        14, // Farm                   → Farm
        15, // Mill                   → Mill
        16, // Bakery                 → Bakery
        9,  // CoalMine               → CoalMine
        10, // IronMine               → IronMine
        12, // IronSmelter            → IronSmelter
        27, // Toolmaker              → ToolMaker
        11, // GoldMine               → GoldMine
        15, // WeaponSmith            → WeaponSmith (no legacy equivalent, maps to IronSmelter as closest)
        28, // Barracks               → Barracks
        22, // Storehouse             → Storehouse
        23, // Residence              → Residence
        25, // Well                   → Well
    };

    // Compile-time check: table must match BuildingType_Count.
    // If BuildingType_Count changes, this line fails to compile,
    // forcing an update to the mapping table above.
    typedef char kCheck_BuildingType_Count_Mismatch[
        sizeof(legacyValues) / sizeof(legacyValues[0]) == BuildingType_Count ? 1 : -1
    ];

    if (simType < BuildingType_Count) {
        return legacyValues[simType];
    }

    // Unknown type — return Building_None. If this fires in testing,
    // the mapping table needs a new entry.
    return 0;
}

} // namespace Scene
