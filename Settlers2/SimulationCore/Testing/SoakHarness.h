#pragma once
#include "ISimulationScenario.h"
#include "EconomyMetrics.h"
#include "IntegrityAudit.h"
#include "TelemetryCollector.h"
#include "../Simulation/Simulation.h"
#include "../Core/TreeSystem.h"
#include "../Systems/EconomySystem.h"
#include "../Warehouse/WarehouseSystem.h"
#include <stdio.h>
#include <string.h>

namespace World {

    class SoakTestBase : public ISimulationScenario {
    public:
        SoakTestBase(
            const char* name,
            uint32_t soakTicks,
            uint32_t checkInterval = 1000,
            uint32_t bootstrapTicks = 10000)
            : m_name(name)
            , m_soakTicks(soakTicks)
            , m_checkInterval(checkInterval)
            , m_bootstrapTicks(bootstrapTicks)
            , m_metricsCount(0)
            , m_windowCount(0)
            , m_disableWarehouse(false)
            , m_telemetryWindowCount(0)
        {
            memset(m_windowStats, 0, sizeof(m_windowStats));
            memset(m_telemetryWindows, 0, sizeof(m_telemetryWindows));
            memset(m_telemetryWindowTicks, 0, sizeof(m_telemetryWindowTicks));
        }

        const char* GetName() const { return m_name; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableConstruction = true;
            config.enableWarehouse = !m_disableWarehouse;
            config.enableSettlement = true;
            config.enableWorkers = true;
            config.enableTreeDepletion = true;
        }

        void Initialize(Simulation& sim)
        {
            WorldModel world;
            world.width = 50;
            world.height = 50;
            SeedTrees(world, 500, 500);
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
        }

        bool Tick(Simulation& sim)
        {
            uint32_t currentTick = sim.GetState().tickCount;

            // Per-tick flow accumulation
            EconomySystem* eco = sim.GetEconomySystem();
            if (eco != NULL) {
                m_flowTracker.AccumulateTick(eco);
            }

            // Per-tick telemetry
            m_telemetry.Tick(sim);

            if (currentTick > 0 && currentTick % m_checkInterval == 0) {
                if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                    printf("[FAIL][%s] Invariant failed at tick %u\n", m_name, currentTick);
                    return false;
                }

                AuditReport audit = RunIntegrityAudit(sim);
                if (!audit.AllPassed()) {
                    PrintAuditReport(audit, m_name, currentTick);
                    printf("[FAIL][%s] Integrity audit failed at tick %u\n", m_name, currentTick);
                    return false;
                }

                    const WorldModel& world = sim.GetWorld();
                WarehouseSystem* wh = sim.GetWarehouseSystem();

                EconomyMetrics m = CollectEconomyMetrics(world, eco, wh);
                RecordMetric(m, currentTick);

                if (currentTick >= m_bootstrapTicks && !ReportAndCheckMetrics(m, currentTick, m_name, &world, eco)) {
                    printf("[FAIL][%s] Metric violation at tick %u\n", m_name, currentTick);
                    return false;
                }

                // Record snapshot and flush flow window
                if (m_snapshotHistory.snapshotCount < EconomySnapshotHistory::kMaxSnapshots) {
                    EconomySnapshot snap = CollectSnapshot(world, eco, wh);
                    snap.tick = currentTick;
                    m_snapshotHistory.snapshots[m_snapshotHistory.snapshotCount] = snap;
                    m_snapshotHistory.snapshotCount++;
                }

                WindowFlowStats stats[EconomyFlowTracker::kMaxResources];
                int flushed = m_flowTracker.FlushWindow(stats);
                if (m_windowCount < kMaxWindows) {
                    for (int r = 0; r < EconomyFlowTracker::kMaxResources; ++r) {
                        m_windowStats[r][m_windowCount] = stats[r];
                    }
                    m_windowTicks[m_windowCount] = currentTick;
                    m_windowCount++;
                }

                // Flush telemetry window
                TelemetryWindow telWin = m_telemetry.FlushWindow(sim);
                if (m_telemetryWindowCount < kMaxTelemetryWindows) {
                    m_telemetryWindows[m_telemetryWindowCount] = telWin;
                    m_telemetryWindowTicks[m_telemetryWindowCount] = currentTick;
                    m_telemetryWindowCount++;
                }
            }

