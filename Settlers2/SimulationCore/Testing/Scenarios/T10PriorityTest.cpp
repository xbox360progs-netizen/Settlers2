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

class T10PriorityTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T10"; }

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

        // Two Woodcutters (construction)
        for (int i = 0; i < 2; ++i) {
            if (loadedWorld.pendingConstructionCount >= kMaxConstructionRequests) break;
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Woodcutter;
            req.position = Vector2i(10, 10 + i * 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // Sawmill (production input demand)
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

        if (currentTick % 500 == 0) {
            printf("  ... tick %u / 4000\n", currentTick);
        }

        if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
            printf("[FAIL] T10 failed at tick %u\n", currentTick);
            return false;
        }

        if (currentTick >= 4000) {
            return Verify(sim);
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        bool ok = true;

        // ── Check 1: PriorityForReason returns correct values ────────
        if (PriorityForReason(TTR_Construction) != TBP_High) {
            printf("[FAIL][T10.A] PriorityForReason(TTR_Construction) = %u, expected %u (TBP_High)\n",
                PriorityForReason(TTR_Construction), TBP_High);
            ok = false;
        } else {
            printf("[PASS][T10.A] PriorityForReason(TTR_Construction) = TBP_High (%u)\n", TBP_High);
        }

        if (PriorityForReason(TTR_Production) != TBP_Normal) {
            printf("[FAIL][T10.A] PriorityForReason(TTR_Production) = %u, expected %u (TBP_Normal)\n",
                PriorityForReason(TTR_Production), TBP_Normal);
            ok = false;
        } else {
            printf("[PASS][T10.A] PriorityForReason(TTR_Production) = TBP_Normal (%u)\n", TBP_Normal);
        }

        // ── Check 2: Construction completing (buildings built) ────────
        int woodcutterCount = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Woodcutter) {
                woodcutterCount++;
            }
        }

        if (woodcutterCount < 2) {
            printf("[FAIL][T10.B] Expected 2 active Woodcutters (construction complete), got %d\n", woodcutterCount);
            ok = false;
        } else {
            printf("[PASS][T10.B] Construction: %d Woodcutters built\n", woodcutterCount);
        }

        // ── Check 3: Production completing (sawmill active, planks produced) ──
        int sawmillCount = 0;
        int planksProduced = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Sawmill) {
                sawmillCount++;
                planksProduced += pb.totalOutput[0];
            }
        }

        if (sawmillCount < 1) {
            printf("[FAIL][T10.C] Expected 1 active Sawmill, got %d\n", sawmillCount);
            ok = false;
        } else {
            printf("[PASS][T10.C] Sawmill built (TTR_Construction → TBP_High transport completed)\n");
        }

        if (planksProduced == 0) {
            printf("[FAIL][T10.C] No planks produced (TTR_Production → TBP_Normal transport failed)\n");
            ok = false;
        } else {
            printf("[PASS][T10.C] Production: %d planks produced (TTR_Production → TBP_Normal transport completed)\n", planksProduced);
        }

        // ── Check 4: Both priority levels coexist ─────────────────────
        int constructionTransportCount = 0;
        int productionTransportCount = 0;
        for (int i = 0; i < world.pendingRequestCount; ++i) {
            const TransportRequest& req = world.pendingRequests[i];
            if (req.reason == TTR_Construction) constructionTransportCount++;
            if (req.reason == TTR_Production) productionTransportCount++;
        }

        if (constructionTransportCount == 0) {
            printf("[FAIL][T10.D] No TTR_Construction transport requests\n");
            ok = false;
        }
        if (productionTransportCount == 0) {
            printf("[FAIL][T10.D] No TTR_Production transport requests\n");
            ok = false;
        }
        if (constructionTransportCount > 0 && productionTransportCount > 0) {
            printf("[PASS][T10.D] Both priority levels coexist: %d Construction (%u), %d Production (%u)\n",
                constructionTransportCount, TBP_High,
                productionTransportCount, TBP_Normal);
        }

        if (ok) {
            printf("[PASS] T10: Priority ordering — all checks passed\n");
        }
        return ok;
    }
};

static T10PriorityTest g_t10PriorityTest;

} // namespace World
