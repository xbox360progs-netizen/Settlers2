#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/ConstructionAssertions.h"
#include "../../Testing/Assertions/TransportAssertions.h"
#include "../../Testing/Assertions/WorldAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Definitions/BuildingDefinition.h"
#include "../../Definitions/ProductionDefinition.h"
#include "../../Warehouse/WarehouseSystem.h"
#include "../../Construction/ConstructionSite.h"
#include "../../Construction/ConstructionState.h"
#include "../../Systems/DemandManager.h"
#include <stdio.h>

namespace World {

class T17WarehouseSoak : public ISimulationScenario {
public:
    const char* GetName() const { return "T17"; }

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

        // 3 Woodcutters (produce Wood)
        for (int i = 0; i < 3; ++i) {
            if (loadedWorld.pendingConstructionCount >= kMaxConstructionRequests) break;
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Woodcutter;
            req.position = Vector2i(10 + i * 8, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // 2 Sawmills (consume Wood, produce Planks)
        for (int i = 0; i < 2; ++i) {
            if (loadedWorld.pendingConstructionCount >= kMaxConstructionRequests) break;
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Sawmill;
            req.position = Vector2i(10 + i * 8, 20);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // Track warehouse stockpile history for monotonic check
        for (int r = 0; r < kMaxTrackedResources; ++r) {
            m_previousStockpile[r] = -1;
        }
        m_setupTick = 0;
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kSoakTicks = 50000;
        static const uint32_t kCheckInterval = 2500;

        // Wait until tick 100 before tracking (buildings need to be constructed)
        if (currentTick >= 100 && m_setupTick == 0) {
            m_setupTick = currentTick;
        }

        if (currentTick % kCheckInterval == 0 && currentTick > 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T17 failed at tick %u\n", currentTick);
                return false;
            }

            if (!CheckMonotonicStockpile(sim, currentTick)) {
                return false;
            }

            // Status report
            const WorldModel& world = sim.GetWorld();
            WarehouseSystem* ws = sim.GetWarehouseSystem();
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
                }
                if (def.production == PT_Sawmill) {
                    activeSawmills++;
                    totalPlanks += pb.totalOutput[0];
                }
            }

            int woodStock = ws ? ws->GetStockpileAmount(ResourceType_Wood) : -1;
            int planksStock = ws ? ws->GetStockpileAmount(ResourceType_Planks) : -1;

            printf("  ... tick %u / %u | WC=%d SM=%d | Wood prod=%d stock=%d | Planks prod=%d stock=%d\n",
                currentTick, kSoakTicks,
                activeWoodcutters, activeSawmills,
                totalWood, woodStock,
                totalPlanks, planksStock);
        }

        if (currentTick >= kSoakTicks) {
            return Verify(sim);
        }
        return true;
    }

