#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"

namespace World {

    static const int kTelemetryMaxResources = static_cast<int>(ResourceType_Count);

    struct LogisticsMetrics {
        float avgDeliveryLatency;
        float maxDeliveryLatency;
        float avgRouteLength;
        int deliveryCount;
        float avgActiveTasks;
        float avgBlockedTasks;
    };

    struct ProductionMetrics {
        float buildingUtilization;
        int activeBuildings;
        int totalBuildings;
        int throughput[kTelemetryMaxResources];
    };

    struct WorkerMetrics {
        float utilization;
        float avgIdle;
        float avgWorking;
        int totalWorkers;
    };

    struct TelemetryWindow {
        uint32_t tick;
        LogisticsMetrics logistics;
        ProductionMetrics production;
        WorkerMetrics workers;
    };

    static const int kMaxTelemetryWindows = 256;

}
