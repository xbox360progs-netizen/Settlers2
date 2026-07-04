#include "../../Testing/ISimulationScenario.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Systems/EconomySystem.h"
#include <stdio.h>

namespace World {

    static const int kCoalMine = 0;
    static const int kIronMine = 1;
    static const int kGoldMine = 2;

    static void SeedMiningWorld(WorldModel& world)
    {
        world.width = 50;
        world.height = 50;

        // Coal Mine — needs food, produces Coal
        ProductionBuilding& coal = world.productionBuildings[world.productionBuildingCount++];
        coal.type = BuildingType_CoalMine;
        coal.position = Vector2i(10, 10);
        coal.owner = 0;
        coal.cycleTimer = 30;
        coal.active = true;
        coal.inputsRequested = false;
        coal.fed = true;
        coal.foodRequested = false;
        coal.foodStored = 10;
        coal.inputResources[0] = ResourceType_None;
        coal.inputRequired[0] = 0;
        coal.inputDelivered[0] = 0;
        coal.outputResources[0] = ResourceType_Coal;
        coal.outputBuffer[0] = 0;
        coal.totalOutput[0] = 0;

        // Iron Mine — needs food, produces IronOre
        ProductionBuilding& iron = world.productionBuildings[world.productionBuildingCount++];
        iron.type = BuildingType_IronMine;
        iron.position = Vector2i(12, 10);
        iron.owner = 0;
        iron.cycleTimer = 30;
        iron.active = true;
        iron.inputsRequested = false;
        iron.fed = true;
        iron.foodRequested = false;
        iron.foodStored = 10;
        iron.inputResources[0] = ResourceType_None;
        iron.inputRequired[0] = 0;
        iron.inputDelivered[0] = 0;
        iron.outputResources[0] = ResourceType_IronOre;
        iron.outputBuffer[0] = 0;
        iron.totalOutput[0] = 0;

        // Gold Mine — needs food, produces GoldOre
        ProductionBuilding& gold = world.productionBuildings[world.productionBuildingCount++];
        gold.type = BuildingType_GoldMine;
        gold.position = Vector2i(14, 10);
        gold.owner = 0;
        gold.cycleTimer = 30;
        gold.active = true;
        gold.inputsRequested = false;
        gold.fed = true;
        gold.foodRequested = false;
        gold.foodStored = 10;
        gold.inputResources[0] = ResourceType_None;
        gold.inputRequired[0] = 0;
        gold.inputDelivered[0] = 0;
        gold.outputResources[0] = ResourceType_GoldOre;
        gold.outputBuffer[0] = 0;
        gold.totalOutput[0] = 0;
    }

    class T47MiningMetallurgyTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T47"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableConsumption = true;
        }

        void Initialize(Simulation&) {}

        bool Tick(Simulation&)
        {
            printf("\n=== T47: Mining & Metallurgy (CoalMine, IronMine, GoldMine) ===\n\n");

            bool allOk = true;

            // ---- Test 1: Coal Mine produces Coal with food ----
            {
                printf("--- Test 1: Coal Mine with food ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableConsumption = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedMiningWorld(world);
                world.productionBuildings[kIronMine].active = false;
                world.productionBuildings[kGoldMine].active = false;
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int coal = eco ? eco->GetTotalProduced(ResourceType_Coal) : 0;
                printf("  Ticks=%u, Coal produced=%d\n", kTicks, coal);

                if (coal <= 0) {
                    printf("  [FAIL] Coal Mine produced no Coal\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Coal Mine produced Coal (%d)\n", coal);
                }
            }

            // ---- Test 2: Iron Mine produces IronOre with food ----
            {
                printf("\n--- Test 2: Iron Mine with food ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableConsumption = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedMiningWorld(world);
                world.productionBuildings[kCoalMine].active = false;
                world.productionBuildings[kGoldMine].active = false;
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int ore = eco ? eco->GetTotalProduced(ResourceType_IronOre) : 0;
                printf("  Ticks=%u, IronOre produced=%d\n", kTicks, ore);

                if (ore <= 0) {
                    printf("  [FAIL] Iron Mine produced no IronOre\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Iron Mine produced IronOre (%d)\n", ore);
                }
            }

            // ---- Test 3: Gold Mine produces GoldOre with food ----
            {
                printf("\n--- Test 3: Gold Mine with food ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableConsumption = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedMiningWorld(world);
                world.productionBuildings[kCoalMine].active = false;
                world.productionBuildings[kIronMine].active = false;
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int ore = eco ? eco->GetTotalProduced(ResourceType_GoldOre) : 0;
                printf("  Ticks=%u, GoldOre produced=%d\n", kTicks, ore);

                if (ore <= 0) {
                    printf("  [FAIL] Gold Mine produced no GoldOre\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Gold Mine produced GoldOre (%d)\n", ore);
                }
            }

            // ---- Test 4: All three mines produce simultaneously ----
            {
                printf("\n--- Test 4: All three mines together ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                cfg.enableConsumption = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedMiningWorld(world);
                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int coal = eco ? eco->GetTotalProduced(ResourceType_Coal) : 0;
                int ironOre = eco ? eco->GetTotalProduced(ResourceType_IronOre) : 0;
                int goldOre = eco ? eco->GetTotalProduced(ResourceType_GoldOre) : 0;
                printf("  Ticks=%u, Coal=%d, IronOre=%d, GoldOre=%d\n", kTicks, coal, ironOre, goldOre);

                if (coal <= 0) {
                    printf("  [FAIL] Coal Mine produced nothing\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Coal Mine produced %d\n", coal);
                }
                if (ironOre <= 0) {
                    printf("  [FAIL] Iron Mine produced nothing\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Iron Mine produced %d\n", ironOre);
                }
                if (goldOre <= 0) {
                    printf("  [FAIL] Gold Mine produced nothing\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Gold Mine produced %d\n", goldOre);
                }
            }

            // ---- Test 5: Definition Query API — GetProducer chain ----
            {
                printf("\n--- Test 5: Definition Query API ---\n");
                ProductionType coalProd = GetProducer(ResourceType_Coal);
                ProductionType ironProd = GetProducer(ResourceType_IronOre);
                ProductionType goldProd = GetProducer(ResourceType_GoldOre);

                bool ok = true;
                if (coalProd != PT_CoalMine) {
                    printf("  [FAIL] GetProducer(Coal) = %d (expected PT_CoalMine=%d)\n", coalProd, PT_CoalMine);
                    ok = false;
                } else {
                    printf("  [PASS] GetProducer(Coal) = PT_CoalMine\n");
                }
                if (ironProd != PT_IronMine) {
                    printf("  [FAIL] GetProducer(IronOre) = %d (expected PT_IronMine=%d)\n", ironProd, PT_IronMine);
                    ok = false;
                } else {
                    printf("  [PASS] GetProducer(IronOre) = PT_IronMine\n");
                }
                if (goldProd != PT_GoldMine) {
                    printf("  [FAIL] GetProducer(GoldOre) = %d (expected PT_GoldMine=%d)\n", goldProd, PT_GoldMine);
                    ok = false;
                } else {
                    printf("  [PASS] GetProducer(GoldOre) = PT_GoldMine\n");
                }
                if (!ok) allOk = false;
            }

            if (allOk) {
                printf("\n[PASS] T47: Mining & Metallurgy complete\n");
            } else {
                printf("\n[FAIL] T47: Some checks failed\n");
            }

            return false;
        }
    };

    T47MiningMetallurgyTest g_t47;

} // namespace World
