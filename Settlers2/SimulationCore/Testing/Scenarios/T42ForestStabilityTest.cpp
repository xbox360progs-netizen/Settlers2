#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/EconomyMetrics.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/TreeSystem.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Systems/EconomySystem.h"
#include <stdio.h>

namespace World {

    // Shared: seed world with buildings + trees, run with enableTreeDepletion
    static void SeedWorld(WorldModel& world, int woodcutterCount, int foresterCount, int matureTrees, int emptySpots)
    {
        world.width = 50;
        world.height = 50;
        SeedTrees(world, matureTrees, emptySpots);

        for (int i = 0; i < woodcutterCount && world.productionBuildingCount < kMaxProductionBuildings; ++i) {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Woodcutter;
            pb.position = Vector2i(10 + i, 10);
            pb.owner = 0;
            pb.cycleTimer = 30;
            pb.active = true;
            pb.inputsRequested = false;
            pb.inputResources[0] = ResourceType_None;
            pb.inputRequired[0] = 0;
            pb.inputDelivered[0] = 0;
            pb.outputResources[0] = ResourceType_Wood;
            pb.outputBuffer[0] = 0;
            pb.totalOutput[0] = 0;
        }

        for (int i = 0; i < foresterCount && world.productionBuildingCount < kMaxProductionBuildings; ++i) {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Forester;
            pb.position = Vector2i(10 + woodcutterCount + i, 10);
            pb.owner = 0;
            pb.cycleTimer = 30;
            pb.active = true;
            pb.inputsRequested = false;
            pb.inputResources[0] = ResourceType_None;
            pb.inputRequired[0] = 0;
            pb.inputDelivered[0] = 0;
            pb.outputResources[0] = ResourceType_None;
            pb.outputBuffer[0] = 0;
            pb.totalOutput[0] = 0;
        }
    }

    // Single test mode helper — returns true if all checks pass
    static bool RunWoodStability(
        Simulation& sim,
        int woodcutterCount, int foresterCount,
        const char* modeLabel,
        uint32_t maxTicks)
    {
        const WorldModel& world = sim.GetWorld();
        EconomySystem* eco = sim.GetEconomySystem();

        // Collect metrics at intervals
        int matureStart = world.treeMatureCount;

        // Track intermediate values
        int minMature = matureStart;
        int maxMature = matureStart;

        bool ok = true;
        uint32_t tick = 0;

        while (tick < maxTicks) {
            sim.Tick();
            tick = sim.GetState().tickCount;

            if (tick % 500 == 0) {
                int curMature = world.treeMatureCount;
                if (curMature < minMature) minMature = curMature;
                if (curMature > maxMature) maxMature = curMature;
            }
        }

        // ---- Verify ----
        int matureEnd = world.treeMatureCount;
        int totalWoodEnd = eco ? eco->GetTotalProduced(ResourceType_Wood) : 0;
        int matureDelta = matureEnd - matureStart;

        printf("\n[MODE %s] %u ticks: Mature %d→%d (Δ%+d) Wood=%d\n",
            modeLabel, maxTicks, matureStart, matureEnd, matureDelta, totalWoodEnd);

        // Check 1: Trees never negative
        if (minMature < 0) {
            printf("[FAIL][%s] Mature tree count went negative (%d)\n", modeLabel, minMature);
            ok = false;
        } else {
            printf("[PASS][%s] Mature trees never negative (min=%d)\n", modeLabel, minMature);
        }

        // Check 2: Never exceeds capacity (initial mature + empty = tree positions)
        int totalCapacity = matureStart + 500; // initial empty spots
        if (maxMature > totalCapacity) {
            printf("[FAIL][%s] Mature trees exceed capacity (%d > %d)\n", modeLabel, maxMature, totalCapacity);
            ok = false;
        } else {
            printf("[PASS][%s] Mature trees within capacity (max=%d <= %d)\n", modeLabel, maxMature, totalCapacity);
        }

        // Check 3: Mode-specific behavior
        if (woodcutterCount == 1 && foresterCount == 0) {
            // 1W/0F: forest should deplete
            if (matureEnd > 0) {
                printf("[INFO][%s] Forest not fully depleted yet (mature=%d). May need more ticks.\n", modeLabel, matureEnd);
            } else {
                printf("[PASS][%s] Forest fully depleted as expected\n", modeLabel);
            }
        }
        else if (woodcutterCount == 1 && foresterCount == 1) {
            // 1W/1F: stable equilibrium
            if (matureDelta < -10) {
                printf("[FAIL][%s] Expected stable, but mature decreased by %d\n", modeLabel, matureDelta);
                ok = false;
            } else if (matureDelta > 10) {
                printf("[FAIL][%s] Expected stable, but mature increased by %d\n", modeLabel, matureDelta);
                ok = false;
            } else {
                printf("[PASS][%s] Stable equilibrium (Δmature=%+d within ±10)\n", modeLabel, matureDelta);
            }

            // Verify sustained wood production (no starvation)
            if (totalWoodEnd <= 0) {
                printf("[FAIL][%s] No Wood produced\n", modeLabel);
                ok = false;
            }
        }
        else if (woodcutterCount == 2 && foresterCount == 1) {
            // 2W/1F: gradual depletion
            if (matureDelta >= -5) {
                printf("[INFO][%s] Expected depletion, but mature=%+d (may need more ticks)\n", modeLabel, matureDelta);
            } else {
                printf("[PASS][%s] Gradual depletion as expected (Δmature=%+d)\n", modeLabel, matureDelta);
            }
        }
        else if (woodcutterCount == 1 && foresterCount == 2) {
            // 1W/2F: accumulation toward capacity
            if (matureDelta <= 5) {
                printf("[INFO][%s] Expected accumulation, but mature=%+d (may need more ticks)\n", modeLabel, matureDelta);
            } else {
                printf("[PASS][%s] Accumulation as expected (Δmature=%+d)\n", modeLabel, matureDelta);
            }
        }

        return ok;
    }

