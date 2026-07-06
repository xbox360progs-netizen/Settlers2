#include "TelemetryCollector.h"
#include "../Systems/EconomySystem.h"
#include "../World/WorldModel.h"
#include "../Transport/TransportController.h"
#include "../Transport/TransportTask.h"
#include "../Core/ResourceTypes.h"
#include <string.h>
#include <stddef.h>

namespace World {

    TelemetryCollector::TelemetryCollector(int windowSize)
        : m_windowSize(windowSize)
        , m_tickCount(0)
        , m_sumActiveTasks(0)
        , m_sumBlockedTasks(0)
        , m_sumActiveBuildings(0)
        , m_sumTotalBuildings(0)
        , m_hasBaseline(false)
        , m_sumIdleWorkers(0)
        , m_sumWorkingWorkers(0)
        , m_deliveryWatermark(0)
    {
        memset(m_lastProduced, 0, sizeof(m_lastProduced));
        memset(&m_lastWindow, 0, sizeof(m_lastWindow));
    }

    void TelemetryCollector::Tick(Simulation& sim)
    {
        ++m_tickCount;
        AccumulatePerTick(sim);
    }

    void TelemetryCollector::AccumulatePerTick(Simulation& sim)
    {
        const SimulationState& state = sim.GetState();
        const WorldModel& world = sim.GetWorld();

        m_sumActiveTasks += state.activeTransportTasks;
        m_sumBlockedTasks += state.blockedTransportTasks;

        int activeCount = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].active) {
                activeCount++;
            }
        }
        m_sumActiveBuildings += activeCount;
        m_sumTotalBuildings += world.productionBuildingCount;

        int idleCount = 0;
        int workingCount = 0;
        for (int i = 0; i < world.workerCount; ++i) {
            if (world.workers[i].state == WorkerState_Idle) {
                idleCount++;
            } else {
                workingCount++;
            }
        }
        m_sumIdleWorkers += idleCount;
        m_sumWorkingWorkers += workingCount;
    }

    TelemetryWindow TelemetryCollector::FlushWindow(Simulation& sim)
    {
        if (m_tickCount == 0) {
            TelemetryWindow empty;
            memset(&empty, 0, sizeof(empty));
            return empty;
        }

        TelemetryWindow w;
        memset(&w, 0, sizeof(w));
        w.tick = sim.GetState().tickCount;

        int n = m_tickCount;
        float fn = static_cast<float>(n);

        // — Logistics —
        w.logistics.avgActiveTasks = static_cast<float>(m_sumActiveTasks) / fn;
        w.logistics.avgBlockedTasks = static_cast<float>(m_sumBlockedTasks) / fn;

        // Sample delivery latency and route length from task pool
        SampleDeliveries(sim, w);

        // — Production —
        if (m_sumTotalBuildings > 0) {
            w.production.buildingUtilization =
                static_cast<float>(m_sumActiveBuildings) / static_cast<float>(m_sumTotalBuildings);
        }
        const WorldModel& world = sim.GetWorld();
        w.production.activeBuildings = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].active) w.production.activeBuildings++;
        }
        w.production.totalBuildings = world.productionBuildingCount;

        // Production throughput (delta from last flush)
        const EconomySystem* eco = sim.GetEconomySystem();
        if (eco != NULL) {
            if (m_hasBaseline) {
                for (int r = 1; r < kTelemetryMaxResources; ++r) {
                    int current = eco->GetTotalProduced(static_cast<ResourceType>(r));
                    w.production.throughput[r] = current - m_lastProduced[r];
                }
            }
            // Record baseline for next window
            for (int r = 1; r < kTelemetryMaxResources; ++r) {
                m_lastProduced[r] = eco->GetTotalProduced(static_cast<ResourceType>(r));
            }
            m_hasBaseline = true;
        }

        // — Workers —
        w.workers.totalWorkers = world.workerCount;
        if (world.workerCount > 0) {
            float avgWorking = static_cast<float>(m_sumWorkingWorkers) / fn;
            float avgIdle = static_cast<float>(m_sumIdleWorkers) / fn;
            w.workers.avgIdle = avgIdle;
            w.workers.avgWorking = avgWorking;
            float total = avgWorking + avgIdle;
            w.workers.utilization = (total > 0.0f) ? (avgWorking / total) : 0.0f;
        }

        // Reset accumulators for next window
        m_tickCount = 0;
        m_sumActiveTasks = 0;
        m_sumBlockedTasks = 0;
        m_sumActiveBuildings = 0;
        m_sumTotalBuildings = 0;
        m_sumIdleWorkers = 0;
        m_sumWorkingWorkers = 0;

        m_lastWindow = w;
        return w;
    }

    void TelemetryCollector::SampleDeliveries(Simulation& sim, TelemetryWindow& w)
    {
        const TransportController* tc = sim.GetTransportController();
        if (tc == NULL) return;

        uint32_t currentTick = sim.GetState().tickCount;
        const TransportTask* pool = tc->GetTaskPool();
        int poolSize = tc->GetPoolSize();

        int latencySum = 0;
        float maxLatency = 0.0f;
        int routeSum = 0;
        int count = 0;
        uint32_t maxId = m_deliveryWatermark;

        for (int i = 0; i < poolSize; ++i) {
            const TransportTask& task = pool[i];
            if (task.state != TTS_Delivered) continue;
            if (task.id <= m_deliveryWatermark) continue;

            uint32_t age = 0;
            if (task.createdTick < currentTick) {
                age = currentTick - task.createdTick;
            }
            latencySum += static_cast<int>(age);
            if (static_cast<float>(age) > maxLatency) {
                maxLatency = static_cast<float>(age);
            }
            routeSum += static_cast<int>(task.route.count);
            count++;
            if (task.id > maxId) maxId = task.id;
        }

        m_deliveryWatermark = maxId;

        w.logistics.deliveryCount = count;
        if (count > 0) {
            w.logistics.avgDeliveryLatency = static_cast<float>(latencySum) / static_cast<float>(count);
            w.logistics.maxDeliveryLatency = maxLatency;
            w.logistics.avgRouteLength = static_cast<float>(routeSum) / static_cast<float>(count);
        }
    }

    float TelemetryCollector::GetAverageDeliveryLatency() const
    {
        return m_lastWindow.logistics.avgDeliveryLatency;
    }

    float TelemetryCollector::GetMaxDeliveryLatency() const
    {
        return m_lastWindow.logistics.maxDeliveryLatency;
    }

    float TelemetryCollector::GetAverageRouteLength() const
    {
        return m_lastWindow.logistics.avgRouteLength;
    }

    float TelemetryCollector::GetBuildingUtilization() const
    {
        return m_lastWindow.production.buildingUtilization;
    }

    float TelemetryCollector::GetWorkerUtilization() const
    {
        return m_lastWindow.workers.utilization;
    }

} // namespace World
