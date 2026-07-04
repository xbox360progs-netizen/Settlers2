#include "../../Testing/ISimulationScenario.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Systems/EconomySystem.h"
#include <stdio.h>

namespace World {

    static const int kFarmIdx = 0;
    static const int kMillIdx = 1;
    static const int kBakeryIdx = 2;

    static void SeedAgricultureWorld(WorldModel& world)
    {
        world.width = 50;
        world.height = 50;

        // Farm — produces Grain from nothing
        ProductionBuilding& farm = world.productionBuildings[world.productionBuildingCount++];
        farm.type = BuildingType_Farm;
        farm.position = Vector2i(10, 10);
        farm.owner = 0;
        farm.cycleTimer = 30;
        farm.active = true;
        farm.inputsRequested = false;
        farm.inputResources[0] = ResourceType_None;
        farm.inputRequired[0] = 0;
        farm.inputDelivered[0] = 0;
        farm.outputResources[0] = ResourceType_Wheat;
        farm.outputBuffer[0] = 0;
        farm.totalOutput[0] = 0;

        // Mill — consumes Grain, produces Flour
        ProductionBuilding& mill = world.productionBuildings[world.productionBuildingCount++];
        mill.type = BuildingType_Mill;
        mill.position = Vector2i(12, 10);
        mill.owner = 0;
        mill.cycleTimer = 30;
        mill.active = true;
        mill.inputsRequested = false;
        mill.inputResources[0] = ResourceType_Wheat;
        mill.inputRequired[0] = 1;
        mill.inputDelivered[0] = 0;
        mill.outputResources[0] = ResourceType_Flour;
        mill.outputBuffer[0] = 0;
        mill.totalOutput[0] = 0;

        // Bakery — consumes Flour, produces Bread
        ProductionBuilding& bakery = world.productionBuildings[world.productionBuildingCount++];
        bakery.type = BuildingType_Bakery;
        bakery.position = Vector2i(14, 10);
        bakery.owner = 0;
        bakery.cycleTimer = 30;
        bakery.active = true;
        bakery.inputsRequested = false;
        bakery.inputResources[0] = ResourceType_Flour;
        bakery.inputRequired[0] = 1;
        bakery.inputDelivered[0] = 0;
        bakery.outputResources[0] = ResourceType_Bread;
        bakery.outputBuffer[0] = 0;
        bakery.totalOutput[0] = 0;
    }

    class T46AgricultureTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T46"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
        }

        void Initialize(Simulation&) {}

        bool Tick(Simulation&)
        {
            printf("\n=== T46: Agriculture pipeline (Farm → Mill → Bakery) ===\n\n");

            bool allOk = true;

            // ---- Test 1: Farm produces Grain ----
            {
                printf("--- Test 1: Farm produces Grain ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedAgricultureWorld(world);
                world.productionBuildings[kMillIdx].active = false;
                world.productionBuildings[kBakeryIdx].active = false;
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int grain = eco ? eco->GetTotalProduced(ResourceType_Wheat) : 0;
                printf("  Ticks=%u, Grain produced=%d\n", kTicks, grain);

                if (grain <= 0) {
                    printf("  [FAIL] Farm produced no Grain\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Farm produced Grain (%d)\n", grain);
                }
            }

            // ---- Test 2: Mill converts Grain → Flour ----
            {
                printf("\n--- Test 2: Mill converts Grain → Flour ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedAgricultureWorld(world);
                world.productionBuildings[kFarmIdx].active = false;
                world.productionBuildings[kBakeryIdx].active = false;
                // Seed 5 Grain deliveries to Mill
                world.productionBuildings[kMillIdx].inputDelivered[0] = 5;
                world.productionBuildings[kMillIdx].inputsRequested = true;
                sim.LoadWorld(world);

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                const WorldModel& w = sim.GetWorld();
                int flour = w.productionBuildings[kMillIdx].totalOutput[0];
                int remaining = w.productionBuildings[kMillIdx].inputDelivered[0];
                int grainUsed = 5 - remaining;
                printf("  Ticks=%u, Grain consumed=%d, Flour produced=%d, Inputs remaining=%d\n",
                    kTicks, grainUsed, flour, remaining);

                if (flour <= 0) {
                    printf("  [FAIL] Mill produced no Flour\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Mill produced Flour (%d)\n", flour);
                }
            }

            // ---- Test 3: Bakery converts Flour → Bread ----
            {
                printf("\n--- Test 3: Bakery converts Flour → Bread ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedAgricultureWorld(world);
                world.productionBuildings[kFarmIdx].active = false;
                world.productionBuildings[kMillIdx].active = false;
                // Seed 5 Flour deliveries to Bakery
                world.productionBuildings[kBakeryIdx].inputDelivered[0] = 5;
                world.productionBuildings[kBakeryIdx].inputsRequested = true;
                sim.LoadWorld(world);

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                const WorldModel& w = sim.GetWorld();
                int bread = w.productionBuildings[kBakeryIdx].totalOutput[0];
                int remaining = w.productionBuildings[kBakeryIdx].inputDelivered[0];
                int flourUsed = 5 - remaining;
                printf("  Ticks=%u, Flour consumed=%d, Bread produced=%d, Inputs remaining=%d\n",
                    kTicks, flourUsed, bread, remaining);

                if (bread <= 0) {
                    printf("  [FAIL] Bakery produced no Bread\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Bakery produced Bread (%d)\n", bread);
                }
            }

            // ---- Test 4: Definition Query API — GetProducer chain ----
            {
                printf("\n--- Test 4: Definition Query API ---\n");
                ProductionType farmProd = GetProducer(ResourceType_Wheat);
                ProductionType millProd = GetProducer(ResourceType_Flour);
                ProductionType bakeryProd = GetProducer(ResourceType_Bread);

                bool ok = true;
                if (farmProd != PT_Farm) {
                    printf("  [FAIL] GetProducer(Grain) = %d (expected PT_Farm=%d)\n", farmProd, PT_Farm);
                    ok = false;
                } else {
                    printf("  [PASS] GetProducer(Grain) = PT_Farm\n");
                }
                if (millProd != PT_Mill) {
                    printf("  [FAIL] GetProducer(Flour) = %d (expected PT_Mill=%d)\n", millProd, PT_Mill);
                    ok = false;
                } else {
                    printf("  [PASS] GetProducer(Flour) = PT_Mill\n");
                }
                if (bakeryProd != PT_Bakery) {
                    printf("  [FAIL] GetProducer(Bread) = %d (expected PT_Bakery=%d)\n", bakeryProd, PT_Bakery);
                    ok = false;
                } else {
                    printf("  [PASS] GetProducer(Bread) = PT_Bakery\n");
                }
                if (!ok) allOk = false;
            }

            if (allOk) {
                printf("\n[PASS] T46: Agriculture pipeline complete\n");
            } else {
                printf("\n[FAIL] T46: Some checks failed\n");
            }

            return false;
        }
    };

    T46AgricultureTest g_t46;

} // namespace World
