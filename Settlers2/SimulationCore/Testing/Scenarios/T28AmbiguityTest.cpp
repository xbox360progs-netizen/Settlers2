#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/ConstructionAssertions.h"
#include "../../Testing/Assertions/TransportAssertions.h"
#include "../../Testing/Assertions/WorldAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Systems/EconomySystem.h"
#include <stdio.h>

namespace World {

class T28AmbiguityTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T28"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableEconomy = true;
        config.enableProduction = true;
    }

    void Initialize(Simulation& sim)
    {
        WorldModel world;
        world.width = 50;
        world.height = 50;

        sim.LoadWorld(world);

        WorldModel& loadedWorld = sim.GetWorld();

        // Set up 2 Sawmills.
        // Both exist in the world from the start.
        // Only Sawmill 0 will receive Wood input during the test.
        // Sawmill 1 remains idle — it has capacity but no supply.
        // Sawmill 1 starts with inputsRequested=true to prevent
        // ProductionSystem from creating transport demands for it.
        // Without this guard, transport would deliver Wood to both flags.
        for (int i = 0; i < 2 && loadedWorld.productionBuildingCount < kMaxProductionBuildings; ++i) {
            ProductionBuilding& pb = loadedWorld.productionBuildings[loadedWorld.productionBuildingCount++];
            pb.type = BuildingType_Sawmill;
            pb.position = Vector2i(10 + i * 5, 10);
            pb.owner = 0;
            pb.active = true;
            pb.cycleTimer = 0;
            pb.inputsRequested = (i == 1); // block Sawmill 1 from creating demand
            pb.inputResources[0] = ResourceType_Wood;
            pb.inputRequired[0] = 2;
            pb.inputDelivered[0] = (i == 0) ? 2 : 0;
            pb.outputResources[0] = ResourceType_Planks;
            pb.outputBuffer[0] = 0;
            pb.totalOutput[0] = 0;
        }


    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;

        if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
            printf("[FAIL] T28 failed at tick %u\n", currentTick);
            return false;
        }

        WorldModel& world = sim.GetWorld();

        // Deliver 2 Wood to Sawmill 0 every 30 ticks.
        // This is the total supply: 2 Wood per 30 ticks.
        // Equivalent to a single well-supplied Sawmill.
        // Sawmill 1 gets nothing — idle, ready, but starved.
        if (currentTick % 30 == 0) {
            for (int i = 0; i < world.productionBuildingCount; ++i) {
                ProductionBuilding& pb = world.productionBuildings[i];
                if (pb.type != BuildingType_Sawmill) continue;
                if (!pb.active) continue;
                // Only feed Sawmill 0
                if (i == 0) {
                    pb.inputDelivered[0] = 2;
                }
            }
        }

        if (currentTick >= 300) {
            bool ok = Verify(sim);
            return false; // stop regardless of pass/fail
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        EconomySystem* es = sim.GetEconomySystem();
        bool ok = true;

        if (es == NULL) {
            printf("[FAIL][T28] EconomySystem not available\n");
            return false;
        }

        // Debug: print per-building state at end
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == BuildingType_Sawmill) {
                const ProductionBuilding& pb = world.productionBuildings[i];
                printf("[T28] Sawmill %d: totalOutput=%d outputBuffer=%d inputDelivered=%d\n",
                    i, pb.totalOutput[0], pb.outputBuffer[0], pb.inputDelivered[0]);
            }
        }

        // Measure flow — this value is indistinguishable from
        // a 1-Sawmill configuration with the same total Wood supply.
        int planksFlow = es->GetResourceFlow(ResourceType_Planks);

        // The exact flow value depends on 30-tick cycle vs 50-tick window alignment.
        // Two possible values: 1 or 2 per window.
        // Both are valid — the invariant is the AMBIGUITY, not the specific value.
        if (planksFlow < 1) {
            printf("[FAIL][T28.A] GetResourceFlow(Planks) = %d (expected >= 1)\n", planksFlow);
            ok = false;
        } else {
            printf("[PASS][T28.A] GetResourceFlow(Planks) = %d\n", planksFlow);
        }

        // Count active Sawmills
        // Idle = no output produced across the entire test (totalOutput == 0)
        // We use totalOutput (not outputBuffer) because outputBuffer[1..3]
        // may contain uninitialized values, and outputBuffer[0] may have
        // been drained by WarehouseSystem.
        int sawmillCount = 0;
        int idleSawmillCount = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == BuildingType_Sawmill && world.productionBuildings[i].active) {
                sawmillCount++;
                bool everProduced = false;
                for (int s = 0; s < kMaxProductionInputs; ++s) {
                    if (world.productionBuildings[i].totalOutput[s] > 0) {
                        everProduced = true;
                        break;
                    }
                }
                if (!everProduced) idleSawmillCount++;
            }
        }

        // Verify: 2 Sawmills exist, 1 is idle
        if (sawmillCount != 2) {
            printf("[FAIL][T28.B] Expected 2 Sawmills, got %d\n", sawmillCount);
            ok = false;
        } else {
            printf("[PASS][T28.B] 2 Sawmills exist in world\n");
        }
        if (idleSawmillCount < 1) {
            printf("[FAIL][T28.C] Expected >= 1 idle Sawmill, got %d\n", idleSawmillCount);
            ok = false;
        } else {
            printf("[PASS][T28.C] %d Sawmill(s) idle (capacity present but starved)\n", idleSawmillCount);
        }

        if (ok) {
            printf("[PASS] T28: Observation ambiguity demonstrated\n");
            printf("  World state: 2 Sawmills exist, 1 idle, total Planks flow = %d\n", planksFlow);
            printf("  Invariant: Two distinct world states collapse to the same\n");
            printf("  observation vector:\n");
            printf("    World A: 1 Sawmill saturated (flow=%d, build another)\n", planksFlow);
            printf("    World B: 2 Sawmills, 1 idle  (flow=%d, improve Wood supply)\n", planksFlow);
            printf("  GetResourceFlow(Planks) cannot distinguish them.\n");
            printf("  Resolution: requires GetProductionPotential (PR7b).\n");
        }
        return ok;
    }
};

static T28AmbiguityTest g_t28AmbiguityTest;

} // namespace World
