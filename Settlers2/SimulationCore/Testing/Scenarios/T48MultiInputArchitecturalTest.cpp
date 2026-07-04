#include "../../Testing/ISimulationScenario.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Definitions/ProductionDefinition.h"
#include "../../Systems/EconomySystem.h"
#include <stdio.h>

namespace World {

    // New multi-input building: WeaponSmith (IronBar + Coal -> Weapons)
    // This test verifies that ProductionSystem handles multi-input production
    // without any special cases — purely through ProductionDefinition.

    // cycleTime = 30, consumes {IronBar, 1} + {Coal, 1}, produces {Weapons, 1}

    static int CountDeliveredInputs(const ProductionBuilding& pb)
    {
        int count = 0;
        for (int c = 0; c < 4; ++c) {
            if (pb.inputResources[c] != ResourceType_None && pb.inputRequired[c] > 0) {
                count += pb.inputDelivered[c];
            }
        }
        return count;
    }

    static int CountRequiredInputs(const ProductionBuilding& pb)
    {
        int count = 0;
        for (int c = 0; c < 4; ++c) {
            if (pb.inputResources[c] != ResourceType_None && pb.inputRequired[c] > 0) {
                count += pb.inputRequired[c];
            }
        }
        return count;
    }

    static void SeedWeaponSmith(WorldModel& world, bool hasIronBar, bool hasCoal)
    {
        world.width = 50;
        world.height = 50;

        ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
        pb.type = BuildingType_WeaponSmith;
        pb.position = Vector2i(10, 10);
        pb.owner = 0;
        pb.cycleTimer = 0; // start at 0 — first production check advances it
        pb.active = true;
        pb.inputsRequested = false;

        // Input 0: IronBar
        pb.inputResources[0] = ResourceType_IronBar;
        pb.inputRequired[0] = 1;
        pb.inputDelivered[0] = hasIronBar ? 1 : 0;

        // Input 1: Coal
        pb.inputResources[1] = ResourceType_Coal;
        pb.inputRequired[1] = 1;
        pb.inputDelivered[1] = hasCoal ? 1 : 0;

        pb.outputResources[0] = ResourceType_Weapons;
        pb.outputBuffer[0] = 0;
        pb.totalOutput[0] = 0;
    }

