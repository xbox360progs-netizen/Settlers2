#pragma once
#include "ISimulationScenario.h"
#include "EconomyMetrics.h"
#include "../Simulation/Simulation.h"
#include "../Core/TreeSystem.h"
#include <stdio.h>

namespace World {

    class SoakTestBase : public ISimulationScenario {
    public:
        SoakTestBase(
            const char* name,
            uint32_t soakTicks,
            uint32_t checkInterval = 1000)
            : m_name(name)
            , m_soakTicks(soakTicks)
            , m_checkInterval(checkInterval)
            , m_metricsCount(0)
        {
        }

        const char* GetName() const { return m_name; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableConstruction = true;
            config.enableWarehouse = true;
            config.enableSettlement = true;
            config.enableTreeDepletion = true;
        }

        void Initialize(Simulation& sim)
        {
            WorldModel world;
            world.width = 50;
            world.height = 50;
            SeedTrees(world, 500, 500);
            sim.LoadWorld(world);
        }

        bool Tick(Simulation& sim)
        {
            uint32_t currentTick = sim.GetState().tickCount;

            if (currentTick > 0 && currentTick % m_checkInterval == 0) {
                if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                    printf("[FAIL][%s] Invariant failed at tick %u\n", m_name, currentTick);
                    return false;
                }

                const WorldModel& world = sim.GetWorld();
                EconomySystem* eco = sim.GetEconomySystem();
                WarehouseSystem* wh = sim.GetWarehouseSystem();

                EconomyMetrics m = CollectEconomyMetrics(world, eco, wh);
                RecordMetric(m, currentTick);

                if (!ReportAndCheckMetrics(m, currentTick, m_name)) {
                    printf("[FAIL][%s] Metric violation at tick %u\n", m_name, currentTick);
                    return false;
                }
            }

            if (currentTick >= m_soakTicks) {
                bool ok = Verify(sim);
                return false;
            }
            return true;
        }

        virtual bool Verify(Simulation& sim)
        {
            EconomySystem* eco = sim.GetEconomySystem();
            if (eco == NULL) {
                printf("[FAIL][%s] EconomySystem not available\n", m_name);
                return false;
            }

            const WorldModel& world = sim.GetWorld();

            EconomyMetrics finalMetrics = CollectEconomyMetrics(
                world, eco, sim.GetWarehouseSystem());

            printf("\n=== [%s] Soak Complete: %u ticks ===\n", m_name, m_soakTicks);
            printf("  Final metrics:\n");
            ReportAndCheckMetrics(finalMetrics, m_soakTicks, m_name);
            printf("  Metrics collected: %d checkpoints\n", m_metricsCount);

            // Verify all 4 resources were produced over the full run
            int totalWood = eco->GetTotalProduced(ResourceType_Wood);
            int totalPlanks = eco->GetTotalProduced(ResourceType_Planks);
            int totalStone = eco->GetTotalProduced(ResourceType_Stone);
            int totalTools = eco->GetTotalProduced(ResourceType_Tools);

            printf("  Total produced: Wood=%d Planks=%d Stone=%d Tools=%d\n",
                totalWood, totalPlanks, totalStone, totalTools);

            bool ok = true;
            if (totalWood <= 0)   { printf("[FAIL][%s] No Wood produced\n", m_name);    ok = false; }
            if (totalPlanks <= 0) { printf("[FAIL][%s] No Planks produced\n", m_name);  ok = false; }
            if (totalStone <= 0)  { printf("[FAIL][%s] No Stone produced\n", m_name);   ok = false; }
            if (totalTools <= 0)  { printf("[FAIL][%s] No Tools produced\n", m_name);   ok = false; }

            // Verify flow <= potential for all resources
            ResourceType resources[] = {
                ResourceType_Wood, ResourceType_Planks,
                ResourceType_Stone, ResourceType_Tools
            };
            for (int r = 0; r < 4; ++r) {
                int flow = eco->GetResourceFlow(resources[r]);
                float potential = eco->GetProductionPotential(resources[r], world);
                if (flow > static_cast<int>(potential * EconomySystem::kFlowWindow + 0.5f)) {
                    printf("[FAIL][%s] Flow exceeds potential for resource %d: %d > %.4f * %d\n",
                        m_name, (int)resources[r], flow, potential, EconomySystem::kFlowWindow);
                    ok = false;
                }
            }

            if (ok) {
                printf("[PASS][%s] All invariants held for %u ticks\n", m_name, m_soakTicks);
            }
            return ok;
        }

    protected:
        virtual void RecordMetric(const EconomyMetrics& m, uint32_t tick)
        {
            m_metricsCount++;
            // Store for trend analysis — for now just count checkpoints
        }

        const char* m_name;
        uint32_t m_soakTicks;
        uint32_t m_checkInterval;
        int m_metricsCount;

        // Trend storage (for future degeneration detection)
        static const int kMaxMetrics = 500;
        EconomyMetrics m_metricHistory[kMaxMetrics];
        uint32_t m_metricTicks[kMaxMetrics];
    };

}
