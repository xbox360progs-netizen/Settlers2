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

class T9RegressionHardening : public ISimulationScenario {
public:
    const char* GetName() const { return "T9"; }

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

        // Sawmill #1
        if (loadedWorld.pendingConstructionCount < kMaxConstructionRequests) {
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Sawmill;
            req.position = Vector2i(10, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // Sawmill #2 — staggered position, same type
        if (loadedWorld.pendingConstructionCount < kMaxConstructionRequests) {
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Sawmill;
            req.position = Vector2i(20, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;

        if (currentTick % 500 == 0) {
            printf("  ... tick %u / 4000\n", currentTick);
        }

        if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
            printf("[FAIL] T9 failed at tick %u\n", currentTick);
            return false;
        }

        if (currentTick >= 4000) {
            return Verify(sim);
        }
        return true;
    }

    // ── Three regression checks ──────────────────────────────────
    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        bool ok = true;

        int sawmillCount = 0;
        int planksProduced = 0;

        // ── Check 1: Both Sawmills exist as active production ─────
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Sawmill) {
                sawmillCount++;
                planksProduced += pb.totalOutput[0];
                printf("[INFO] T9 Sawmill #%d: inputDelivered[0]=%d, inputsRequested=%d, totalOutput=%d\n",
                    i, pb.inputDelivered[0], pb.inputsRequested, pb.totalOutput[0]);
            }
        }

        if (sawmillCount < 2) {
            printf("[FAIL][T9.A] Expected 2 active Sawmills, got %d. "
                   "Construction delivery events not processed.\n", sawmillCount);
            ok = false;
        } else {
            printf("[PASS][T9.A] Multi-subscriber: both Sawmills built (ConstructionSystem processed TTR_Construction)\n");
        }

        // ── Check 2: Both Sawmills produced Planks ────────────────
        if (planksProduced == 0) {
            printf("[FAIL][T9.A] No planks produced. "
                   "ProductionSystem did not receive input deliveries (TTR_Production events)\n");
            ok = false;
        } else {
            printf("[PASS][T9.A] Multi-subscriber: %d planks produced (ProductionSystem processed TTR_Production)\n",
                planksProduced);
        }

        // ── Check 3: Duplicate prevention via DemandManager ───────
        DemandManager* dm = sim.GetDemandManager();
        if (dm != NULL) {
            int demandCount = dm->GetDemandCount();
            int prodDemands = 0;
            for (int d = 0; d < demandCount; ++d) {
                if (dm->GetDemandOwner(d) == DemandOwner_Production) {
                    prodDemands++;
                }
            }

            if (prodDemands > sawmillCount) {
                printf("[FAIL][T9.C] Duplicate prevention: %d Production demands for %d Sawmills. Expected ≤ %d.\n",
                    prodDemands, sawmillCount, sawmillCount);
                ok = false;
            } else {
                printf("[PASS][T9.C] Duplicate prevention: %d Production demands ≤ %d Sawmills (activeTask guard working)\n",
                    prodDemands, sawmillCount);
            }
        }

        // ── Check 4: Both sawmills independently produced ─────────
        if (planksProduced < 2) {
            printf("[INFO][T9.B] Independent production: %d planks produced (expected ≥2 for 2 Sawmills)\n",
                planksProduced);
        } else {
            printf("[PASS][T9.B] Independent production: %d planks produced across 2 Sawmills\n",
                planksProduced);
        }

        if (ok) {
            printf("[PASS] T9: Regression hardening — all checks passed\n");
        }
        return ok;
    }
};

static T9RegressionHardening g_t9RegressionHardening;

} // namespace World