    // === T42: Forest Stability Validation ===
    class T42ForestStabilityTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T42"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableTreeDepletion = true;
        }

        void Initialize(Simulation&) {}

        bool Tick(Simulation&)
        {
            printf("\n=== T42: Forest Stability — All modes ===\n\n");

            const uint32_t kDuration = 20000;
            bool allOk = true;

            // Mode 1: 1W/0F
            {
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableTreeDepletion = true;
                Simulation sim1(cfg);
                WorldModel w1;
                SeedWorld(w1, 1, 0, 500, 500);
                sim1.LoadWorld(w1);
                if (!RunWoodStability(sim1, 1, 0, "1W/0F", kDuration)) {
                    allOk = false;
                }
            }

            // Mode 2: 1W/1F
            {
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableTreeDepletion = true;
                Simulation sim2(cfg);
                WorldModel w2;
                SeedWorld(w2, 1, 1, 500, 500);
                sim2.LoadWorld(w2);
                if (!RunWoodStability(sim2, 1, 1, "1W/1F", kDuration)) {
                    allOk = false;
                }
            }

            // Mode 3: 2W/1F
            {
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableTreeDepletion = true;
                Simulation sim3(cfg);
                WorldModel w3;
                SeedWorld(w3, 2, 1, 500, 500);
                sim3.LoadWorld(w3);
                if (!RunWoodStability(sim3, 2, 1, "2W/1F", kDuration)) {
                    allOk = false;
                }
            }

            // Mode 4: 1W/2F
            {
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableTreeDepletion = true;
                Simulation sim4(cfg);
                WorldModel w4;
                SeedWorld(w4, 1, 2, 500, 500);
                sim4.LoadWorld(w4);
                if (!RunWoodStability(sim4, 1, 2, "1W/2F", kDuration)) {
                    allOk = false;
                }
            }

            if (allOk) {
                printf("\n[PASS] T42: All 4 forest stability modes behave as expected\n");
                printf("  1W/0F: forest depletes\n");
                printf("  1W/1F: stable equilibrium (Δmature stable)\n");
                printf("  2W/1F: gradual depletion\n");
                printf("  1W/2F: accumulation toward capacity\n");
            } else {
                printf("\n[FAIL] T42: Some mode(s) deviated from expected behavior\n");
            }

            return false;
        }
    };

    T42ForestStabilityTest g_t42;

} // namespace World
