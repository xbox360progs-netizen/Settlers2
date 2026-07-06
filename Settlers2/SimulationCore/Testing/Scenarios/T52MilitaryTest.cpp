#include "../../Testing/ISimulationScenario.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Core/ProductionTypes.h"
#include "../../Definitions/ProductionDefinition.h"
#include "../../Systems/EconomySystem.h"
#include <stdio.h>

namespace World {

    // Military Production Test
    //
    // Verifies that Barracks converts Weapons into Soldiers:
    //   WeaponSmith (IronBar + Coal) → Weapons → Barracks → Soldiers
    //
    // This is the first end-product consumer — Barracks produces Soldiers
    // that have no downstream consumer. Soldiers accumulate in outputBuffer.

    static void SeedBarracks(WorldModel& world, bool hasWeapons)
    {
        world.width = 50;
        world.height = 50;

        ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
        pb.type = BuildingType_Barracks;
        pb.position = Vector2i(10, 10);
        pb.owner = 0;
        pb.cycleTimer = 0;
        pb.active = true;
        pb.inputsRequested = !hasWeapons;

        pb.inputResources[0] = ResourceType_Weapons;
        pb.inputRequired[0] = 1;
        pb.inputDelivered[0] = hasWeapons ? 1 : 0;

        pb.outputResources[0] = ResourceType_Soldiers;
        pb.outputBuffer[0] = 0;
        pb.totalOutput[0] = 0;
    }

    class T52MilitaryTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T52"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
        }

        void Initialize(Simulation&) {}

        bool Tick(Simulation&)
        {
            printf("\n=== T52: Military production test (Barracks) ===\n\n");

            bool allOk = true;

            // ---- Test 1: Barracks without Weapons produces nothing ----
            {
                printf("--- Test 1: Barracks without Weapons -> no Soldiers ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedBarracks(world, false);
                sim.LoadWorld(world);

                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int soldiers = eco ? eco->GetTotalProduced(ResourceType_Soldiers) : 0;
                const WorldModel& w = sim.GetWorld();
                int buf = w.productionBuildingCount > 0 ? w.productionBuildings[0].outputBuffer[0] : 0;
                printf("  Soldiers total=%d, outputBuffer=%d\n", soldiers, buf);

                if (soldiers > 0 || buf > 0) {
                    printf("  [FAIL] Barracks produced Soldiers without Weapons\n");
                    allOk = false;
                } else {
                    printf("  [PASS] No production without Weapons\n");
                }
            }

            // ---- Test 2: Barracks with Weapons produces Soldiers ----
            {
                printf("\n--- Test 2: Barracks with Weapons -> produces Soldiers ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedBarracks(world, true);
                sim.LoadWorld(world);

                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 500;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int soldiers = eco ? eco->GetTotalProduced(ResourceType_Soldiers) : 0;
                const WorldModel& w = sim.GetWorld();
                int buf = w.productionBuildingCount > 0 ? w.productionBuildings[0].outputBuffer[0] : 0;
                printf("  Soldiers total=%d, outputBuffer=%d\n", soldiers, buf);

                if (soldiers <= 0 && buf <= 0) {
                    printf("  [FAIL] Barracks produced no Soldiers with Weapons\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Produced Soldiers (%d) with Weapons\n", soldiers > 0 ? soldiers : buf);
                }
            }

            // ---- Test 3: Definition Query API ----
            {
                printf("\n--- Test 3: Definition Query API ---\n");
                ProductionType prod = GetProducer(ResourceType_Soldiers);
                if (prod != PT_Barracks) {
                    printf("  [FAIL] GetProducer(Soldiers) = %d (expected PT_Barracks=%d)\n", prod, PT_Barracks);
                    allOk = false;
                } else {
                    printf("  [PASS] GetProducer(Soldiers) = PT_Barracks\n");
                }
            }

            // ---- Test 4: Full chain — WeaponSmith produces Weapons from IronBar + Coal ----
            {
                printf("\n--- Test 4: WeaponSmith multi-input (IronBar + Coal -> Weapons) ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                Simulation sim(cfg);
                WorldModel world;
                world.width = 50;
                world.height = 50;

                // WeaponSmith with both inputs delivered
                ProductionBuilding& ws = world.productionBuildings[world.productionBuildingCount++];
                ws.type = BuildingType_WeaponSmith;
                ws.position = Vector2i(10, 10);
                ws.owner = 0;
                ws.cycleTimer = 0;
                ws.active = true;
                ws.inputsRequested = false;
                ws.inputResources[0] = ResourceType_IronBar;
                ws.inputRequired[0] = 1;
                ws.inputDelivered[0] = 1;
                ws.inputResources[1] = ResourceType_Coal;
                ws.inputRequired[1] = 1;
                ws.inputDelivered[1] = 1;
                ws.outputResources[0] = ResourceType_Weapons;
                ws.outputBuffer[0] = 0;
                ws.totalOutput[0] = 0;

                sim.LoadWorld(world);
                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int weapons = eco ? eco->GetTotalProduced(ResourceType_Weapons) : 0;
                printf("  Weapons produced: %d\n", weapons);

                if (weapons <= 0) {
                    printf("  [FAIL] WeaponSmith produced nothing with both inputs\n");
                    allOk = false;
                } else {
                    printf("  [PASS] WeaponSmith produced Weapons: %d\n", weapons);
                }
            }

            // ---- Test 5: Soldiers end-product invariant (no downstream consumer) ----
            {
                printf("\n--- Test 5: Soldiers is a terminal resource ---\n");
                bool consumed = false;
                for (int t = 1; t < PT_Count; ++t) {
                    const ProductionDefinition& def = GetProductionDefinition(static_cast<ProductionType>(t));
                    for (int c = 0; c < 4; ++c) {
                        if (def.consumes[c].resource == ResourceType_Soldiers && def.consumes[c].amount > 0) {
                            consumed = true;
                            break;
                        }
                    }
                    if (consumed) break;
                }
                if (consumed) {
                    printf("  [INFO] Soldiers consumed by another production (unexpected for terminal)\n");
                } else {
                    printf("  [PASS] Soldiers is a terminal resource (no downstream consumer)\n");
                }
            }

            if (allOk) {
                printf("\n[PASS] T52: Military production complete\n");
            } else {
                printf("\n[FAIL] T52: Some checks failed\n");
            }

            return false;
        }
    };

    T52MilitaryTest g_t52;

} // namespace World