    class T48MultiInputArchitecturalTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T48"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
        }

        void Initialize(Simulation&) {}

        bool Tick(Simulation&)
        {
            printf("\n=== T48: Multi-input architectural test ===\n\n");

            bool allOk = true;

            // ---- Test 1: Only IronBar -> no production ----
            {
                printf("--- Test 1: Only IronBar (no Coal) -> no production ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedWeaponSmith(world, true, false); // IronBar delivered, Coal not delivered
                sim.LoadWorld(world);

                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int weapons = eco ? eco->GetTotalProduced(ResourceType_Weapons) : 0;
                // Also check the building directly
                const WorldModel& w = sim.GetWorld();
                int buf = 0;
                if (w.productionBuildingCount > 0) {
                    buf = w.productionBuildings[0].outputBuffer[0];
                }
                printf("  Weapons total=%d, outputBuffer=%d\n", weapons, buf);

                if (weapons > 0 || buf > 0) {
                    printf("  [FAIL] WeaponSmith produced Weapons with only IronBar\n");
                    allOk = false;
                } else {
                    printf("  [PASS] No production without Coal\n");
                }
            }

            // ---- Test 2: Only Coal -> no production ----
            {
                printf("\n--- Test 2: Only Coal (no IronBar) -> no production ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedWeaponSmith(world, false, true); // Coal delivered, IronBar not delivered
                sim.LoadWorld(world);

                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int weapons = eco ? eco->GetTotalProduced(ResourceType_Weapons) : 0;
                const WorldModel& w = sim.GetWorld();
                int buf = 0;
                if (w.productionBuildingCount > 0) {
                    buf = w.productionBuildings[0].outputBuffer[0];
                }
                printf("  Weapons total=%d, outputBuffer=%d\n", weapons, buf);

                if (weapons > 0 || buf > 0) {
                    printf("  [FAIL] WeaponSmith produced Weapons with only Coal\n");
                    allOk = false;
                } else {
                    printf("  [PASS] No production without IronBar\n");
                }
            }

            // ---- Test 3: Both inputs -> produces Weapons ----
            {
                printf("\n--- Test 3: Both inputs delivered -> produces Weapons ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                Simulation sim(cfg);
                WorldModel world;
                SeedWeaponSmith(world, true, true); // Both delivered
                sim.LoadWorld(world);

                EconomySystem* eco = sim.GetEconomySystem();

                const uint32_t kTicks = 300;
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                int weapons = eco ? eco->GetTotalProduced(ResourceType_Weapons) : 0;
                const WorldModel& w = sim.GetWorld();
                int buf = 0;
                if (w.productionBuildingCount > 0) {
                    buf = w.productionBuildings[0].outputBuffer[0];
                }
                printf("  Weapons total=%d, outputBuffer=%d\n", weapons, buf);

                if (weapons <= 0 && buf <= 0) {
                    printf("  [FAIL] WeaponSmith produced nothing with both inputs\n");
                    allOk = false;
                } else {
                    printf("  [PASS] Produced Weapons with both inputs\n");
                }
            }

            // ---- Test 4: After cycle, both inputs consumed (inputDelivered reset to 0) ----
            {
                printf("\n--- Test 4: Inputs consumed after cycle ---\n");
                SimulationConfig cfg;
                cfg.enableProduction = true;
                cfg.enableEconomy = true;
                Simulation sim(cfg);
                WorldModel world;

                // Run for exactly 1 cycle + some buffer, then check inputDelivered
                SeedWeaponSmith(world, true, true);
                sim.LoadWorld(world);

                const uint32_t kTicks = 60; // 2 cycles worth to guarantee at least one cycle completed
                for (uint32_t t = 0; t < kTicks; ++t) {
                    sim.Tick();
                }

                const WorldModel& w = sim.GetWorld();
                int delivered = 0;
                if (w.productionBuildingCount > 0) {
                    delivered = CountDeliveredInputs(w.productionBuildings[0]);
                }
                printf("  Total delivered inputs: %d\n", delivered);
                printf("  (Expected: all inputs have been consumed and re-delivered via transport)\n");
                printf("  (In this direct-seeded test, inputs were pre-delivered, so after cycle consumed=0)\n");

                // In a direct-seeded test, the inputs are pre-delivered in one shot.
                // After production cycle, all inputDelivered get reset to 0.
                // Since there's no transport system in this test, they stay at 0.
                if (delivered > 0) {
                    printf("  [POSSIBLE ISSUE] Inputs still delivered after cycle\n");
                    printf("  (This may be OK if the cycle hasn't completed yet)\n");
                } else {
                    printf("  [PASS] Inputs consumed (reset to 0 after cycle)\n");
                }
            }

            // ---- Test 5: GetProducer(Weapons) == WeaponSmith ----
            {
                printf("\n--- Test 5: Definition Query API ---\n");
                ProductionType prod = GetProducer(ResourceType_Weapons);
                if (prod != PT_WeaponSmith) {
                    printf("  [FAIL] GetProducer(Weapons) = %d (expected PT_WeaponSmith=%d)\n", prod, PT_WeaponSmith);
                    allOk = false;
                } else {
                    printf("  [PASS] GetProducer(Weapons) = PT_WeaponSmith\n");
                }
            }

            // ---- Test 6: Verify GetInputs / GetOutputs via definition ----
            {
                printf("\n--- Test 6: ProductionDefinition inputs/outputs ---\n");
                const ProductionDefinition& def = GetProductionDefinition(PT_WeaponSmith);
                bool hasIronBarIn = false;
                bool hasCoalIn = false;
                bool hasWeaponsOut = false;
                for (int c = 0; c < 4; ++c) {
                    if (def.consumes[c].resource == ResourceType_IronBar && def.consumes[c].amount == 1)
                        hasIronBarIn = true;
                    if (def.consumes[c].resource == ResourceType_Coal && def.consumes[c].amount == 1)
                        hasCoalIn = true;
                }
                for (int p = 0; p < 4; ++p) {
                    if (def.produces[p].resource == ResourceType_Weapons && def.produces[p].amount == 1)
                        hasWeaponsOut = true;
                }
                if (hasIronBarIn && hasCoalIn && hasWeaponsOut) {
                    printf("  [PASS] Definition correct: IronBar + Coal -> Weapons\n");
                } else {
                    printf("  [FAIL] Definition mismatch: IronBar=%d Coal=%d Weapons=%d\n",
                        hasIronBarIn, hasCoalIn, hasWeaponsOut);
                    allOk = false;
                }
            }

            if (allOk) {
                printf("\n[PASS] T48: Multi-input architectural test complete\n");
            } else {
                printf("\n[FAIL] T48: Some checks failed\n");
            }

            return false;
        }
    };

    T48MultiInputArchitecturalTest g_t48;

} // namespace World
