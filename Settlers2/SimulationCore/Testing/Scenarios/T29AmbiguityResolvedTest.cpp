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

class T29AmbiguityResolvedTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T29"; }

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

        // Same setup as T28: 2 Sawmills, only 1 receives Wood.
        // Sawmill 1 starts with inputsRequested=true to prevent
        // ProductionSystem from creating transport demands for it.
        for (int i = 0; i < 2 && loadedWorld.productionBuildingCount < kMaxProductionBuildings; ++i) {
            ProductionBuilding& pb = loadedWorld.productionBuildings[loadedWorld.productionBuildingCount++];
            pb.type = BuildingType_Sawmill;
            pb.position = Vector2i(10 + i * 5, 10);
            pb.owner = 0;
            pb.active = true;
            pb.cycleTimer = 0;
            pb.inputsRequested = (i == 1);
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
            printf("[FAIL] T29 failed at tick %u\n", currentTick);
            return false;
        }

        WorldModel& world = sim.GetWorld();

        // Same supply pattern as T28: 2 Wood to Sawmill 0 every 30 ticks.
        // Sawmill 1 gets nothing.
        if (currentTick % 30 == 0) {
            for (int i = 0; i < world.productionBuildingCount; ++i) {
                ProductionBuilding& pb = world.productionBuildings[i];
                if (pb.type != BuildingType_Sawmill) continue;
                if (!pb.active) continue;
                if (i == 0) {
                    pb.inputDelivered[0] = 2;
                }
            }
        }

        if (currentTick >= 300) {
            bool ok = Verify(sim);
            return false;
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        EconomySystem* es = sim.GetEconomySystem();
        bool ok = true;

        if (es == NULL) {
            printf("[FAIL][T29] EconomySystem not available\n");
            return false;
        }

        int planksFlow = es->GetResourceFlow(ResourceType_Planks);
        float planksPotential = es->GetProductionPotential(ResourceType_Planks, world);
        float flowPerTick = (float)planksFlow / EconomySystem::kFlowWindow;

        printf("[T29] GetResourceFlow(Planks) = %d\n", planksFlow);
        printf("[T29] GetProductionPotential(Planks) = %.4f units/tick\n", planksPotential);
        printf("[T29] Flow per tick = %.4f\n", flowPerTick);

        if (planksFlow < 1) {
            printf("[FAIL][T29.A] GetResourceFlow(Planks) = %d (expected >= 1)\n", planksFlow);
            ok = false;
        } else {
            printf("[PASS][T29.A] Production is active (flow=%d)\n", planksFlow);
        }

        // Core assertion: unused capacity is detectable via potential > flow.
        // In T28 this was ambiguous (flow alone indistinguishable from 1 saturated Sawmill).
        // With potential added, (flow, potential) is injective for these two world states.
        // Sawmill cycleTime=30, produces 1 Plank per cycle.
        // 2 active Sawmills → potential = 2 * (1/30) ≈ 0.0667 Planks/tick.
        // Actual flow: 2 Wood per 30 ticks → max 1 Plank per 30 ticks → 0.0333/tick.
        // So flow < potential proves unused capacity.
        if (planksPotential <= flowPerTick + 0.0001f) {
            printf("[FAIL][T29.B] flow (%.4f/tick) >= potential (%.4f/tick) — "
                "unused capacity not detected\n",
                flowPerTick, planksPotential);
            ok = false;
        } else {
            printf("[PASS][T29.B] flow < potential: unused capacity detected "
                "(%.4f < %.4f)\n", flowPerTick, planksPotential);
        }

        // Verify the world state: 2 Sawmills, 1 idle
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

        if (sawmillCount < 2) {
            printf("[FAIL][T29.C] Expected >= 2 Sawmills, got %d\n", sawmillCount);
            ok = false;
        } else {
            printf("[PASS][T29.C] %d Sawmills exist\n", sawmillCount);
        }
        if (idleSawmillCount < 1) {
            printf("[FAIL][T29.D] Expected >= 1 idle Sawmill, got %d\n", idleSawmillCount);
            ok = false;
        } else {
            printf("[PASS][T29.D] %d Sawmill(s) idle — capacity present but starved\n", idleSawmillCount);
        }

        if (ok) {
            printf("[PASS] T29: Ambiguity resolved by GetProductionPotential\n");
            printf("  World: 2 Sawmills exist, 1 idle\n");
            printf("  flow=%.4f/tick, potential=%.4f/tick\n", flowPerTick, planksPotential);
            printf("  (flow, potential) distinguishes World B from World A\n");
            printf("  where World A has flow ~ potential (1 saturated Sawmill).\n");
        }
        return ok;
    }
};

static T29AmbiguityResolvedTest g_t29AmbiguityResolvedTest;

} // namespace World
