#include "SettlementSystem.h"
#include <stddef.h>
#include "../Systems/EconomySystem.h"
#include "../Systems/JobManager.h"
#include "../World/WorldModel.h"
#include "../Construction/ConstructionSite.h"
#include "../Core/JobTypes.h"
#include "../Core/BuildingTypes.h"
#include "../Core/ResourceTypes.h"
#include "../Definitions/BuildingDefinition.h"
#include "../Definitions/ProductionDefinition.h"

namespace World {

    SettlementSystem::SettlementSystem()
        : m_jobManager(NULL)
        , m_economySystem(NULL)
        , m_tickCount(0)
        , m_hasMineFoodRule(false)
        , m_eventCount(0)
    {
        for (int i = 0; i < kMaxEvents; ++i) {
            m_events[i] = ExpansionEvent();
        }
    }

    SettlementSystem::~SettlementSystem()
    {
    }

    void SettlementSystem::Tick(WorldModel& world)
    {
        ++m_tickCount;

        BootstrapProduction(world);
        BootstrapIndustry(world);
        BootstrapAgriculture(world);
        BootstrapInfrastructure(world);
        BootstrapMining(world);
        BootstrapForestry(world);
        BootstrapToolProduction(world);
        BootstrapHunter(world);
        BootstrapFisher(world);
        BootstrapMiningExpanded(world);
        BootstrapMetallurgy(world);
        BootstrapMilitary(world);
    }

    void SettlementSystem::BootstrapProduction(WorldModel& world)
    {
        // Rule: ensure Woodcutter exists
        // If no Woodcutter built, under construction, or pending → publish BuildWoodcutter Job

        if (HasBuilding(BuildingType_Woodcutter, world)) return;
        if (HasConstructionSite(BuildingType_Woodcutter, world)) return;
        if (HasPendingJob(BuildingType_Woodcutter, world)) return;

        if (m_jobManager == NULL) return;

        // Encode building type in buildingIndex field
        RecordEvent(ER_Bootstrap_Woodcutter, BuildingType_Woodcutter, world);
        m_jobManager->CreateJob(JobType_Construction, 100, (uint8_t)BuildingType_Woodcutter, 10);
    }

    void SettlementSystem::BootstrapIndustry(WorldModel& world)
    {
        if (m_jobManager == NULL) return;
        if (m_economySystem == NULL) return;

        // Rule 1: build first Sawmill when enough Wood stockpiled
        if (!HasBuilding(BuildingType_Sawmill, world) &&
            !HasConstructionSite(BuildingType_Sawmill, world) &&
            !HasPendingJob(BuildingType_Sawmill, world))
        {
            if (m_economySystem->GetAvailable(ResourceType_Wood, world) < kWoodForSawmill) return;
            RecordEvent(ER_First_Sawmill, BuildingType_Sawmill, world);
            m_jobManager->CreateJob(JobType_Construction, 200, (uint8_t)BuildingType_Sawmill, 10);
            return;
        }

        // Rule 2: capacity-aware expansion — build another Sawmill only when
        // current capacity is fully utilized and Wood supply is sufficient.
        if (HasPendingJob(BuildingType_Sawmill, world)) return;
        if (HasConstructionSite(BuildingType_Sawmill, world)) return;

        float planksPotential = m_economySystem->GetProductionPotential(ResourceType_Planks, world);
        if (planksPotential <= 0.0f) return;

        // Check for unused capacity: if flow < potential, don't expand
        int planksFlow = m_economySystem->GetResourceFlow(ResourceType_Planks);
        float flowPerTick = (float)planksFlow / EconomySystem::kFlowWindow;
        if (flowPerTick < planksPotential - 0.001f) return;

        // Check Wood supply: we need surplus beyond what existing Sawmills consume.
        // Sawmill: 2 Wood → 1 Plank. Wood consumed ≈ current Planks flow × 2.
        float woodPerTick = (float)m_economySystem->GetResourceFlow(ResourceType_Wood) / EconomySystem::kFlowWindow;
        if (woodPerTick <= flowPerTick * 2.0f + 0.001f) return;

        RecordEvent(ER_Expand_Sawmill, BuildingType_Sawmill, world);
        m_jobManager->CreateJob(JobType_Construction, 200, (uint8_t)BuildingType_Sawmill, 10);
    }

