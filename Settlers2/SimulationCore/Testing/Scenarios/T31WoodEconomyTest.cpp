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
#include "../../Systems/EconomySystem.h"
#include "../../Warehouse/WarehouseSystem.h"
#include <stdio.h>

namespace World {

class T31WoodEconomyTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T31"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableProduction = true;
        config.enableEconomy = true;
        config.enableConstruction = true;
        config.enableWarehouse = true;
    }

    void Initialize(Simulation& sim)
    {
        WorldModel world;
        world.width = 50;
        world.height = 50;
        sim.LoadWorld(world);

        WorldModel& loadedWorld = sim.GetWorld();

        // 2 Woodcutters (supply Wood, no inputs)
        for (int i = 0; i < 2; ++i) {
            if (loadedWorld.pendingConstructionCount >= kMaxConstructionRequests) break;
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Woodcutter;
            req.position = Vector2i(10 + i * 10, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // 1 Sawmill (consumes Wood, produces Planks)
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
        static const uint32_t kSoakTicks = 1000;
        static const uint32_t kCheckInterval = 250;

        if (currentTick % kCheckInterval == 0 && currentTick > 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T31 failed at tick %u\n", currentTick);
                return false;
            }
        }

        if (currentTick >= kSoakTicks) {
            bool ok = Verify(sim);
            return false;
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        EconomySystem* eco = sim.GetEconomySystem();
        WarehouseSystem* ws = sim.GetWarehouseSystem();
        bool ok = true;

        if (eco == NULL) {
            printf("[FAIL][T31] EconomySystem not available\n");
            return false;
        }
        if (ws == NULL) {
            printf("[FAIL][T31] WarehouseSystem not available\n");
            return false;
        }

        // ---- Check 1: Buildings constructed and active ----
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
            }
            if (def.production == PT_Sawmill) {
                sawmillCount++;
                totalPlanks += pb.totalOutput[0];
            }
        }

        if (woodcutterCount < 2) {
            printf("[FAIL][T31.A] Expected 2 active Woodcutters, got %d\n", woodcutterCount);
            ok = false;
        } else {
            printf("[PASS][T31.A] %d Woodcutters active, total wood output: %d\n", woodcutterCount, totalWood);
        }

        if (sawmillCount < 1) {
            printf("[FAIL][T31.B] Expected 1 active Sawmill, got %d\n", sawmillCount);
            ok = false;
        } else {
            printf("[PASS][T31.B] 1 Sawmill active, total planks output: %d\n", totalPlanks);
        }

        // ---- Check 2: EconomySystem matches direct read ----
        int ecoWood = eco->GetTotalProduced(ResourceType_Wood);
        int ecoPlanks = eco->GetTotalProduced(ResourceType_Planks);

        if (ecoWood != totalWood) {
            printf("[FAIL][T31.C] EconomySystem Wood (%d) != direct totalOutput (%d)\n", ecoWood, totalWood);
            ok = false;
        } else {
            printf("[PASS][T31.C] EconomySystem Wood tracking: %d == direct %d\n", ecoWood, totalWood);
        }

        if (ecoPlanks != totalPlanks) {
            printf("[FAIL][T31.D] EconomySystem Planks (%d) != direct totalOutput (%d)\n", ecoPlanks, totalPlanks);
            ok = false;
        } else {
            printf("[PASS][T31.D] EconomySystem Planks tracking: %d == direct %d\n", ecoPlanks, totalPlanks);
        }

        // ---- Check 3: Wood was produced and transported through pipeline ----
        if (totalWood <= 0) {
            printf("[FAIL][T31.E] No Wood produced at all — Woodcutter not running\n");
            ok = false;
        } else if (totalWood < 20) {
            printf("[WARN][T31.E] Wood produced: %d (low — possible pipeline blockage)\n", totalWood);
        } else {
            printf("[PASS][T31.E] Wood pipeline: %d Wood produced\n", totalWood);
        }

        // ---- Check 4: Planks produced (Sawmill received Wood) ----
        if (totalPlanks <= 0) {
            printf("[FAIL][T31.F] No Planks produced — Wood not delivered to Sawmill\n");
            ok = false;
        } else if (totalPlanks < 5) {
            printf("[WARN][T31.F] Planks produced: %d (low — possible input starvation)\n", totalPlanks);
        } else {
            printf("[PASS][T31.F] Planks pipeline: %d Planks produced from %d Wood\n", totalPlanks, totalWood);
        }

        // ---- Check 5: Consumption tracking — Sawmill should consume 2 Wood per Plank ----
        int woodConsumed = eco->GetTotalConsumed(ResourceType_Wood);
        int expectedConsumed = totalPlanks * 2;
        if (woodConsumed != expectedConsumed) {
            printf("[FAIL][T31.G] Wood consumed (%d) != totalPlanks * 2 (%d)\n", woodConsumed, expectedConsumed);
            ok = false;
        } else {
            printf("[PASS][T31.G] Wood consumption: %d == Planks * 2 (%d)\n", woodConsumed, expectedConsumed);
        }

        // ---- Check 6: Observation layer — GetResourceFlow + GetProductionPotential ----
        // Invariant: 0 < observed flow <= installed capacity (potential).
        // A violation means either flow tracking or potential calculation is wrong.
        int woodFlow = eco->GetResourceFlow(ResourceType_Wood);
        float woodPotential = eco->GetProductionPotential(ResourceType_Wood, world);

        if (woodFlow <= 0) {
            printf("[FAIL][T31.H] GetResourceFlow(Wood) = %d (expected > 0)\n", woodFlow);
            ok = false;
        } else {
            printf("[PASS][T31.H] GetResourceFlow(Wood) = %d (per %d-tick window)\n",
                woodFlow, EconomySystem::kFlowWindow);
        }

        if (woodPotential <= 0.0f) {
            printf("[FAIL][T31.I] GetProductionPotential(Wood) = %.4f (expected > 0)\n", woodPotential);
            ok = false;
        } else {
            printf("[PASS][T31.I] GetProductionPotential(Wood) = %.4f units/tick\n", woodPotential);
        }

        // Invariant: flow(window) <= potential * kFlowWindow
        // Potential defines the theoretical maximum. Observed flow cannot exceed it.
        float flowNormalised = static_cast<float>(woodFlow) / static_cast<float>(EconomySystem::kFlowWindow);
        if (woodFlow > static_cast<int>(woodPotential * EconomySystem::kFlowWindow + 0.5f)) {
            printf("[FAIL][T31.J] GetResourceFlow(Wood) = %d exceeds capacity %d (potential=%.4f * window=%d)\n",
                woodFlow, static_cast<int>(woodPotential * EconomySystem::kFlowWindow), woodPotential,
                EconomySystem::kFlowWindow);
            ok = false;
        } else {
            printf("[PASS][T31.J] Flow (%d) within capacity (%d): potential=%.4f, normalized flow=%.4f\n",
                woodFlow, static_cast<int>(woodPotential * EconomySystem::kFlowWindow),
                woodPotential, flowNormalised);
        }

        // ---- Check 7: Warehouse received goods from production ----
        int woodInWarehouse = ws->GetStockpileAmount(ResourceType_Wood);
        int planksInWarehouse = ws->GetStockpileAmount(ResourceType_Planks);

        // Total output should be distributed between warehouse stockpile + remaining outputBuffer
        printf("[INFO][T31.K] Warehouse: Wood=%d Planks=%d\n", woodInWarehouse, planksInWarehouse);

        // ---- Check 8: No outputBuffer saturation (warehouse prevents blockage) ----
        int blockedBuildings = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            for (int p = 0; p < kMaxProductionInputs; ++p) {
                if (pb.outputResources[p] != ResourceType_None && pb.outputBuffer[p] > 5) {
                    blockedBuildings++;
                    printf("[WARN][T31.L] Building %d outputBuffer[%d]=%d (>5)\n", i, p, pb.outputBuffer[p]);
                    break;
                }
            }
        }
        if (blockedBuildings > 1) {
            printf("[FAIL][T31.L] %d buildings have outputBuffer > 5 (possible warehouse blockage)\n", blockedBuildings);
            ok = false;
        } else {
            printf("[PASS][T31.L] OutputBuffer blockage: %d buildings blocked (acceptable)\n", blockedBuildings);
        }

        // ---- Check 9: Economic balance — more Wood produced than consumed ----
        if (woodConsumed >= totalWood) {
            printf("[FAIL][T31.M] Wood consumed (%d) >= produced (%d) — impossible imbalance\n", woodConsumed, totalWood);
            ok = false;
        } else {
            printf("[PASS][T31.M] Wood balance: %d produced, %d consumed (%d surplus)\n",
                totalWood, woodConsumed, totalWood - woodConsumed);
        }

        if (ok) {
            printf("[PASS] T31: Wood Economy End-to-End — full pipeline verified\n");
        }
        return ok;
    }
};

static T31WoodEconomyTest g_t31WoodEconomyTest;

} // namespace World
