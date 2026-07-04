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
#include "../../Warehouse/WarehouseSystem.h"
#include <stdio.h>

namespace World {

class T15WarehouseTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T15"; }

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

        // 2 Woodcutters (supply Wood)
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
        static const uint32_t kSoakTicks = 1000;
        static const uint32_t kCheckInterval = 250;

        if (currentTick % kCheckInterval == 0 && currentTick > 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T15 failed at tick %u\n", currentTick);
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

        // Check 1: Buildings constructed
        int woodcutterCount = 0;
        int sawmillCount = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Woodcutter) woodcutterCount++;
            if (def.production == PT_Sawmill) sawmillCount++;
        }

        if (woodcutterCount < 2) {
            printf("[FAIL][T15.A] Expected 2 active Woodcutters, got %d\n", woodcutterCount);
            ok = false;
        } else {
            printf("[PASS][T15.A] %d Woodcutters active\n", woodcutterCount);
        }

        if (sawmillCount < 1) {
            printf("[FAIL][T15.B] Expected 1 active Sawmill, got %d\n", sawmillCount);
            ok = false;
        } else {
            printf("[PASS][T15.B] %d Sawmills active\n", sawmillCount);
        }

        // Check 2: Warehouse received goods from production
        WarehouseSystem* ws = GetWarehouseSystem(sim);
        if (ws == NULL) {
            printf("[FAIL][T15] WarehouseSystem not available\n");
            return false;
        }

        int woodInWarehouse = ws->GetStockpileAmount(ResourceType_Wood);
        int planksInWarehouse = ws->GetStockpileAmount(ResourceType_Planks);

        if (woodInWarehouse == 0) {
            printf("[FAIL][T15.C] No Wood in warehouse — production output not flowing\n");
            ok = false;
        } else {
            printf("[PASS][T15.C] Warehouse received %d Wood from production\n", woodInWarehouse);
        }

        if (planksInWarehouse == 0) {
            printf("[FAIL][T15.D] No Planks in warehouse — Sawmill→warehouse pipeline broken\n");
            ok = false;
        } else {
            printf("[PASS][T15.D] Warehouse received %d Planks from Sawmill\n", planksInWarehouse);
        }

        // Check 3: Production buildings have outputBuffer drained by warehouse demand
        // At least some buildings should have outputBuffer == 0 (warehouse consumed)
        int drainedBuildings = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            bool allDrained = true;
            for (int p = 0; p < kMaxProductionInputs; ++p) {
                if (pb.outputBuffer[p] > 0) {
                    allDrained = false;
                    break;
                }
            }
            if (allDrained) drainedBuildings++;
        }
        printf("[INFO][T15.E] Buildings with drained outputBuffer: %d / %d\n",
            drainedBuildings, world.productionBuildingCount);

        // Check 4: Transport handled warehouse demands
        int warehouseBalanceRequests = 0;
        for (int i = 0; i < world.pendingRequestCount; ++i) {
            if (world.pendingRequests[i].reason == TTR_WarehouseBalance) {
                warehouseBalanceRequests++;
            }
        }
        if (warehouseBalanceRequests == 0) {
            printf("[FAIL][T15.F] No TTR_WarehouseBalance transport requests — warehouse not creating demand\n");
            ok = false;
        } else {
            printf("[PASS][T15.F] Warehouse created %d transport requests (TTR_WarehouseBalance)\n",
                warehouseBalanceRequests);
        }

        if (ok) {
            printf("[PASS] T15: Warehouse system — production→warehouse pipeline verified\n");
        }
        return ok;
    }

    WarehouseSystem* GetWarehouseSystem(Simulation& sim) {
        return sim.GetWarehouseSystem();
    }
};

static T15WarehouseTest g_t15WarehouseTest;

} // namespace World
