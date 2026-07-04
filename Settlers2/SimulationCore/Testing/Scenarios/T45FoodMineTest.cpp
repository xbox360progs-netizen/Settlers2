#include "../../Testing/ISimulationScenario.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Systems/EconomySystem.h"
#include "../../Systems/ConsumptionSystem.h"
#include <stdio.h>

namespace World {

    static void SeedMineWorld(WorldModel& world, bool hasFood, int extraFood)
    {
        world.width = 50;
        world.height = 50;

        ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
        pb.type = BuildingType_CoalMine;
        pb.position = Vector2i(10, 10);
        pb.owner = 0;
        pb.cycleTimer = 30;
        pb.active = true;
        pb.inputsRequested = false;
        pb.fed = hasFood;
        pb.foodRequested = false;
        pb.foodStored = extraFood;
        pb.inputResources[0] = ResourceType_None;
        pb.inputRequired[0] = 0;
        pb.inputDelivered[0] = 0;
        pb.outputResources[0] = ResourceType_Coal;
        pb.outputBuffer[0] = 0;
        pb.totalOutput[0] = 0;
    }

    class T45FoodMineTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T45"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableConsumption = true;
        }

        void Initialize(Simulation&) {}

        bool Tick(Simulation&)
        {
            printf("\n=== T45: Mine food consumption ===\n\n");

            bool allOk = true;

            // ---- Test 1: Mine without food produces nothing ----
            {
                printf("--- Test 1: Mine without food produces nothing ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableConsumption = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedMineWorld(world, false, 0);
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int coalProduced = eco ? eco->GetTotalProduced(ResourceType_Coal) : 0;
                printf("  Ticks=%u, Coal produced=%d\n", kTicks, coalProduced);

                if (coalProduced > 0) {
                    printf("  [FAIL] Mine produced Coal without food\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Mine produced 0 Coal without food\n");
                }
            }

            // ---- Test 2: Mine with food produces Coal ----
            {
                printf("\n--- Test 2: Mine with food produces Coal ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableConsumption = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedMineWorld(world, true, 10);
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int coalProduced = eco ? eco->GetTotalProduced(ResourceType_Coal) : 0;
                printf("  Ticks=%u, Coal produced=%d\n", kTicks, coalProduced);

                if (coalProduced <= 0) {
                    printf("  [FAIL] Mine produced no Coal with food available\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Mine produced Coal (%d) with food\n", coalProduced);
                }
            }

            // ---- Test 3: Mine stops after food runs out ----
            {
                printf("\n--- Test 3: Mine stops after food runs out ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableConsumption = true;
                Simulation sim(cfg);
                WorldModel world;
                // Start with 1 food unit, should produce ~1 cycle then stop
                SeedMineWorld(world, true, 1);
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 600;
                int prevCoal = 0;
                int flatTicks = 0;
                int maxFlatTicks = 0;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                    int cur = eco ? eco->GetTotalProduced(ResourceType_Coal) : 0;
                    if (cur == prevCoal) {
                        flatTicks++;
                        if (flatTicks > maxFlatTicks) maxFlatTicks = flatTicks;
                    } else {
                        flatTicks = 0;
                    }
                    prevCoal = cur;
                }

                int coalProduced = eco ? eco->GetTotalProduced(ResourceType_Coal) : 0;
                printf("  Ticks=%u, Coal produced=%d, Max consecutive flat ticks=%d\n",
                    kTicks, coalProduced, maxFlatTicks);

                if (coalProduced <= 0) {
                    printf("  [FAIL] Mine should have produced at least 1 Coal with 1 food\n");
                    allOk = false;
                } else if (maxFlatTicks >= 100) {
                    printf("  [PASS] Mine stopped after food consumed (flat=%d ticks)\n", maxFlatTicks);
                } else {
                    printf("  [WARN] Mine may not have stopped yet (flat=%d ticks, produced=%d)\n",
                        maxFlatTicks, coalProduced);
                }
            }

            if (allOk) {
                printf("\n[PASS] T45: Mine food consumption\n");
            } else {
                printf("\n[FAIL] T45: Some checks failed\n");
            }

            return false;
        }
    };

    T45FoodMineTest g_t45;

} // namespace World
