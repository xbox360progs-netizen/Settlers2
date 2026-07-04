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

class T14MultiInput : public ISimulationScenario {
public:
    const char* GetName() const { return "T14"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableProduction = true;
        config.enableEconomy = true;
        config.enableConstruction = true;
    }

    void Initialize(Simulation& sim)
    {
        // Economy must be enabled for Toolmaker's input chain

        WorldModel world;
        world.width = 50;
        world.height = 50;
        sim.LoadWorld(world);

        WorldModel& loadedWorld = sim.GetWorld();

        // Woodcutter — supplies Wood (no inputs)
        if (loadedWorld.pendingConstructionCount < kMaxConstructionRequests) {
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Woodcutter;
            req.position = Vector2i(10, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // Stonemason — supplies Stone (no inputs)
        if (loadedWorld.pendingConstructionCount < kMaxConstructionRequests) {
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Stonemason;
            req.position = Vector2i(20, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // Toolmaker — consumes Wood + Stone → produces Tools (multi-input)
        if (loadedWorld.pendingConstructionCount < kMaxConstructionRequests) {
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Toolmaker;
            req.position = Vector2i(30, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kSoakTicks = 2000;
        static const uint32_t kCheckInterval = 500;

        if (currentTick % kCheckInterval == 0 && currentTick > 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T14 failed at tick %u\n", currentTick);
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
        int woodcutters = 0;
        int stonemasons = 0;
        int toolmakers = 0;
        int totalTools = 0;

        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Woodcutter) woodcutters++;
            if (def.production == PT_Stonemason) stonemasons++;
            if (def.production == PT_Toolmaker) {
                toolmakers++;
                totalTools += pb.totalOutput[0];
            }
        }

        printf("  ... tick %u / 2000 | WC=%d SM=%d TM=%d Tools=%d\n",
            tick, woodcutters, stonemasons, toolmakers, totalTools);
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        bool ok = true;

        // Collect building stats
        int woodcutterCount = 0;
        int stonemasonCount = 0;
        int toolmakerCount = 0;
        int totalTools = 0;

        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);

            if (def.production == PT_Woodcutter) woodcutterCount++;
            if (def.production == PT_Stonemason) stonemasonCount++;
            if (def.production == PT_Toolmaker) {
                toolmakerCount++;
                totalTools += pb.totalOutput[0];
            }
        }

        // Check 1: Toolmaker built
        if (toolmakerCount < 1) {
            printf("[FAIL][T14.A] Toolmaker not built (construction failed)\n");
            ok = false;
        } else {
            printf("[PASS][T14.A] Toolmaker built\n");
        }

        // Check 2: Input suppliers built
        if (woodcutterCount < 1) {
            printf("[FAIL][T14.B] Woodcutter not built\n");
            ok = false;
        } else {
            printf("[PASS][T14.B] Woodcutter built (Wood supplier)\n");
        }

        if (stonemasonCount < 1) {
            printf("[FAIL][T14.C] Stonemason not built\n");
            ok = false;
        } else {
            printf("[PASS][T14.C] Stonemason built (Stone supplier)\n");
        }

        // Check 3: Toolmaker produced tools (multi-input cycle verified)
        if (totalTools == 0) {
            printf("[FAIL][T14.D] No tools produced. "
                   "Multi-input cycle (Wood + Stone → Tools) did not complete.\n");
            ok = false;
        } else {
            printf("[PASS][T14.D] Multi-input production: %d tools produced (Wood + Stone → Tools)\n",
                totalTools);
        }

        if (ok) {
            printf("[PASS] T14: Multi-input production — Toolmaker end-to-end\n");
        }
        return ok;
    }
};

static T14MultiInput g_t14MultiInput;

} // namespace World