    void SettlementSystem::BootstrapInfrastructure(WorldModel& world)
    {
        // Rule: build first Storehouse when there's at least one active production building
        if (HasBuilding(BuildingType_Storehouse, world)) return;
        if (HasConstructionSite(BuildingType_Storehouse, world)) return;
        if (HasPendingJob(BuildingType_Storehouse, world)) return;
        if (m_jobManager == NULL) return;

        bool hasProducer = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].active) {
                hasProducer = true;
                break;
            }
        }
        if (!hasProducer) return;

        RecordEvent(ER_Bootstrap_Storehouse, BuildingType_Storehouse, world);
        m_jobManager->CreateJob(JobType_Construction, 150, (uint8_t)BuildingType_Storehouse, 10);
    }

    void SettlementSystem::BootstrapMining(WorldModel& world)
    {
        // Rule: build a producer for Stone using Definition Query API.
        // No hardcoded BuildingType — discover from resource dependency.
        if (m_jobManager == NULL) return;
        if (m_economySystem == NULL) return;

        // Discover Stone producer from definitions
        ProductionType prodType = GetProducer(ResourceType_Stone);
        if (prodType == PT_None) return;
        BuildingType bldType = GetBuildingTypeForProduction(prodType);
        if (bldType == BuildingType_None) return;

        // Already have one?
        if (HasBuilding(bldType, world)) return;
        if (HasConstructionSite(bldType, world)) return;
        if (HasPendingJob(bldType, world)) return;

        // Read build cost from definition — no hardcoded constants
        const BuildingDefinition& def = GetBuildingDefinition(bldType);
        int woodNeeded = 0;
        for (int i = 0; i < 4; ++i) {
            if (def.buildCost[i].resource == ResourceType_Wood) {
                woodNeeded = def.buildCost[i].required;
                break;
            }
        }
        if (m_economySystem->GetAvailable(ResourceType_Wood, world) < woodNeeded) return;

        RecordEvent(ER_First_Quarry, bldType, world);
        m_jobManager->CreateJob(JobType_Construction, 300, (uint8_t)bldType, 10);
    }

    void SettlementSystem::BootstrapForestry(WorldModel& world)
    {
        // Observation-based forestry management.
        // Reads actual tree state (mature count, empty spots) and building counts
        // to decide when Foresters are needed — no hardcoded 1:1 ratio.

        if (m_jobManager == NULL) return;
        if (m_economySystem == NULL) return;

        int wcCount = m_economySystem->GetBuildingCount(PT_Woodcutter, world);
        int fCount = m_economySystem->GetBuildingCount(PT_Forester, world);

        if (wcCount == 0) return; // no cutters → no foresters needed yet

        // Don't queue multiple foresters at once
        if (HasConstructionSite(BuildingType_Forester, world)) return;
        if (HasPendingJob(BuildingType_Forester, world)) return;

        int matureTrees = world.treeMatureCount;
        int emptySpots = world.treeEmptySpots;

        // Rule 1: No Forester at all → build first one if there's room to plant
        if (fCount == 0) {
            if (emptySpots > 5) {
                RecordEvent(ER_First_Forester, BuildingType_Forester, world);
                m_jobManager->CreateJob(JobType_Construction, 400, (uint8_t)BuildingType_Forester, 10);
            }
            return;
        }

        // Rule 2: Build additional Foresters if empty spots are accumulating
        // (signals cutting exceeds planting capacity) and more cutters than planters.
        if (emptySpots > 15 && wcCount > fCount && fCount < 3) {
            RecordEvent(ER_Expand_Forester, BuildingType_Forester, world);
            m_jobManager->CreateJob(JobType_Construction, 400, (uint8_t)BuildingType_Forester, 10);
            return;
        }

        // Rule 3: Emergency — very few mature trees but room for planting
        if (matureTrees < 5 && emptySpots > 10 && fCount < 2) {
            RecordEvent(ER_Emergency_Forester, BuildingType_Forester, world);
            m_jobManager->CreateJob(JobType_Construction, 400, (uint8_t)BuildingType_Forester, 10);
            return;
        }
    }

    void SettlementSystem::BootstrapToolProduction(WorldModel& world)
    {
        // Rule: discover Toolmaker through Definition Query API.
        // Dependency chain resolves naturally across ticks:
        // BootstrapProduction → Woodcutter → BootstrapIndustry → Sawmill →
        // BootstrapMining → Stonemason → BootstrapToolProduction → Toolmaker
        if (m_jobManager == NULL) return;
        if (m_economySystem == NULL) return;

        ProductionType toolProd = GetProducer(ResourceType_Tools);
        if (toolProd == PT_None) return;
        BuildingType toolBld = GetBuildingTypeForProduction(toolProd);
        if (toolBld == BuildingType_None) return;

        if (HasBuilding(toolBld, world)) return;
        if (HasConstructionSite(toolBld, world)) return;
        if (HasPendingJob(toolBld, world)) return;

        // Check that input producers exist (they handle their own bootstrap)
        const ProductionDefinition& prodDef = GetProductionDefinition(toolProd);
        for (int i = 0; i < 4; ++i) {
            if (prodDef.consumes[i].resource == ResourceType_None) break;
            ProductionType inputProd = GetProducer(prodDef.consumes[i].resource);
            if (inputProd == PT_None) return;
            BuildingType inputBld = GetBuildingTypeForProduction(inputProd);
            if (inputBld == BuildingType_None) return;
            if (!HasBuilding(inputBld, world)) return;
        }

        // Check build cost from definition
        const BuildingDefinition& bldDef = GetBuildingDefinition(toolBld);
        for (int i = 0; i < 4; ++i) {
            if (bldDef.buildCost[i].resource == ResourceType_None) continue;
            if (bldDef.buildCost[i].required <= 0) continue;
            if (m_economySystem->GetAvailable(bldDef.buildCost[i].resource, world) < static_cast<int>(bldDef.buildCost[i].required)) {
                return;
            }
        }

        RecordEvent(ER_First_Toolmaker, toolBld, world);
        m_jobManager->CreateJob(JobType_Construction, 500, (uint8_t)toolBld, 10);
    }

    void SettlementSystem::BootstrapHunter(WorldModel& world)
    {
        // Rule: discover Hunter through Definition Query API.
        // Hunter produces Meat from Animals (renewable resource).
        if (m_jobManager == NULL) return;
        if (m_economySystem == NULL) return;

        ProductionType prodType = GetProducer(ResourceType_Meat);
        if (prodType == PT_None) return;
        BuildingType bldType = GetBuildingTypeForProduction(prodType);
        if (bldType == BuildingType_None) return;

        if (HasBuilding(bldType, world)) return;
        if (HasConstructionSite(bldType, world)) return;
        if (HasPendingJob(bldType, world)) return;

        // Check build cost from definition
        const BuildingDefinition& bldDef = GetBuildingDefinition(bldType);
        for (int i = 0; i < 4; ++i) {
            if (bldDef.buildCost[i].resource == ResourceType_None) continue;
            if (bldDef.buildCost[i].required <= 0) continue;
            if (m_economySystem->GetAvailable(bldDef.buildCost[i].resource, world) < static_cast<int>(bldDef.buildCost[i].required)) {
                return;
            }
        }

        RecordEvent(ER_First_Hunter, bldType, world);
        m_jobManager->CreateJob(JobType_Construction, 350, (uint8_t)bldType, 10);
    }

    void SettlementSystem::BootstrapFisher(WorldModel& world)
    {
        // Rule: discover Fisher through Definition Query API.
        // Fisher produces Fish (renewable resource).
        if (m_jobManager == NULL) return;
        if (m_economySystem == NULL) return;

        ProductionType prodType = GetProducer(ResourceType_Fish);
        if (prodType == PT_None) return;
        BuildingType bldType = GetBuildingTypeForProduction(prodType);
        if (bldType == BuildingType_None) return;

        if (HasBuilding(bldType, world)) return;
        if (HasConstructionSite(bldType, world)) return;
        if (HasPendingJob(bldType, world)) return;

        // Check build cost from definition
        const BuildingDefinition& bldDef = GetBuildingDefinition(bldType);
        for (int i = 0; i < 4; ++i) {
            if (bldDef.buildCost[i].resource == ResourceType_None) continue;
            if (bldDef.buildCost[i].required <= 0) continue;
            if (m_economySystem->GetAvailable(bldDef.buildCost[i].resource, world) < static_cast<int>(bldDef.buildCost[i].required)) {
                return;
            }
        }

        RecordEvent(ER_First_Fisher, bldType, world);
        m_jobManager->CreateJob(JobType_Construction, 350, (uint8_t)bldType, 10);
    }

    void SettlementSystem::BootstrapMiningExpanded(WorldModel& world)
    {
        // Rule: if mine exists but no food producer, build one
        if (m_jobManager == NULL) return;

        bool hasMine = HasBuilding(BuildingType_CoalMine, world) ||
                       HasBuilding(BuildingType_IronMine, world);
        if (!hasMine) return;

        // Check if any food producer exists, under construction, or pending
        bool hasFoodProducer = HasBuilding(BuildingType_Hunter, world) ||
                               HasBuilding(BuildingType_Fisher, world) ||
                               HasConstructionSite(BuildingType_Hunter, world) ||
                               HasConstructionSite(BuildingType_Fisher, world) ||
                               HasPendingJob(BuildingType_Hunter, world) ||
                               HasPendingJob(BuildingType_Fisher, world);
        if (hasFoodProducer) return;

        // Build first available food producer
        ProductionType foodProdType = GetProducer(ResourceType_Meat);
        if (foodProdType == PT_None) {
            foodProdType = GetProducer(ResourceType_Fish);
        }
        if (foodProdType == PT_None) return;

        BuildingType foodBldType = GetBuildingTypeForProduction(foodProdType);
        if (foodBldType == BuildingType_None) return;

        // Check build cost from definition
        const BuildingDefinition& bldDef = GetBuildingDefinition(foodBldType);
        for (int i = 0; i < 4; ++i) {
            if (bldDef.buildCost[i].resource == ResourceType_None) continue;
            if (bldDef.buildCost[i].required <= 0) continue;
            if (m_economySystem != NULL &&
                m_economySystem->GetAvailable(bldDef.buildCost[i].resource, world) < static_cast<int>(bldDef.buildCost[i].required)) {
                return;
            }
        }

        RecordEvent(ER_Mine_Food, foodBldType, world);
        m_jobManager->CreateJob(JobType_Construction, 350, (uint8_t)foodBldType, 10);
        m_hasMineFoodRule = true;
    }

    void SettlementSystem::BootstrapAgriculture(WorldModel& world)
    {
        // Agriculture chain: Farm (Wheat) → Mill (Flour) → Bakery (Bread).
        // Uses Definition Query API throughout — no hardcoded BuildingType.

        if (m_jobManager == NULL) return;
        if (m_economySystem == NULL) return;

        // Rule 1: Build Farm (renewable — produces Wheat from nothing)
        {
            ProductionType prod = GetProducer(ResourceType_Wheat);
            if (prod == PT_None) return;
            BuildingType bld = GetBuildingTypeForProduction(prod);
            if (bld == BuildingType_None) return;

            if (!HasBuilding(bld, world) && !HasConstructionSite(bld, world) && !HasPendingJob(bld, world)) {
                const BuildingDefinition& def = GetBuildingDefinition(bld);
                bool canAfford = true;
                for (int i = 0; i < 4; ++i) {
                    if (def.buildCost[i].resource == ResourceType_None) continue;
                    if (def.buildCost[i].required <= 0) continue;
                    if (m_economySystem->GetAvailable(def.buildCost[i].resource, world) < static_cast<int>(def.buildCost[i].required)) {
                        canAfford = false;
                        break;
                    }
                }
                if (canAfford) {
                    RecordEvent(ER_First_Farm, bld, world);
                    m_jobManager->CreateJob(JobType_Construction, 310, (uint8_t)bld, 10);
                    return;
                }
            }
        }

        // Rule 2: Build Mill (Wheat → Flour) — only if Farm exists
        {
            BuildingType farmBld = GetBuildingTypeForProduction(GetProducer(ResourceType_Wheat));
            if (!HasBuilding(farmBld, world)) return;

            ProductionType prod = GetProducer(ResourceType_Flour);
            if (prod == PT_None) return;
            BuildingType bld = GetBuildingTypeForProduction(prod);
            if (bld == BuildingType_None) return;

            if (!HasBuilding(bld, world) && !HasConstructionSite(bld, world) && !HasPendingJob(bld, world)) {
                const BuildingDefinition& def = GetBuildingDefinition(bld);
                bool canAfford = true;
                for (int i = 0; i < 4; ++i) {
                    if (def.buildCost[i].resource == ResourceType_None) continue;
                    if (def.buildCost[i].required <= 0) continue;
                    if (m_economySystem->GetAvailable(def.buildCost[i].resource, world) < static_cast<int>(def.buildCost[i].required)) {
                        canAfford = false;
                        break;
                    }
                }
                if (canAfford) {
                    RecordEvent(ER_First_Mill, bld, world);
                    m_jobManager->CreateJob(JobType_Construction, 320, (uint8_t)bld, 10);
                    return;
                }
            }
        }

        // Rule 3: Build Bakery (Flour → Bread) — only if Mill exists
        {
            BuildingType millBld = GetBuildingTypeForProduction(GetProducer(ResourceType_Flour));
            if (!HasBuilding(millBld, world)) return;

            ProductionType prod = GetProducer(ResourceType_Bread);
            if (prod == PT_None) return;
            BuildingType bld = GetBuildingTypeForProduction(prod);
            if (bld == BuildingType_None) return;

            if (!HasBuilding(bld, world) && !HasConstructionSite(bld, world) && !HasPendingJob(bld, world)) {
                const BuildingDefinition& def = GetBuildingDefinition(bld);
                bool canAfford = true;
                for (int i = 0; i < 4; ++i) {
                    if (def.buildCost[i].resource == ResourceType_None) continue;
                    if (def.buildCost[i].required <= 0) continue;
                    if (m_economySystem->GetAvailable(def.buildCost[i].resource, world) < static_cast<int>(def.buildCost[i].required)) {
                        // Use total produced as proxy when output buffer is drained
                        // by high construction demand (Sawmills consume all available Wood).
                        if (def.buildCost[i].resource != ResourceType_Wood ||
                            m_economySystem->GetTotalProduced(ResourceType_Wood) < static_cast<int>(def.buildCost[i].required)) {
                            canAfford = false;
                            break;
                        }
                    }
                }
                if (canAfford) {
                    RecordEvent(ER_First_Bakery, bld, world);
                    m_jobManager->CreateJob(JobType_Construction, 330, (uint8_t)bld, 10);
                    return;
                }
            }
        }
    }

    void SettlementSystem::BootstrapMetallurgy(WorldModel& world)
    {
        // Metallurgy chain: CoalMine + IronMine → IronSmelter → WeaponSmith.
        // Uses Definition Query API throughout — no hardcoded BuildingType.

        if (m_jobManager == NULL) return;
        if (m_economySystem == NULL) return;

        // Rule 1: Build CoalMine
        {
            ProductionType prod = GetProducer(ResourceType_Coal);
            if (prod == PT_None) return;
            BuildingType bld = GetBuildingTypeForProduction(prod);
            if (bld == BuildingType_None) return;

            if (!HasBuilding(bld, world) && !HasConstructionSite(bld, world) && !HasPendingJob(bld, world)) {
                const BuildingDefinition& def = GetBuildingDefinition(bld);
                bool canAfford = true;
                for (int i = 0; i < 4; ++i) {
                    if (def.buildCost[i].resource == ResourceType_None) continue;
                    if (def.buildCost[i].required <= 0) continue;
                    if (m_economySystem->GetAvailable(def.buildCost[i].resource, world) < static_cast<int>(def.buildCost[i].required)) {
                        canAfford = false;
                        break;
                    }
                }
                if (canAfford) {
                    RecordEvent(ER_First_CoalMine, bld, world);
                    m_jobManager->CreateJob(JobType_Construction, 360, (uint8_t)bld, 10);
                    return;
                }
            }
        }

        // Rule 5: Expand CoalMine after sustained saturation (hysteresis)
        {
            ProductionType coalProd = GetProducer(ResourceType_Coal);
            if (coalProd != PT_None) {
                BuildingType coalBld = GetBuildingTypeForProduction(coalProd);
                if (coalBld != BuildingType_None) {
                    // Only evaluate saturation if CoalMine exists
                    if (HasBuilding(coalBld, world)) {
                        // Guard: don't queue multiple expansions
                        bool alreadyBuilding = HasConstructionSite(coalBld, world) || HasPendingJob(coalBld, world);

                        if (!alreadyBuilding) {
                            // Determine saturation: full capacity + low buffer
                            float potential = m_economySystem->GetProductionPotential(ResourceType_Coal, world);
                            int flow = m_economySystem->GetResourceFlow(ResourceType_Coal);
                            float flowPerTick = (float)flow / EconomySystem::kFlowWindow;
                            bool atFullCapacity = (flowPerTick >= potential - 0.001f);
                            int available = m_economySystem->GetAvailable(ResourceType_Coal, world);
                            bool outputDepleted = (available <= 2);

                            // Check affordability
                            const BuildingDefinition& def = GetBuildingDefinition(coalBld);
                            bool canAfford = true;
                            for (int i = 0; i < 4; ++i) {
                                if (def.buildCost[i].resource == ResourceType_None) continue;
                                if (def.buildCost[i].required <= 0) continue;
                                if (m_economySystem->GetAvailable(def.buildCost[i].resource, world) < static_cast<int>(def.buildCost[i].required)) {
                                    canAfford = false;
                                    break;
                                }
                            }

                            bool saturated = atFullCapacity && outputDepleted && canAfford;

                            // Feed into signal accumulator (3 consecutive windows = ~3000 ticks)
                            if (m_coalSignal.Update(saturated, 3, 1000)) {
                                RecordEvent(ER_Expand_CoalMine, coalBld, world);
                                m_jobManager->CreateJob(JobType_Construction, 360, (uint8_t)coalBld, 10);
                                m_coalSignal.Reset();
                            }
                        } else {
                            // Reset signal while construction is in progress
                            m_coalSignal.Reset();
                        }
                    }
                }
            }
        }

        // Rule 2: Build IronMine
        {
            ProductionType prod = GetProducer(ResourceType_IronOre);
            if (prod == PT_None) return;
            BuildingType bld = GetBuildingTypeForProduction(prod);
            if (bld == BuildingType_None) return;

            if (!HasBuilding(bld, world) && !HasConstructionSite(bld, world) && !HasPendingJob(bld, world)) {
                const BuildingDefinition& def = GetBuildingDefinition(bld);
                bool canAfford = true;
                for (int i = 0; i < 4; ++i) {
                    if (def.buildCost[i].resource == ResourceType_None) continue;
                    if (def.buildCost[i].required <= 0) continue;
                    if (m_economySystem->GetAvailable(def.buildCost[i].resource, world) < static_cast<int>(def.buildCost[i].required)) {
                        canAfford = false;
                        break;
                    }
                }
                if (canAfford) {
                    RecordEvent(ER_First_IronMine, bld, world);
                    m_jobManager->CreateJob(JobType_Construction, 370, (uint8_t)bld, 10);
                    return;
                }
            }
        }

        // Rule 3: Build IronSmelter (IronOre → IronBar) — only if IronMine exists
        {
            ProductionType ironProd = GetProducer(ResourceType_IronOre);
            BuildingType ironMineBld = GetBuildingTypeForProduction(ironProd);
            if (!HasBuilding(ironMineBld, world)) return;

            ProductionType prod = GetProducer(ResourceType_IronBar);
            if (prod == PT_None) return;
            BuildingType bld = GetBuildingTypeForProduction(prod);
            if (bld == BuildingType_None) return;

            if (!HasBuilding(bld, world) && !HasConstructionSite(bld, world) && !HasPendingJob(bld, world)) {
                const BuildingDefinition& def = GetBuildingDefinition(bld);
                bool canAfford = true;
                for (int i = 0; i < 4; ++i) {
                    if (def.buildCost[i].resource == ResourceType_None) continue;
                    if (def.buildCost[i].required <= 0) continue;
                    if (m_economySystem->GetAvailable(def.buildCost[i].resource, world) < static_cast<int>(def.buildCost[i].required)) {
                        canAfford = false;
                        break;
                    }
                }
                if (canAfford) {
                    RecordEvent(ER_First_IronSmelter, bld, world);
                    m_jobManager->CreateJob(JobType_Construction, 380, (uint8_t)bld, 10);
                    return;
                }
            }
        }

        // Rule 4: Build WeaponSmith (IronBar + Coal → Weapons)
        {
            ProductionType ironProd = GetProducer(ResourceType_IronBar);
            BuildingType ironSmlBld = GetBuildingTypeForProduction(ironProd);
            if (!HasBuilding(ironSmlBld, world)) return;

            ProductionType prod = GetProducer(ResourceType_Weapons);
            if (prod == PT_None) return;
            BuildingType bld = GetBuildingTypeForProduction(prod);
            if (bld == BuildingType_None) return;

            if (!HasBuilding(bld, world) && !HasConstructionSite(bld, world) && !HasPendingJob(bld, world)) {
                const BuildingDefinition& def = GetBuildingDefinition(bld);
                bool canAfford = true;
                for (int i = 0; i < 4; ++i) {
                    if (def.buildCost[i].resource == ResourceType_None) continue;
                    if (def.buildCost[i].required <= 0) continue;
                    if (m_economySystem->GetAvailable(def.buildCost[i].resource, world) < static_cast<int>(def.buildCost[i].required)) {
                        // Use total produced as proxy when output buffer is drained
                        if (def.buildCost[i].resource != ResourceType_Wood ||
                            m_economySystem->GetTotalProduced(ResourceType_Wood) < static_cast<int>(def.buildCost[i].required)) {
                            canAfford = false;
                            break;
                        }
                    }
                }
                if (canAfford) {
                    RecordEvent(ER_First_WeaponSmith, bld, world);
                    m_jobManager->CreateJob(JobType_Construction, 390, (uint8_t)bld, 10);
                    return;
                }
            }
        }
    }

    void SettlementSystem::BootstrapMilitary(WorldModel& world)
    {
        // Rule: discover Barracks through Definition Query API.
        // Soldiers are the first end-product that has no downstream consumer.

        if (m_jobManager == NULL) return;
        if (m_economySystem == NULL) return;

        ProductionType barracksProd = GetProducer(ResourceType_Soldiers);
        if (barracksProd == PT_None) return;
        BuildingType barracksBld = GetBuildingTypeForProduction(barracksProd);
        if (barracksBld == BuildingType_None) return;

        // Already built or in progress
        if (HasBuilding(barracksBld, world)) return;
        if (HasConstructionSite(barracksBld, world)) return;
        if (HasPendingJob(barracksBld, world)) return;

        // Check input producers exist
        const ProductionDefinition& prodDef = GetProductionDefinition(barracksProd);
        for (int i = 0; i < 4; ++i) {
            if (prodDef.consumes[i].resource == ResourceType_None) break;
            ProductionType inputProd = GetProducer(prodDef.consumes[i].resource);
            if (inputProd == PT_None) return;
            BuildingType inputBld = GetBuildingTypeForProduction(inputProd);
            if (inputBld == BuildingType_None) return;
            if (!HasBuilding(inputBld, world)) return;
        }

        // Check build cost from definition
        const BuildingDefinition& bldDef = GetBuildingDefinition(barracksBld);
        for (int i = 0; i < 4; ++i) {
            if (bldDef.buildCost[i].resource == ResourceType_None) continue;
            if (bldDef.buildCost[i].required <= 0) continue;
            if (m_economySystem->GetAvailable(bldDef.buildCost[i].resource, world) < static_cast<int>(bldDef.buildCost[i].required)) {
                // Use total produced as proxy when output buffer is drained
                if (bldDef.buildCost[i].resource != ResourceType_Wood ||
                    m_economySystem->GetTotalProduced(ResourceType_Wood) < static_cast<int>(bldDef.buildCost[i].required)) {
                    return;
                }
            }
        }

        RecordEvent(ER_First_Barracks, barracksBld, world);
        m_jobManager->CreateJob(JobType_Construction, 600, (uint8_t)barracksBld, 10);
    }

    bool SettlementSystem::HasBuilding(BuildingType type, const WorldModel& world) const
    {
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == type) return true;
        }
        return false;
    }

    bool SettlementSystem::HasConstructionSite(BuildingType type, const WorldModel& world) const
    {
        for (int i = 0; i < world.activeSiteCount; ++i) {
            if (world.activeSites[i].type == type) return true;
        }
        return false;
    }

    bool SettlementSystem::HasPendingJob(BuildingType type, const WorldModel& world) const
    {
        if (m_jobManager == NULL) return false;

        bool hasActiveJob = false;
        bool hasCompletedJob = false;
        for (int i = 0; i < m_jobManager->GetJobCount(); ++i) {
            const Job& job = m_jobManager->GetJob(i);
            if (job.type != JobType_Construction || job.buildingIndex != (uint8_t)type) continue;
            if (job.state == JobState_Completed) {
                hasCompletedJob = true;
            } else {
                hasActiveJob = true;
            }
        }
        if (hasActiveJob) return true;

        // A completed job without a matching site or building means the pipeline
        // is in the gap between job completion and site creation (1 tick).
        // Treat as pending to prevent duplicate construction decisions.
        if (hasCompletedJob) {
            bool hasSite = false;
            for (int s = 0; s < world.activeSiteCount; ++s) {
                if (world.activeSites[s].type == type) { hasSite = true; break; }
            }
            bool hasBuilding = false;
            for (int b = 0; b < world.productionBuildingCount; ++b) {
                if (world.productionBuildings[b].type == type) { hasBuilding = true; break; }
            }
            if (!hasSite && !hasBuilding) return true;
        }

        // Check for unfulfilled construction requests
        for (int i = 0; i < world.pendingConstructionCount; ++i) {
            if (!world.pendingConstructionRequests[i].fulfilled &&
                world.pendingConstructionRequests[i].type == type) {
                return true;
            }
        }
        return false;
    }

    const ExpansionEvent& SettlementSystem::GetExpansionEvent(int index) const
    {
        static const ExpansionEvent s_empty;
        if (index < 0 || index >= m_eventCount) return s_empty;
        return m_events[index];
    }

    void SettlementSystem::RecordEvent(ExpansionRuleId ruleId, BuildingType bt, const WorldModel& world)
    {
        if (m_eventCount >= kMaxEvents) return;
        ExpansionEvent& ev = m_events[m_eventCount];
        ev.tick = m_tickCount;
        ev.ruleId = ruleId;
        ev.buildingType = bt;
        ev.woodFlow = m_economySystem ? m_economySystem->GetResourceFlow(ResourceType_Wood) : 0;
        ev.woodPotential = m_economySystem ? m_economySystem->GetProductionPotential(ResourceType_Wood, world) : 0.0f;
        ev.stoneFlow = m_economySystem ? m_economySystem->GetResourceFlow(ResourceType_Stone) : 0;
        ev.stonePotential = m_economySystem ? m_economySystem->GetProductionPotential(ResourceType_Stone, world) : 0.0f;
        ev.availableWood = m_economySystem ? m_economySystem->GetAvailable(ResourceType_Wood, world) : 0;
        m_eventCount++;
    }

}
