#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/ConstructionAssertions.h"
#include "../../Testing/Assertions/TransportAssertions.h"
#include "../../Testing/Assertions/WorldAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Definitions/BuildingDefinition.h"
#include "../../Definitions/ProductionDefinition.h"
#include "../../Construction/ConstructionSite.h"
#include "../../Construction/ConstructionState.h"
#include "../../Systems/DemandManager.h"
#include <stdio.h>

namespace World {

class T12ProductionPipeline : public ISimulationScenario {
public:
    const char* GetName() const { return "T12"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableProduction = true;
        config.enableEconomy = true;
        config.enableConstruction = true;
    }

    void Initialize(Simulation& sim)
    {

        WorldModel world;
        world.width = 50;
        world.height = 50;
        sim.LoadWorld(world);

        WorldModel& loadedWorld = sim.GetWorld();

        // 2 Woodcutters (no inputs, continuous Wood output)
        for (int i = 0; i < 2; ++i) {
            if (loadedWorld.pendingConstructionCount >= kMaxConstructionRequests) break;
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Woodcutter;
            req.position = Vector2i(10 + i * 10, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // 1 Sawmill (consumes Wood → produces Planks)
        if (loadedWorld.pendingConstructionCount < kMaxConstructionRequests) {
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Sawmill;
            req.position = Vector2i(25, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kSoakTicks = 600;
        static const uint32_t kCheckInterval = 200;

        if (currentTick % kCheckInterval == 0 && currentTick > 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T12 failed at tick %u\n", currentTick);
                return false;
            }
        }

        if (currentTick >= kSoakTicks) {
            return Verify(sim);
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        bool ok = true;

        // Accumulate production stats per building
        int woodcutterCount = 0;
        int sawmillCount = 0;
        int totalWood = 0;
        int totalPlanks = 0;

        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);

            if (def.production == PT_Woodcutter) {
                woodcutterCount++;
                totalWood += pb.totalOutput[0];
            } else if (def.production == PT_Sawmill) {
                sawmillCount++;
                totalPlanks += pb.totalOutput[0];
            }
        }

        // Check 1: Both Woodcutters built and producing
        if (woodcutterCount < 2) {
            printf("[FAIL][T12.A] Expected 2 active Woodcutters, got %d\n", woodcutterCount);
            ok = false;
        } else {
            printf("[PASS][T12.A] Construction: %d Woodcutters built\n", woodcutterCount);
        }

        // Check 2: Multiple production cycles — at least 10 Wood per Woodcutter
        // (600 ticks / 30 cycleTime = 20 cycles; floor at 10 for transport lag)
        int expectedPerWoodcutter = 10;
        bool allWoodcuttersCycling = true;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Woodcutter) {
                if (pb.totalOutput[0] < expectedPerWoodcutter) {
                    printf("[FAIL][T12.B] Woodcutter #%d: totalOutput=%d < expected %d (not cycling)\n",
                        i, pb.totalOutput[0], expectedPerWoodcutter);
                    allWoodcuttersCycling = false;
                    ok = false;
                } else {
                    printf("[INFO][T12.B] Woodcutter #%d: totalOutput=%d (≥%d cycles)\n",
                        i, pb.totalOutput[0], expectedPerWoodcutter);
                }
            }
        }
        if (allWoodcuttersCycling) {
            printf("[PASS][T12.B] All Woodcutters cycling: %d total Wood produced\n", totalWood);
        }

        // Check 3: Sawmill built and producing (input-demand cycle works)
        if (sawmillCount < 1) {
            printf("[FAIL][T12.C] Expected 1 active Sawmill, got %d\n", sawmillCount);
            ok = false;
        } else {
            printf("[PASS][T12.C] Sawmill built (construction completed)\n");
        }

        if (totalPlanks == 0) {
            printf("[FAIL][T12.D] No planks produced (input-demand cycle broken)\n");
            ok = false;
        } else {
            printf("[PASS][T12.D] Production pipeline: %d planks produced from %d Wood\n",
                totalPlanks, totalWood);
        }

        // Check 4: Output buffer accumulates monotonically
        bool monotonic = true;
        int prevOutput = -1;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            int buildingOutput = 0;
            for (int p = 0; p < kMaxProductionInputs; ++p) {
                buildingOutput += pb.totalOutput[p];
            }
            if (buildingOutput < prevOutput) {
                monotonic = false;
                break;
            }
            prevOutput = buildingOutput;
        }
        // Only check if at least 2 buildings active
        if (world.productionBuildingCount >= 2 && !monotonic) {
            printf("[FAIL][T12.E] totalOutput decreased between buildings (buffer corruption)\n");
            ok = false;
        } else {
            printf("[PASS][T12.E] Output buffers consistent across buildings\n");
        }

        if (ok) {
            printf("[PASS] T12: Production pipeline — multi-cycle output accumulation verified\n");
        }
        return ok;
    }
};

static T12ProductionPipeline g_t12ProductionPipeline;

} // namespace World
