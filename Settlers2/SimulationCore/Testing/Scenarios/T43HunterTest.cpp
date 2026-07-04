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

    // Seed world with Hunter building and initial animals
    static void SeedHunterWorld(WorldModel& world, int animals, int maxAnimals)
    {
        world.width = 50;
        world.height = 50;
        SeedAnimals(world, animals, maxAnimals);

        ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
        pb.type = BuildingType_Hunter;
        pb.position = Vector2i(10, 10);
        pb.owner = 0;
        pb.cycleTimer = 30;
        pb.active = true;
        pb.inputsRequested = false;
        pb.inputResources[0] = ResourceType_None;
        pb.inputRequired[0] = 0;
        pb.inputDelivered[0] = 0;
        pb.outputResources[0] = ResourceType_Meat;
        pb.outputBuffer[0] = 0;
        pb.totalOutput[0] = 0;
    }

    // === T43: Hunter + Animals integration test ===
    class T43HunterTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T43"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableTreeDepletion = true;
        }

        void Initialize(Simulation&) {}

        bool Tick(Simulation&)
        {
            printf("\n=== T43: Hunter + Animals ===\n\n");

            bool allOk = true;

            // ---- Test 1: Hunter produces Meat when animals available ----
            {
                printf("--- Test 1: Hunter produces Meat with animals ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableTreeDepletion = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedHunterWorld(world, 50, 100);
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int meatProduced = eco ? eco->GetTotalProduced(ResourceType_Meat) : 0;
                int animalsLeft = sim.GetWorld().animalCount;

                printf("  Ticks=%u, Meat produced=%d, Animals left=%d\n", kTicks, meatProduced, animalsLeft);

                if (meatProduced <= 0) {
                    printf("  [FAIL] Hunter produced no Meat\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Hunter produced Meat (%d)\n", meatProduced);
                }

                if (animalsLeft >= 50) {
                    printf("  [PASS] Animal population regenerated (animals=%d >= 50)\n", animalsLeft);
                } else {
                    printf("  [FAIL] Animal population depleted (animals=%d < 50)\n", animalsLeft);
                    allOk = false;
                }
            }

            // ---- Test 2: Hunter produces nothing when no animals ----
            {
                printf("\n--- Test 2: Hunter produces nothing without animals ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableTreeDepletion = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedHunterWorld(world, 0, 100);
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int meatProduced = eco ? eco->GetTotalProduced(ResourceType_Meat) : 0;

                printf("  Ticks=%u, Meat produced=%d\n", kTicks, meatProduced);

                if (meatProduced > 0) {
                    printf("  [FAIL] Hunter produced Meat without animals\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Hunter produced 0 Meat without animals\n");
                }
            }

            // ---- Test 3: Animal population regrows over time ----
            {
                printf("\n--- Test 3: Animal population regrows ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableTreeDepletion = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedHunterWorld(world, 10, 50);
                sim.LoadWorld(world);

                const uint32_t kTicks = 5000;
                int maxAnimals = 0;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                    int cur = sim.GetWorld().animalCount;
                    if (cur > maxAnimals) maxAnimals = cur;
                }

                int finalAnimals = sim.GetWorld().animalCount;
                printf("  Ticks=%u, Initial=10, Max seen=%d, Final=%d, Capacity=50\n",
                    kTicks, maxAnimals, finalAnimals);

                if (finalAnimals > 10 && finalAnimals <= 50) {
                    printf("  [PASS] Population regrows toward capacity (%d)\n", finalAnimals);
                } else if (finalAnimals <= 10) {
                    printf("  [FAIL] Population did not regrow (%d)\n", finalAnimals);
                    allOk = false;
                } else if (finalAnimals > 50) {
                    printf("  [FAIL] Population exceeded capacity (%d > 50)\n", finalAnimals);
                    allOk = false;
                }
            }

            if (allOk) {
                printf("\n[PASS] T43: Hunter + Animals integration\n");
            } else {
                printf("\n[FAIL] T43: Some checks failed\n");
            }

            return false;
        }
    };

    T43HunterTest g_t43;

} // namespace World
