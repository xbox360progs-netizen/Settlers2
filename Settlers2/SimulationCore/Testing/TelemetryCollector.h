#pragma once
#include "TelemetryTypes.h"
#include "../Simulation/Simulation.h"

namespace World {

    class TelemetryCollector {
    public:
        TelemetryCollector(int windowSize = 1000);

        void Tick(Simulation& sim);
        TelemetryWindow FlushWindow(Simulation& sim);

        float GetAverageDeliveryLatency() const;
        float GetMaxDeliveryLatency() const;
        float GetAverageRouteLength() const;
        float GetBuildingUtilization() const;
        float GetWorkerUtilization() const;

    private:
        void AccumulatePerTick(Simulation& sim);
        TelemetryWindow ComputeWindow(Simulation& sim);
        void SampleDeliveries(Simulation& sim, TelemetryWindow& w);

        int m_windowSize;
        int m_tickCount;

        int m_sumActiveTasks;
        int m_sumBlockedTasks;

        int m_sumActiveBuildings;
        int m_sumTotalBuildings;
        int m_lastProduced[kTelemetryMaxResources];
        bool m_hasBaseline;

        int m_sumIdleWorkers;
        int m_sumWorkingWorkers;

        uint32_t m_deliveryWatermark;

        TelemetryWindow m_lastWindow;
    };

}
