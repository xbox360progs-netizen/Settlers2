#include "../../Testing/ISimulationScenario.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/TreeSystem.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Systems/EconomySystem.h"
#include <stdio.h>

namespace World {

    static void SeedFisherWorld(WorldModel& world, int fish, int maxFish)
    {
        world.width = 50;
        world.height = 50;
        SeedFish(world, fish, maxFish);

        ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
        pb.type = BuildingType_Fisher;
        pb.position = Vector2i(10, 10);
        pb.owner = 0;
        pb.cycleTimer = 30;
        pb.active = true;
        pb.inputsRequested = false;
        pb.inputResources[0] = ResourceType_None;
        pb.inputRequired[0] = 0;
        pb.inputDelivered[0] = 0;
        pb.outputResources[0] = ResourceType_Fish;
        pb.outputBuffer[0] = 0;
        pb.totalOutput[0] = 0;
    }

    // === T44: Fisher + Fish integration test ===
    class T44FisherTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T44"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableTreeDepletion = true;
        }

        void Initialize(Simulation&) {}

        bool Tick(Simulation&)
        {
            printf("\n=== T44: Fisher + Fish ===\n\n");

            bool allOk = true;

            // ---- Test 1: Fisher produces Fish when available ----
            {
                printf("--- Test 1: Fisher produces Fish with fish population ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableTreeDepletion = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedFisherWorld(world, 50, 100);
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int fishProduced = eco ? eco->GetTotalProduced(ResourceType_Fish) : 0;
                int fishLeft = sim.GetWorld().fishCount;

                printf("  Ticks=%u, Fish produced=%d, Fish left=%d\n", kTicks, fishProduced, fishLeft);

                if (fishProduced <= 0) {
                    printf("  [FAIL] Fisher produced no Fish\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Fisher produced Fish (%d)\n", fishProduced);
                }

                if (fishLeft >= 50) {
                    printf("  [PASS] Fish population regenerated (fish=%d >= 50)\n", fishLeft);
                } else {
                    printf("  [FAIL] Fish population depleted (fish=%d < 50)\n", fishLeft);
                    allOk = false;
                }
            }

            // ---- Test 2: Fisher produces nothing when no fish ----
            {
                printf("\n--- Test 2: Fisher produces nothing without fish ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableTreeDepletion = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedFisherWorld(world, 0, 100);
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int fishProduced = eco ? eco->GetTotalProduced(ResourceType_Fish) : 0;

                printf("  Ticks=%u, Fish produced=%d\n", kTicks, fishProduced);

                if (fishProduced > 0) {
                    printf("  [FAIL] Fisher produced Fish without fish population\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Fisher produced 0 Fish without fish population\n");
                }
            }

            // ---- Test 3: Fish population regrows over time ----
            {
                printf("\n--- Test 3: Fish population regrows ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableTreeDepletion = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedFisherWorld(world, 10, 50);
                sim.LoadWorld(world);

                const uint32_t kTicks = 5000;
                int maxFish = 0;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                    int cur = sim.GetWorld().fishCount;
                    if (cur > maxFish) maxFish = cur;
                }

                int finalFish = sim.GetWorld().fishCount;
                printf("  Ticks=%u, Initial=10, Max seen=%d, Final=%d, Capacity=50\n",
                    kTicks, maxFish, finalFish);

                if (finalFish > 10 && finalFish <= 50) {
                    printf("  [PASS] Population regrows toward capacity (%d)\n", finalFish);
                } else if (finalFish <= 10) {
                    printf("  [FAIL] Population did not regrow (%d)\n", finalFish);
                    allOk = false;
                } else if (finalFish > 50) {
                    printf("  [FAIL] Population exceeded capacity (%d > 50)\n", finalFish);
                    allOk = false;
                }
            }

            if (allOk) {
                printf("\n[PASS] T44: Fisher + Fish integration\n");
            } else {
                printf("\n[FAIL] T44: Some checks failed\n");
            }

            return false;
        }
    };

    T44FisherTest g_t44;

} // namespace World
