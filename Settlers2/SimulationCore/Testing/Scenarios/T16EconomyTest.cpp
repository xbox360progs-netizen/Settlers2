#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/ConstructionAssertions.h"
#include "../../Testing/Assertions/TransportAssertions.h"
#include "../../Testing/Assertions/WorldAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Definitions/BuildingDefinition.h"
#include "../../Definitions/ProductionDefinition.h"
#include "../../Systems/EconomySystem.h"
#include "../../Construction/ConstructionSite.h"
#include "../../Construction/ConstructionState.h"
#include <stdio.h>

namespace World {

class T16EconomyTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T16"; }

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

        // 2 Woodcutters (produce Wood)
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
        static const uint32_t kSoakTicks = 500;
        static const uint32_t kCheckInterval = 100;

        if (currentTick % kCheckInterval == 0 && currentTick > 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T16 failed at tick %u\n", currentTick);
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
        EconomySystem* eco = sim.GetEconomySystem();
        if (eco == NULL) {
            printf("[FAIL][T16] EconomySystem not available\n");
            return false;
        }

        bool ok = true;

        // Check 1: Buildings constructed and active
        int woodcutterCount = 0;
        int sawmillCount = 0;
        int woodProducedDirect = 0;
        int planksProducedDirect = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Woodcutter) {
                woodcutterCount++;
                woodProducedDirect += pb.totalOutput[0];
            }
            if (def.production == PT_Sawmill) {
                sawmillCount++;
                planksProducedDirect += pb.totalOutput[0];
            }
        }

        if (woodcutterCount < 2) {
            printf("[FAIL][T16.A] Expected 2 active Woodcutters, got %d\n", woodcutterCount);
            ok = false;
        } else {
            printf("[PASS][T16.A] %d Woodcutters active\n", woodcutterCount);
        }

        if (sawmillCount < 1) {
            printf("[FAIL][T16.B] Expected 1 active Sawmill, got %d\n", sawmillCount);
            ok = false;
        } else {
            printf("[PASS][T16.B] %d Sawmills active\n", sawmillCount);
        }

        // Check 2: EconomySystem tracked production matches direct read
        int ecoWood = eco->GetTotalProduced(ResourceType_Wood);
        int ecoPlanks = eco->GetTotalProduced(ResourceType_Planks);

        if (ecoWood != woodProducedDirect) {
            printf("[FAIL][T16.C] EconomySystem Wood (%d) != direct totalOutput (%d)\n",
                ecoWood, woodProducedDirect);
            ok = false;
        } else {
            printf("[PASS][T16.C] EconomySystem Wood tracking: %d == direct %d\n",
                ecoWood, woodProducedDirect);
        }

        if (ecoPlanks != planksProducedDirect) {
            printf("[FAIL][T16.D] EconomySystem Planks (%d) != direct totalOutput (%d)\n",
                ecoPlanks, planksProducedDirect);
            ok = false;
        } else {
            printf("[PASS][T16.D] EconomySystem Planks tracking: %d == direct %d\n",
                ecoPlanks, planksProducedDirect);
        }

        // Check 3: Wood was produced (production works)
        if (ecoWood <= 0) {
            printf("[FAIL][T16.E] No Wood produced — production not running\n");
            ok = false;
        } else {
            printf("[PASS][T16.E] Total Wood produced: %d\n", ecoWood);
        }

        // Check 4: Planks produced
        if (ecoPlanks <= 0) {
            printf("[FAIL][T16.F] No Planks produced — Sawmill not running\n");
            ok = false;
        } else {
            printf("[PASS][T16.F] Total Planks produced: %d\n", ecoPlanks);
        }

        // Check 5: Consumption tracking — Sawmill consumes 2 Wood per Plank
        int ecoConsumed = eco->GetTotalConsumed(ResourceType_Wood);
        int expectedConsumed = ecoPlanks * 2;
        if (ecoConsumed != expectedConsumed) {
            printf("[FAIL][T16.G] Wood consumed (%d) != Planks * 2 (%d)\n",
                ecoConsumed, expectedConsumed);
            ok = false;
        } else {
            printf("[PASS][T16.G] Wood consumption tracking: %d == Planks * 2 (%d)\n",
                ecoConsumed, expectedConsumed);
        }

        // Check 6: No spurious tracking (Planks should not be consumed)
        int planksConsumed = eco->GetTotalConsumed(ResourceType_Planks);
        if (planksConsumed != 0) {
            printf("[FAIL][T16.H] Planks reported as consumed (%d) — should be 0\n",
                planksConsumed);
            ok = false;
        } else {
            printf("[PASS][T16.H] Planks consumed: 0 (correct)\n");
        }

        // Check 7: Wood consumed should be less than total wood produced
        // (Woodcutters produce, Sawmill consumes some)
        if (ecoConsumed >= ecoWood) {
            printf("[FAIL][T16.I] Wood consumed (%d) >= wood produced (%d) — impossible\n",
                ecoConsumed, ecoWood);
            ok = false;
        } else {
            printf("[PASS][T16.I] Wood balance: produced %d > consumed %d\n",
                ecoWood, ecoConsumed);
        }

        if (ok) {
            printf("[PASS] T16: Economy v1 — resource flow tracking verified\n");
        }
        return ok;
    }
};

static T16EconomyTest g_t16EconomyTest;

} // namespace World