    bool CheckMonotonicStockpile(Simulation& sim, uint32_t tick)
    {
        if (m_setupTick == 0) return true;

        WarehouseSystem* ws = sim.GetWarehouseSystem();
        if (ws == NULL) return true;

        int woodStock = ws->GetStockpileAmount(ResourceType_Wood);
        int planksStock = ws->GetStockpileAmount(ResourceType_Planks);

        if (m_previousStockpile[0] >= 0 && woodStock < m_previousStockpile[0]) {
            printf("[FAIL][T17.M] Wood stockpile decreased at tick %u: %d -> %d\n",
                tick, m_previousStockpile[0], woodStock);
            return false;
        }
        if (m_previousStockpile[1] >= 0 && planksStock < m_previousStockpile[1]) {
            printf("[FAIL][T17.M] Planks stockpile decreased at tick %u: %d -> %d\n",
                tick, m_previousStockpile[1], planksStock);
            return false;
        }

        m_previousStockpile[0] = woodStock;
        m_previousStockpile[1] = planksStock;
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        WarehouseSystem* ws = sim.GetWarehouseSystem();
        bool ok = true;

        if (ws == NULL) {
            printf("[FAIL][T17] WarehouseSystem not available\n");
            return false;
        }

        // Check 1: Production output — minimum throughput
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
            }
            if (def.production == PT_Sawmill) {
                activeSawmills++;
                totalPlanks += pb.totalOutput[0];
            }
        }

        // 50k ticks / 30 per cycle ≈ 1666 cycles max per building
        // 3 Woodcutters → ~5000 Wood theoretical max
        if (totalWood < 1500) {
            printf("[FAIL][T17.A] Wood throughput too low: %d (expected ≥1500 for 50k ticks)\n", totalWood);
            ok = false;
        } else {
            printf("[PASS][T17.A] Wood throughput: %d (≥1500)\n", totalWood);
        }

        // 2 Sawmills → ~3332 Planks theoretical max, expect ≥1000
        if (totalPlanks < 1000) {
            printf("[FAIL][T17.B] Planks throughput too low: %d (expected ≥1000 for 50k ticks)\n", totalPlanks);
            ok = false;
        } else {
            printf("[PASS][T17.B] Planks throughput: %d (≥1000)\n", totalPlanks);
        }

        // Check 2: Warehouse received goods
        int woodStock = ws->GetStockpileAmount(ResourceType_Wood);
        int planksStock = ws->GetStockpileAmount(ResourceType_Planks);

        if (woodStock == 0) {
            printf("[FAIL][T17.C] Warehouse received no Wood — production→warehouse pipeline broken\n");
            ok = false;
        } else {
            printf("[PASS][T17.C] Warehouse Wood stockpile: %d\n", woodStock);
        }

        if (planksStock == 0) {
            printf("[FAIL][T17.D] Warehouse received no Planks — Sawmill→warehouse pipeline broken\n");
            ok = false;
        } else {
            printf("[PASS][T17.D] Warehouse Planks stockpile: %d\n", planksStock);
        }

        // Check 3: Resource balance — produced = warehouse + outputBuffer (no resource leak)
        int woodInBuffers = 0;
        int planksInBuffers = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            const ProductionBuilding& pb = world.productionBuildings[i];
            if (!pb.active) continue;
            const BuildingDefinition& def = GetBuildingDefinition(pb.type);
            if (def.production == PT_Woodcutter) {
                woodInBuffers += pb.outputBuffer[0];
            }
            if (def.production == PT_Sawmill) {
                planksInBuffers += pb.outputBuffer[0];
            }
        }

        int woodDelivered = totalWood - woodInBuffers;
        int planksDelivered = totalPlanks - planksInBuffers;

        // Warehouse stockpile should match delivered amount (within tolerance for in-flight)
        int woodDiff = woodStock - woodDelivered;
        int planksDiff = planksStock - planksDelivered;

        if (woodDiff < 0 || woodDiff > 4) {
            printf("[FAIL][T17.E] Wood stockpile inconsistency: stock=%d delivered=%d diff=%d\n",
                woodStock, woodDelivered, woodDiff);
            ok = false;
        } else {
            printf("[PASS][T17.E] Wood balance: produced=%d buffer=%d warehouse=%d (diff=%d)\n",
                totalWood, woodInBuffers, woodStock, woodDiff);
        }

        if (planksDiff < 0 || planksDiff > 4) {
            printf("[FAIL][T17.F] Planks stockpile inconsistency: stock=%d delivered=%d diff=%d\n",
                planksStock, planksDelivered, planksDiff);
            ok = false;
        } else {
            printf("[PASS][T17.F] Planks balance: produced=%d buffer=%d warehouse=%d (diff=%d)\n",
                totalPlanks, planksInBuffers, planksStock, planksDiff);
        }

        // Check 4: Monotonic stockpile verified throughout soak
        if (m_previousStockpile[0] != woodStock || m_previousStockpile[1] != planksStock) {
            printf("[INFO][T17.G] Final stockpile: Wood=%d Planks=%d\n", woodStock, planksStock);
        }

        // Check 5: Demand tracking — no stuck demands
        DemandManager* dm = sim.GetDemandManager();
        if (dm != NULL) {
            int demandCount = dm->GetDemandCount();
            int stuckDemands = 0;
            for (int d = 0; d < demandCount; ++d) {
                if (dm->GetDemandRemaining(d) > 0) {
                    stuckDemands++;
                }
            }
            if (stuckDemands > 10) {
                printf("[FAIL][T17.H] Too many stuck demands: %d (max 10)\n", stuckDemands);
                ok = false;
            } else {
                printf("[PASS][T17.H] Demand queue: %d active demands (≤10 acceptable)\n", stuckDemands);
            }
        }

        // Check 6: Transport handled warehouse demands
        int warehouseTransportReqs = 0;
        for (int i = 0; i < world.pendingRequestCount; ++i) {
            if (world.pendingRequests[i].reason == TTR_WarehouseBalance) {
                warehouseTransportReqs++;
            }
        }
        if (warehouseTransportReqs == 0) {
            printf("[FAIL][T17.I] No TTR_WarehouseBalance transport requests across 50k ticks\n");
            ok = false;
        } else {
            printf("[PASS][T17.I] TTR_WarehouseBalance requests: %d\n", warehouseTransportReqs);
        }

        if (ok) {
            printf("[PASS] T17: Warehouse Soak — 50k ticks, stockpile monotonic, no resource leak\n");
        }
        return ok;
    }

private:
    static const int kMaxTrackedResources = 4;
    int m_previousStockpile[kMaxTrackedResources];
    uint32_t m_setupTick;
};

static T17WarehouseSoak g_t17WarehouseSoak;

} // namespace World
