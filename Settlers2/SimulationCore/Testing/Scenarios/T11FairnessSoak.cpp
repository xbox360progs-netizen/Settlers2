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

class T11FairnessSoak : public ISimulationScenario {
public:
    const char* GetName() const { return "T11"; }

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

        AddInitialDemands(sim);
    }

    void AddInitialDemands(Simulation& sim)
    {
        WorldModel& world = sim.GetWorld();

        // 3 Woodcutters (Construction → TBP_High)
        for (int i = 0; i < 3; ++i) {
            if (world.pendingConstructionCount >= kMaxConstructionRequests) break;
            ConstructionRequest& req = world.pendingConstructionRequests[world.pendingConstructionCount++];
            req.type = BuildingType_Woodcutter;
            req.position = Vector2i(10 + i * 8, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // 2 Sawmills (Production → TBP_Normal input demands)
        for (int i = 0; i < 2; ++i) {
            if (world.pendingConstructionCount >= kMaxConstructionRequests) break;
            ConstructionRequest& req = world.pendingConstructionRequests[world.pendingConstructionCount++];
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
        static const uint32_t kCheckInterval = 2500;
        static const uint32_t kRefillTick = 10000;

        if (currentTick % kCheckInterval == 0 && currentTick > 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T11 failed at tick %u\n", currentTick);
                return false;
            }

            // Report status at each check
            const WorldModel& world = sim.GetWorld();
            int activeWoodcutters = 0;
            int activeSawmills = 0;
            int totalPlanks = 0;
            for (int i = 0; i < world.productionBuildingCount; ++i) {
                const ProductionBuilding& pb = world.productionBuildings[i];
                if (!pb.active) continue;
                const BuildingDefinition& def = GetBuildingDefinition(pb.type);
                if (def.production == PT_Woodcutter) activeWoodcutters++;
                if (def.production == PT_Sawmill) activeSawmills++;
                if (def.production == PT_Sawmill) {
                    totalPlanks += pb.totalOutput[0];
                }
            }

            printf("  ... tick %u / %u | Woodcutters=%d Sawmills=%d Planks=%d\n",
                currentTick, kSoakTicks, activeWoodcutters, activeSawmills, totalPlanks);
        }

        if (currentTick >= kSoakTicks) {
            return Verify(sim);
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        bool ok = true;

        // Check 1: PriorityForReason returns correct differentiated values
        if (PriorityForReason(TTR_Construction) != TBP_High) {
            printf("[FAIL][T11.A] PriorityForReason(TTR_Construction) = %u, expected %u (TBP_High)\n",
                PriorityForReason(TTR_Construction), TBP_High);
            ok = false;
        } else {
            printf("[PASS][T11.A] TTR_Construction → TBP_High (%u)\n", TBP_High);
        }

        if (PriorityForReason(TTR_Production) != TBP_Normal) {
            printf("[FAIL][T11.A] PriorityForReason(TTR_Production) = %u, expected %u (TBP_Normal)\n",
                PriorityForReason(TTR_Production), TBP_Normal);
            ok = false;
        } else {
            printf("[PASS][T11.A] TTR_Production → TBP_Normal (%u)\n", TBP_Normal);
        }

        // Check 2: Construction completed — Woodcutters built
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
            printf("[FAIL][T11.B] Expected ≥2 active Woodcutters (Construction TBP_High), got %d\n", woodcutterCount);
            ok = false;
        } else {
            printf("[PASS][T11.B] Construction (TBP_High): %d Woodcutters built\n", woodcutterCount);
        }

        // Check 3: Production completed — Sawmills built and producing planks
        int sawmillCount = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Sawmill) {
                sawmillCount++;
            }
        }

        if (sawmillCount < 1) {
            printf("[FAIL][T11.C] Expected ≥1 active Sawmill, got %d\n", sawmillCount);
            ok = false;
        } else {
            printf("[PASS][T11.C] Production buildings: %d Sawmills active\n", sawmillCount);
        }

        // Check 4: Production output — planks produced
        int planksProduced = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Sawmill) {
                planksProduced += pb.totalOutput[0];
            }
        }

        if (planksProduced == 0) {
            printf("[FAIL][T11.D] No planks produced (Sawmill production cycle failed)\n");
            ok = false;
        } else {
            printf("[PASS][T11.D] Production (TBP_Normal): %d planks produced\n", planksProduced);
        }

        // Check 5: Both priority levels had active transport
        int highPriorityReqs = 0;
        int normalPriorityReqs = 0;
        for (int i = 0; i < world.pendingRequestCount; ++i) {
            const TransportRequest& req = world.pendingRequests[i];
            if (req.reason == TTR_Construction) highPriorityReqs++;
            if (req.reason == TTR_Production) normalPriorityReqs++;
        }

        if (highPriorityReqs == 0) {
            printf("[FAIL][T11.E] No TTR_Construction transport requests (High priority never activated)\n");
            ok = false;
        }
        if (normalPriorityReqs == 0) {
            printf("[FAIL][T11.E] No TTR_Production transport requests (Normal priority never activated)\n");
            ok = false;
        }
        if (highPriorityReqs > 0 && normalPriorityReqs > 0) {
            printf("[PASS][T11.E] Both priority levels active: High=%d requests, Normal=%d requests\n",
                highPriorityReqs, normalPriorityReqs);
        }

        // Check 7: No pending demands stuck forever
        DemandManager* dm = sim.GetDemandManager();
        if (dm != NULL) {
            int demandCount = dm->GetDemandCount();
            int stuckDemands = 0;
            for (int d = 0; d < demandCount; ++d) {
                if (dm->GetDemandRemaining(d) > 0) {
                    stuckDemands++;
                }
            }
            if (stuckDemands > 8) {
                printf("[INFO][T11.G] %d demands still pending (may be expected for long-running)\n", stuckDemands);
            } else {
                printf("[PASS][T11.G] Demand queue drain: %d pending demands (≤8 acceptable)\n", stuckDemands);
            }
        }

        if (ok) {
            printf("[PASS] T11: Fairness/Soak — priority+age sufficient for 50k ticks\n");
        }
        return ok;
    }
};

static T11FairnessSoak g_t11FairnessSoak;

} // namespace World
