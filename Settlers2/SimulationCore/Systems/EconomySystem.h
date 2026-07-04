#pragma once
#include <stdint.h>
#include "ISimulationSystem.h"
#include "../Core/ResourceTypes.h"
#include "../Core/ProductionTypes.h"

namespace World {

    struct WorldModel;

    class EconomySystem : public ISimulationSystem {
    public:
        EconomySystem();
        ~EconomySystem();

        void Tick(WorldModel& world);

        // Resource flow tracking accessors
        int GetTotalProduced(ResourceType type) const;
        int GetTotalConsumed(ResourceType type) const;

        // Current available amount — sum of output buffers across all buildings.
        // Observational: reflects resources available for collection/use right now.
        // Semantics: "how much of this resource can Settlement AI consider available?"
        int GetAvailable(ResourceType type, const WorldModel& world) const;

        // Current production flow — units produced in the last flow window.
        // Observational: reflects production throughput, smoothed over a window.
        // Semantics: "is this resource being produced at a steady rate?"
        // Implementation: windowed delta of totalOutput. Window hides per-tick noise.
        int GetResourceFlow(ResourceType type) const;

        // Production potential — maximum steady-state production rate of all active
        // producers, assuming continuous input availability (units per tick).
        // Observational: reflects installed capacity, not current output.
        // Semantics: "how much could this resource be producing right now?"
        // Implementation: for each active producer: sum(outputAmount / cycleTime).
        // Does NOT depend on current input buffers, transport delays, or output blockage.
        float GetProductionPotential(ResourceType type, const WorldModel& world) const;

        // Flow window size (ticks). Exposed so consumers can normalize flow to per-tick.
        static const int kFlowWindow = 50;

        // Building count — number of active production buildings of a given production type.
        int GetBuildingCount(ProductionType type, const WorldModel& world) const;

        // Demand backlog — number of unfulfilled transport requests for a resource.
        int GetDemandBacklog(ResourceType type, const WorldModel& world) const;

    private:
        void ScanProduction(WorldModel& world);

        uint32_t m_tickCount;

        static const int kResourceCount = static_cast<int>(ResourceType_Count);
        static const int kMaxBuildingSlots = 4;
        int m_totalProduced[kResourceCount];
        int m_totalConsumed[kResourceCount];

        // Per-building tracking for delta computation
        static const int kMaxBuildings = 64;
        int m_lastOutput[kMaxBuildings][kMaxBuildingSlots];

        // Flow tracking — accumulates deltas within a window, then snapshots to m_currentFlow
        int m_currentFlow[kResourceCount];
        int m_flowAccumulator[kResourceCount];
        int m_flowElapsed;
    };

} // namespace World
