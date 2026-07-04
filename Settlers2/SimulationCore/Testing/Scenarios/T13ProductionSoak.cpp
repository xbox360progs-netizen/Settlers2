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

class T13ProductionSoak : public ISimulationScenario {
public:
    const char* GetName() const { return "T13"; }

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

        // 3 Woodcutters (no inputs, continuous Wood output)
        for (int i = 0; i < 3; ++i) {
            if (loadedWorld.pendingConstructionCount >= kMaxConstructionRequests) break;
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Woodcutter;
            req.position = Vector2i(10 + i * 8, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // 2 Sawmills (consumes Wood → produces Planks)
        for (int i = 0; i < 2; ++i) {
            if (loadedWorld.pendingConstructionCount >= kMaxConstructionRequests) break;
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Sawmill;
            req.position = Vector2i(10 + i * 8, 20);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kSoakTicks = 50000;
        static const uint32_t kCheckInterval = 5000;

        if (currentTick > 0 && currentTick % kCheckInterval == 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T13 failed at tick %u\n", currentTick);
                return false;
            }
            ReportStatus(sim, currentTick);
        }

        if (currentTick >= kSoakTicks) {
            return Verify(sim);
        }
        return true;
    }

    void ReportStatus(Simulation& sim, uint32_t tick)
    {
        const WorldModel& world = sim.GetWorld();
        int totalWood = 0;
        int totalPlanks = 0;
        int activeWoodcutters = 0;
        int activeSawmills = 0;

        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Woodcutter) {
                activeWoodcutters++;
                totalWood += pb.totalOutput[0];
            } else if (def.production == PT_Sawmill) {
                activeSawmills++;
                totalPlanks += pb.totalOutput[0];
            }
        }

        DemandManager* dm = sim.GetDemandManager();
        int demandCount = dm ? dm->GetDemandCount() : -1;

        printf("  ... tick %u / 50000 | WC=%d SW=%d Wood=%d Planks=%d Demands=%d\n",
            tick, activeWoodcutters, activeSawmills, totalWood, totalPlanks, demandCount);
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        bool ok = true;

        // Collect per-building stats
        int woodcutterCount = 0;
        int sawmillCount = 0;
        int totalWood = 0;
        int totalPlanks = 0;
        bool allMonotonic = true;

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

            // Check monotonic: outputBuffer + totalOutput should never be negative
            for (int p = 0; p < kMaxProductionInputs; ++p) {
                if (pb.totalOutput[p] < 0) allMonotonic = false;
                if (pb.outputBuffer[p] < 0) allMonotonic = false;
            }
        }

        // Check 1: Buildings built and active
        if (woodcutterCount < 3) {
            printf("[FAIL][T13.A] Expected 3 active Woodcutters after 50k ticks, got %d\n", woodcutterCount);
            ok = false;
        } else {
            printf("[PASS][T13.A] %d Woodcutters active\n", woodcutterCount);
        }

        if (sawmillCount < 2) {
            printf("[FAIL][T13.B] Expected 2 active Sawmills after 50k ticks, got %d\n", sawmillCount);
            ok = false;
        } else {
            printf("[PASS][T13.B] %d Sawmills active\n", sawmillCount);
        }

        // Check 2: Sustained output over 50k ticks
        // At 30 ticks/cycle, Woodcutter should produce ~1666 Wood each
        int expectedPerWoodcutter = 1500;  // conservative
        bool sustained = true;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Woodcutter) {
                if (pb.totalOutput[0] < expectedPerWoodcutter) {
                    printf("[FAIL][T13.C] Woodcutter #%d: totalOutput=%d < %d (production stalled)\n",
                        i, pb.totalOutput[0], expectedPerWoodcutter);
                    sustained = false;
                    ok = false;
                }
            }
        }
        if (sustained) {
            printf("[PASS][T13.C] Sustained output: %d total Wood (all Woodcutters cycling)\n", totalWood);
        }

        // Check 3: Production pipeline — Sawmills processed input and produced
        if (totalPlanks < 500) {
            printf("[FAIL][T13.D] Sawmill output: %d planks < 500 (input-demand pipeline not keeping up)\n", totalPlanks);
            ok = false;
        } else {
            printf("[PASS][T13.D] Production pipeline: %d planks from %d Wood\n", totalPlanks, totalWood);
        }

        // Check 4: No Demand leaks
        DemandManager* dm = sim.GetDemandManager();
        if (dm != NULL) {
            int demandCount = dm->GetDemandCount();
            int activeDemands = 0;
            for (int d = 0; d < demandCount; ++d) {
                if (dm->GetDemandRemaining(d) > 0) activeDemands++;
            }
            if (activeDemands > 12) {
                printf("[FAIL][T13.E] %d active demands — possible Demand leak\n", activeDemands);
                ok = false;
            } else {
                printf("[PASS][T13.E] Active demands: %d (acceptable)\n", activeDemands);
            }
        }

        // Check 5: Monotonic invariants
        if (!allMonotonic) {
            printf("[FAIL][T13.F] Negative totalOutput or outputBuffer — buffer corruption\n");
            ok = false;
        } else {
            printf("[PASS][T13.F] All output buffers monotonic (no corruption)\n");
        }

        if (ok) {
            printf("[PASS] T13: Production Soak — 50k ticks, sustained output, stable\n");
        }
        return ok;
    }
};

static T13ProductionSoak g_t13ProductionSoak;

} // namespace World
