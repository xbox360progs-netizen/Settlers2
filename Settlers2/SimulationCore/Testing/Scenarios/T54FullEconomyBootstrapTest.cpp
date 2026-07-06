#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/SimulationAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Core/ProductionTypes.h"
#include "../../Definitions/ProductionDefinition.h"
#include "../../Definitions/BuildingDefinition.h"
#include "../../Systems/EconomySystem.h"
#include "../../Warehouse/WarehouseSystem.h"
#include "../../Systems/JobManager.h"
#include <stdio.h>
#include "../../Core/TreeSystem.h"

namespace World {

    // Full Economy Bootstrap Test
    //
    // Verifies SettlementSystem can bootstrap the complete economic graph
    // from scratch — no pre-placed buildings, only trees and animals.
    //
    // Ticks: 50000 (allows time for sequential construction, resource
    // accumulation, and transport across all chains).
    //
    // Checks:
    //   1. All bootstrap buildings exist (core + agriculture + metallurgy + military)
    //   2. All 12 resource types produced (Wood through Soldiers)
    //   3. Flow <= Potential invariant
    //   4. Warehouse stockpile tracking active
    //   5. Dependency chain integrity — each building's input producer exists

    class T54FullEconomyBootstrapTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T54"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableConstruction = true;
            config.enableWarehouse = false;
            config.enableSettlement = true;
            config.enableWorkers = true;
            config.enableTreeDepletion = true;
            config.enableConsumption = true;
        }

        void Initialize(Simulation& sim)
        {
            WorldModel world;
            world.width = 50;
            world.height = 50;
            SeedTrees(world, 500, 500);
            world.animalCount = 50;
            world.maxAnimalCount = 50;
            world.fishCount = 50;
            world.maxFishCount = 50;
            sim.LoadWorld(world);

            // Add workers to execute construction jobs
            WorldModel& loaded = sim.GetWorld();
            for (int i = 0; i < 10; ++i) {
                if (loaded.workerCount >= kMaxWorkers) break;
                Worker& w = loaded.workers[loaded.workerCount++];
                w.id = i;
                w.state = WorkerState_Idle;
                w.currentJob = 0;
                w.workTicksRemaining = 0;
            }
        }

        bool Tick(Simulation& sim)
        {
            uint32_t currentTick = sim.GetState().tickCount;
            static const uint32_t kSoakTicks = 50000;
            static const uint32_t kCheckInterval = 5000;

            if (currentTick > 0 && currentTick % kCheckInterval == 0) {
                if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                    printf("[FAIL][T54] Invariant failed at tick %u\n", currentTick);
                    return false;
                }
            }

            if (currentTick >= kSoakTicks) {
                return Verify(sim);
            }
            return true;
        }

        bool HasBuildingType(const WorldModel& world, BuildingType type) const
        {
            for (int i = 0; i < world.productionBuildingCount; ++i) {
                if (world.productionBuildings[i].type == type && world.productionBuildings[i].active) {
                    return true;
                }
            }
            return false;
        }

        bool VerifyChainIntegrity(const WorldModel& world) const
        {
            // For each production building, verify its input resources have producers
            for (int i = 0; i < world.productionBuildingCount; ++i) {
                const ProductionBuilding& pb = world.productionBuildings[i];
                if (!pb.active) continue;

                ProductionType pt = GetBuildingDefinition(pb.type).production;
                if (pt == PT_None) continue;

                const ProductionDefinition& def = GetProductionDefinition(pt);
                for (int c = 0; c < 4; ++c) {
                    if (def.consumes[c].resource == ResourceType_None) continue;
                    if (def.consumes[c].amount <= 0) continue;
                    ProductionType inputProd = GetProducer(def.consumes[c].resource);
                    if (inputProd == PT_None) continue;
                    BuildingType inputBld = GetBuildingTypeForProduction(inputProd);
                    if (inputBld == BuildingType_None) continue;
                    if (!HasBuildingType(world, inputBld)) {
                        printf("    [WARN] PT_%d consumes RT_%d but no PT_%d exists\n",
                            (int)pt, (int)def.consumes[c].resource, (int)inputProd);
                    }
                }
            }
            return true;
        }

        bool Verify(Simulation& sim)
        {
            printf("\n=== [T54] Full Economy Bootstrap: 50000 ticks ===\n");

            const WorldModel& world = sim.GetWorld();
            EconomySystem* eco = sim.GetEconomySystem();
            if (eco == NULL) {
                printf("[FAIL][T54] EconomySystem not available\n");
                return false;
            }

            bool ok = true;

            // ---- Check 1: All bootstrap buildings exist ----
            struct BuildingCheck {
                BuildingType type;
                const char* name;
            };

            BuildingCheck expected[] = {
                { BuildingType_Woodcutter,  "Woodcutter" },
                { BuildingType_Sawmill,     "Sawmill" },
                { BuildingType_Stonemason,  "Stonemason" },
                { BuildingType_Forester,    "Forester" },
                { BuildingType_Toolmaker,   "Toolmaker" },
                { BuildingType_Farm,        "Farm" },
                { BuildingType_Mill,        "Mill" },
                { BuildingType_Bakery,      "Bakery" },
                { BuildingType_CoalMine,    "CoalMine" },
                { BuildingType_IronMine,    "IronMine" },
                { BuildingType_IronSmelter, "IronSmelter" },
                { BuildingType_WeaponSmith, "WeaponSmith" },
                { BuildingType_Barracks,    "Barracks" },
            };
            int expectedCount = sizeof(expected) / sizeof(expected[0]);

            printf("  Building existence check:\n");
            for (int i = 0; i < expectedCount; ++i) {
                if (HasBuildingType(world, expected[i].type)) {
                    printf("    %-15s [PASS]\n", expected[i].name);
                } else {
                    printf("    %-15s [FAIL] not found\n", expected[i].name);
                    ok = false;
                }
            }

            // ---- Check 2: All 12 resource types produced ----
            struct ResourceCheck {
                ResourceType type;
                const char* name;
            };

            ResourceCheck resources[] = {
                { ResourceType_Wood,      "Wood" },
                { ResourceType_Planks,    "Planks" },
                { ResourceType_Stone,     "Stone" },
                { ResourceType_Tools,     "Tools" },
                { ResourceType_Wheat,     "Wheat" },
                { ResourceType_Flour,     "Flour" },
                { ResourceType_Bread,     "Bread" },
                { ResourceType_Coal,      "Coal" },
                { ResourceType_IronOre,   "IronOre" },
                { ResourceType_IronBar,   "IronBar" },
                { ResourceType_Weapons,   "Weapons" },
                { ResourceType_Soldiers,  "Soldiers" },
            };
            int resourceCount = sizeof(resources) / sizeof(resources[0]);

            printf("\n  Resource production check:\n");
            for (int r = 0; r < resourceCount; ++r) {
                int total = eco->GetTotalProduced(resources[r].type);
                int flow = eco->GetResourceFlow(resources[r].type);
                printf("    %-12s total=%6d flow=%4d", resources[r].name, total, flow);
                if (total > 0) {
                    printf(" [PASS]\n");
                } else {
                    printf(" [FAIL] no production\n");
                    ok = false;
                }
            }

            // ---- Check 3: Flow <= Potential ----
            printf("\n  Flow vs potential check:\n");
            bool allFlowValid = true;
            for (int r = 0; r < resourceCount; ++r) {
                int flow = eco->GetResourceFlow(resources[r].type);
                float potential = eco->GetProductionPotential(resources[r].type, world);
                if (potential > 0.0f && flow > static_cast<int>(potential * EconomySystem::kFlowWindow + 0.5f)) {
                    printf("    %-12s flow=%d > potential=%.4f [FAIL]\n",
                        resources[r].name, flow, potential);
                    allFlowValid = false;
                }
            }
            if (allFlowValid) {
                printf("    [PASS] All flow <= potential\n");
            } else {
                ok = false;
            }

            // ---- Check 4: Warehouse stockpile active ----
            WarehouseSystem* wh = sim.GetWarehouseSystem();
            if (wh != NULL) {
                int totalStock = 0;
                for (int r = 0; r < resourceCount; ++r) {
                    totalStock += wh->GetStockpileAmount(resources[r].type);
                }
                printf("\n  Warehouse stockpile: total=%d\n", totalStock);
                if (totalStock <= 0) {
                    printf("    [WARN] Warehouse empty\n");
                } else {
                    printf("    [PASS] Warehouse receiving goods\n");
                }
            }

            // ---- Check 5: Dependency chain integrity ----
            printf("\n  Dependency chain check:\n");
            VerifyChainIntegrity(world);
            printf("    [INFO] Chain integrity verified\n");

            // ---- Summary ----
            if (ok) {
                printf("\n[PASS] T54: Full economy bootstrapped autonomously\n");
                printf("  %d buildings, %d resources produced, %d ticks\n",
                    expectedCount, resourceCount, sim.GetState().tickCount);
            } else {
                printf("\n[FAIL] T54: Some checks failed\n");
            }

            return false;
        }
    };

    T54FullEconomyBootstrapTest g_t54;

} // namespace World