            if (currentTick >= m_soakTicks) {
                Verify(sim);
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
            ReportAndCheckMetrics(finalMetrics, m_soakTicks, m_name, &world, eco);
            printf("  Metrics collected: %d checkpoints\n", m_metricsCount);

            // Prepare window stats for stability report + flow checks
            WindowFlowStats lastStats[EconomyFlowTracker::kMaxResources];
            int oscStreaks[EconomyFlowTracker::kMaxResources];
            bool haveWindowStats = (m_windowCount > 0);
            if (haveWindowStats) {
                for (int r = 0; r < EconomyFlowTracker::kMaxResources; ++r) {
                    lastStats[r] = m_windowStats[r][m_windowCount - 1];
                }
                m_flowTracker.GetOscillationStreaks(oscStreaks);

                PrintStabilityReport(lastStats, EconomyFlowTracker::kMaxResources, m_name, oscStreaks);
                PrintStabilityPropagation(lastStats, EconomyFlowTracker::kMaxResources, m_name, oscStreaks);
            }

            // Print snapshot comparison
            ReportSnapshotComparison(m_snapshotHistory, m_soakTicks, m_name);

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

            // Verify flow <= discrete production upper bound for all resources
            if (haveWindowStats) {
                for (int r = 1; r < EconomyFlowTracker::kMaxResources; ++r) {
                    const WindowFlowStats& s = lastStats[r];
                    if (s.sampleCount == 0) continue;
                    ResourceType rt = static_cast<ResourceType>(r);
                    float potential = eco->GetProductionPotential(rt, world);
                    if (potential <= 0.0f) continue;
                    int discreteBound = ComputeDiscreteProductionUpperBound(rt, world, EconomySystem::kFlowWindow);
                    int meanFlow = static_cast<int>(s.mean + 0.5f);
                    if (meanFlow > discreteBound) {
                        printf("[FAIL][%s] Flow exceeds discrete bound for %s: mean %d > %d\n",
                            m_name, ResourceTypeToString(rt), meanFlow, discreteBound);
                        DiagnoseFlowVsPotential(world, eco, rt,
                            meanFlow, potential, m_soakTicks, m_name);
                        ok = false;
                    }
                }
            }

            // Final integrity audit
            AuditReport finalAudit = RunIntegrityAudit(sim);
            PrintAuditReport(finalAudit, m_name, m_soakTicks);
            if (!finalAudit.AllPassed()) {
                printf("[FAIL][%s] Integrity audit failed\n", m_name);
                ok = false;
            }

            // Print last stored telemetry window (collected at checkpoint)
            if (m_telemetryWindowCount > 0) {
                const TelemetryWindow& last = m_telemetryWindows[m_telemetryWindowCount - 1];
                printf("\n=== Telemetry [%s] tick=%u ===\n", m_name, last.tick);
                printf("  Deliveries:   %d (avg %.1f ticks, max %.0f, avg route %.1f hops)\n",
                    last.logistics.deliveryCount,
                    last.logistics.avgDeliveryLatency,
                    last.logistics.maxDeliveryLatency,
                    last.logistics.avgRouteLength);
                printf("  Transport:    avg %.1f active, %.1f blocked\n",
                    last.logistics.avgActiveTasks,
                    last.logistics.avgBlockedTasks);
                printf("  Buildings:    %d/%d active (%.0f%% util)\n",
                    last.production.activeBuildings,
                    last.production.totalBuildings,
                    last.production.buildingUtilization * 100.0f);
                if (eco != NULL) {
                    printf("  Throughput:   Wood=%d Planks=%d Stone=%d Tools=%d\n",
                        last.production.throughput[(int)ResourceType_Wood],
                        last.production.throughput[(int)ResourceType_Planks],
                        last.production.throughput[(int)ResourceType_Stone],
                        last.production.throughput[(int)ResourceType_Tools]);
                }
                printf("  Workers:      %.1f%% utilization (%.1f idle, %.1f working)\n",
                    last.workers.utilization * 100.0f,
                    last.workers.avgIdle,
                    last.workers.avgWorking);
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
            m_metricHistory[m_metricsCount - 1] = m;
            m_metricTicks[m_metricsCount - 1] = tick;
        }

        const char* m_name;
        uint32_t m_soakTicks;
        uint32_t m_checkInterval;
        uint32_t m_bootstrapTicks;
        int m_metricsCount;
        bool m_disableWarehouse;

        // Trend storage
        static const int kMaxMetrics = 500;
        EconomyMetrics m_metricHistory[kMaxMetrics];
        uint32_t m_metricTicks[kMaxMetrics];

        // Flow tracking — accumulates every tick, flushed at checkpoints
        EconomyFlowTracker m_flowTracker;

        // Telemetry — per-tick accumulation, flushed at checkpoints
        TelemetryCollector m_telemetry;

        // Snapshot history — one entry per checkpoint
        EconomySnapshotHistory m_snapshotHistory;

        // Windowed flow stats — one set per checkpoint flush
        static const int kMaxWindows = 256;
        WindowFlowStats m_windowStats[EconomyFlowTracker::kMaxResources][kMaxWindows];
        uint32_t m_windowTicks[kMaxWindows];
        int m_windowCount;

        // Telemetry windows
        TelemetryWindow m_telemetryWindows[kMaxTelemetryWindows];
        uint32_t m_telemetryWindowTicks[kMaxTelemetryWindows];
        int m_telemetryWindowCount;
    };

}
