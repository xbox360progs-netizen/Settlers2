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
#include <stdio.h>

namespace World {

class T8ProductionTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T8"; }

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
        if (loadedWorld.pendingConstructionCount < kMaxConstructionRequests) {
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Woodcutter;
            req.position = Vector2i(10, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
            printf("  Tick %u: Add Woodcutter at (10,10)\n", sim.GetState().tickCount);
        }

        if (loadedWorld.pendingConstructionCount < kMaxConstructionRequests) {
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Woodcutter;
            req.position = Vector2i(20, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
            printf("  Tick %u: Add Woodcutter at (20,10)\n", sim.GetState().tickCount);
        }

        if (loadedWorld.pendingConstructionCount < kMaxConstructionRequests) {
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Sawmill;
            req.position = Vector2i(15, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
            printf("  Tick %u: Add Sawmill at (15,10)\n", sim.GetState().tickCount);
        }
        }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;

        if (currentTick % 1000 == 0) {
        printf("  ... tick %u / 25000\n", currentTick);
        }

        if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
        printf("[FAIL] T8 failed at tick %u\n", currentTick);
        return false;
        }

        if (currentTick >= 25000) {
        bool result = Verify(sim.GetWorld());
        return false;
        }
        return true;
    }

    bool Verify(const WorldModel& world) {
        int woodcutterCount = 0;
        int sawmillCount = 0;
        int woodProduced = 0;
        int planksProduced = 0;

        for (int i = 0; i < world.productionBuildingCount; ++i) {
        const ProductionBuilding& pb = world.productionBuildings[i];
        if (!pb.active) continue;

        const BuildingDefinition& def = GetBuildingDefinition(pb.type);
        if (def.production == PT_Woodcutter) {
            woodcutterCount++;
            woodProduced += pb.totalOutput[0];
        } else if (def.production == PT_Sawmill) {
            sawmillCount++;
            planksProduced += pb.totalOutput[0];
        }
        }

        printf("[INFO] Woodcutter active: %d\n", woodcutterCount);
        printf("[INFO] Sawmill active: %d\n", sawmillCount);
        printf("[INFO] Wood produced (total): %d\n", woodProduced);
        printf("[INFO] Planks produced (total): %d\n", planksProduced);

        if (woodcutterCount < 2) {
        printf("[FAIL][T8] Expected 2 active Woodcutters, got %d\n", woodcutterCount);
        return false;
        }
        if (woodProduced == 0) {
        printf("[FAIL][T8] No wood production output (Woodcutters not cycling)\n");
        return false;
        }
        if (sawmillCount < 1) {
        printf("[FAIL][T8] Expected 1 active Sawmill, got %d\n", sawmillCount);
        return false;
        }
        if (planksProduced == 0) {
        printf("[FAIL][T8] No planks produced (Sawmill input-demand cycle failed)\n");
        return false;
        }

        printf("[PASS] T8: Production system working (Woodcutter + Sawmill with input-demand)\n");
        return true;
    }
};

static T8ProductionTest g_t8ProductionTest;

} // namespace World
