#include "../../Testing/ISimulationScenario.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../Settlement/SettlementSystem.h"
#include "../../Systems/EconomySystem.h"
#include "../../Systems/JobManager.h"
#include "../../World/WorldModel.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Core/JobTypes.h"

namespace World {

    class T55ExpansionRulesTest : public ISimulationScenario {
    public:
        T55ExpansionRulesTest() {}

        virtual const char* GetName() const { return "T55"; }
        virtual const char* GetDescription() const { return "ExpansionRulesTest — verify expansion event recording"; }

        virtual void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableConstruction = true;
            config.enableSettlement = true;
            config.enableWorkers = true;
            config.enableConsumption = false;
        }

        virtual void Initialize(Simulation& sim)
        {
            WorldModel world;
            world.width = 50;
            world.height = 50;
            sim.LoadWorld(world);

            // Add workers to execute construction jobs
            WorldModel& loaded = sim.GetWorld();
            for (int i = 0; i < 10; ++i) {
                if (loaded.workerCount >= kMaxWorkers) break;
                Worker& w = loaded.workers[loaded.workerCount++];
                w.id = i;
                w.state = WorkerState_Idle;
                w.currentJob = 0;
                w.workTicksRemaining = 0;
            }

            // Seed trees for forestry rules
            loaded.treeMatureCount = 200;
            loaded.treeEmptySpots = 300;
        }

        virtual bool Tick(Simulation& sim)
        {
            const WorldModel& world = sim.GetWorld();
            uint32_t tick = sim.GetState().tickCount;

            // Run for enough ticks for bootstrap to complete
            if (tick >= 2000) {
                return Verify(sim, world);
            }

            return true;
        }

    private:
        bool Verify(Simulation& sim, const WorldModel& world)
        {
            bool ok = true;

            SettlementSystem* settlement = sim.GetSettlementSystem();
            if (settlement == NULL) {
                printf("[FAIL] T55: SettlementSystem is NULL\n");
                return false;
            }

            int eventCount = settlement->GetExpansionEventCount();
            printf("[T55] Expansion events recorded: %d\n", eventCount);

            if (eventCount == 0) {
                printf("[FAIL] T55: No expansion events recorded\n");
                return false;
            }

            // Check BootstrapProduction fired for Woodcutter
            {
                const ExpansionEvent& ev = settlement->GetExpansionEvent(0);
                if (ev.ruleId != ER_Bootstrap_Woodcutter) {
                    printf("[FAIL] T55: First event expected ER_Bootstrap_Woodcutter, got ruleId=%d\n", (int)ev.ruleId);
                    ok = false;
                } else {
                    printf("[PASS] T55: First event is Bootstrap_Woodcutter\n");
                }

                if (ev.buildingType != BuildingType_Woodcutter) {
                    printf("[FAIL] T55: First event expected BuildingType_Woodcutter, got type=%d\n", (int)ev.buildingType);
                    ok = false;
                } else {
                    printf("[PASS] T55: First event building type is Woodcutter\n");
                }

                if (ev.tick == 0) {
                    printf("[FAIL] T55: First event tick is 0 (uninitialized)\n");
                    ok = false;
                } else {
                    printf("[PASS] T55: First event tick is %u\n", ev.tick);
                }

                printf("[T55]  Event 0: tick=%u rule=%d building=%d woodFlow=%d woodPot=%.2f stoneFlow=%d stonePot=%.2f availWood=%d\n",
                    ev.tick, (int)ev.ruleId, (int)ev.buildingType,
                    ev.woodFlow, ev.woodPotential, ev.stoneFlow, ev.stonePotential, ev.availableWood);
            }

            // Verify Woodcutter was actually built
            bool hasWoodcutter = false;
            for (int i = 0; i < world.productionBuildingCount; ++i) {
                if (world.productionBuildings[i].type == BuildingType_Woodcutter) {
                    hasWoodcutter = true;
                    break;
                }
            }
            if (!hasWoodcutter) {
                printf("[FAIL] T55: Woodcutter was not built after 2000 ticks\n");
                ok = false;
            } else {
                printf("[PASS] T55: Woodcutter was built\n");
            }

            // If multiple events, verify ordering and data integrity
            for (int i = 1; i < eventCount; ++i) {
                const ExpansionEvent& prev = settlement->GetExpansionEvent(i - 1);
                const ExpansionEvent& ev = settlement->GetExpansionEvent(i);

                if (ev.tick < prev.tick) {
                    printf("[FAIL] T55: Events out of order at index %d (tick %u < %u)\n", i, ev.tick, prev.tick);
                    ok = false;
                }

                if (ev.ruleId <= ER_None || ev.ruleId >= ER_Count) {
                    printf("[FAIL] T55: Event %d has invalid ruleId=%d\n", i, (int)ev.ruleId);
                    ok = false;
                }

                if (ev.buildingType == BuildingType_None) {
                    printf("[FAIL] T55: Event %d has BuildingType_None\n", i);
                    ok = false;
                }

                printf("[T55]  Event %d: tick=%u rule=%d building=%d woodFlow=%d woodPot=%.2f stoneFlow=%d stonePot=%.2f availWood=%d\n",
                    i, ev.tick, (int)ev.ruleId, (int)ev.buildingType,
                    ev.woodFlow, ev.woodPotential, ev.stoneFlow, ev.stonePotential, ev.availableWood);
            }

            if (eventCount >= 2) {
                printf("[PASS] T55: Multiple expansion rules fired (%d)\n", eventCount);
            } else {
                printf("[T55] Note: Only one expansion rule fired\n");
            }

            if (ok) {
                printf("[PASS] T55: All expansion event checks passed\n");
            } else {
                printf("[FAIL] T55: Some expansion event checks failed\n");
            }

            return false; // stop
        }
    };

    T55ExpansionRulesTest g_t55ExpansionRulesTest;

}
