#pragma once
#include "../World/WorldModel.h"
#include "../Core/ResourceTypes.h"
#include "../Core/ProductionTypes.h"
#include <stdint.h>
#include <stddef.h>

namespace World {

    class EconomySystem;
    class WarehouseSystem;

    // Per-resource snapshot at a given tick
    struct ResourceMetrics {
        ResourceType type;
        int totalProduced;
        int totalConsumed;
        int flow;
        int stockpile;
        int outputBuffer;
    };

    // Per-production-type building snapshot
    struct BuildingMetrics {
        ProductionType pt;
        int count;
        int activeCount;
    };

    // Ecology state snapshot
    struct EcologyMetrics {
        int matureTrees;
        int emptySpots;
        int animals;
        int fish;
    };

    // A single economic checkpoint
    struct EconomySnapshot {
        uint32_t tick;
        ResourceMetrics resources[static_cast<int>(ResourceType_Count)];
        int resourceCount;
        BuildingMetrics buildings[static_cast<int>(PT_Count)];
        int buildingCount;
        EcologyMetrics ecology;
        int idleWorkers;
        int pendingRequests;
    };

    // Accumulated snapshot history across a long soak run
    struct EconomySnapshotHistory {
        static const int kMaxSnapshots = 20;
        EconomySnapshot snapshots[kMaxSnapshots];
        int snapshotCount;
    };

    // Summary metrics for a single checkpoint (used by SoakHarness).
    struct EconomyMetrics {
        uint32_t tick;
        int totalProduced[ResourceType_Count];
        int totalConsumed[ResourceType_Count];
        int flow[ResourceType_Count];
        float potential[ResourceType_Count];
        int stockpile[ResourceType_Count];
        int outputBuffer[ResourceType_Count];
        int buildingCount;
        int idleWorkers;
        int pendingRequests;
    };

    // Collect economy metrics from current simulation state.
    EconomyMetrics CollectEconomyMetrics(
        const WorldModel& world,
        const EconomySystem* eco,
        const WarehouseSystem* wh
    );

    // Report and check metrics — returns true if all invariants hold.
    // If world is non-NULL, per-building diagnostics are printed
    // when flow exceeds potential.
    bool ReportAndCheckMetrics(
        const EconomyMetrics& m,
        uint32_t tick,
        const char* name,
        const WorldModel* world = NULL,
        const EconomySystem* eco = NULL
    );

    // Collect a full snapshot of the current economy state.
    EconomySnapshot CollectSnapshot(
        const WorldModel& world,
        const EconomySystem* eco,
        const WarehouseSystem* wh
    );

    // Append a snapshot to the history (if space permits).
    void RecordSnapshot(
        EconomySnapshotHistory& history,
        const WorldModel& world,
        const EconomySystem* eco,
        const WarehouseSystem* wh
    );

    // Print a comparison report across all snapshots.
    // Returns false if any supply crisis or regression is detected.
    bool ReportSnapshotComparison(
        const EconomySnapshotHistory& history,
        uint32_t finalTick,
        const char* name
    );

    // Print ecology metrics at a checkpoint.
    bool ReportEcologyMetrics(const EcologyMetrics& e, uint32_t tick);

    // Compute the discrete production upper bound per window:
    // Σ active_producers outputPerCycle * (floor(window / cycleTime) + 1)
    // This is the strict physical maximum independent of phase alignment
    // between production cycles and the measurement window.
    // Unlike GetProductionPotential() * window (continuous rate), this
    // accounts for the fact that multiple discrete completions from
    // multiple producers can cluster within one window.
    int ComputeDiscreteProductionUpperBound(
        ResourceType type,
        const WorldModel& world,
        int windowSize
    );

    // Diagnostic: when flow exceeds potential, print per-building breakdown
    // to identify whether the cause is inactive buildings, burst overlap, or
    // measurement mismatch. Shows both continuous potential and discrete upper bound.
    void DiagnoseFlowVsPotential(
        const WorldModel& world,
        const EconomySystem* eco,
        ResourceType rt,
        int observedFlow,
        float potential,
        uint32_t tick,
        const char* name
    );

    // ──────────────────────────────────────────────
    // Level 2 & 3: Window statistics + stability
    // ──────────────────────────────────────────────

    // Per-resource accumulator — updated every tick
    struct FlowAccumulator {
        int sum;
        int min;
        int max;
        int count;
        int sumSq;
    };

    // Computed statistics for one resource across a window
    struct WindowFlowStats {
        float mean;
        float stddev;
        float cv;         // coefficient of variation (stddev/mean, percent)
        int min;
        int max;
        float trend;      // % change from previous window mean
        int sampleCount;
    };

    // Stability classification for a single resource
    enum StabilityClass {
        SC_Stable = 0,
        SC_Oscillating,
        SC_Growing,
        SC_Declining,
        SC_Unknown
    };

    // Per-resource stability result (one per tracked resource)
    struct ResourceStability {
        ResourceType type;
        WindowFlowStats stats;
        StabilityClass classification;
    };

    // Per-tick flow tracker — aggregates flow over a window.
    // Usage:
    //   1. Create once per soak run
    //   2. Call AccumulateTick(eco) every tick
    //   3. Every N ticks (e.g. 1000), call FlushWindow() to get stats
    class EconomyFlowTracker {
    public:
        EconomyFlowTracker();

        // Record current flow for all produced resources (call every tick)
        void AccumulateTick(const EconomySystem* eco);

        // Compute window stats from accumulators, reset for next window.
        // outStats must be at least kMaxResources in size.
        // Returns number of resources with data written.
        int FlushWindow(WindowFlowStats* outStats);

        // Classify stability from window statistics
        static StabilityClass ClassifyStability(const WindowFlowStats& stats);

        // Maximum number of resources the tracker can hold
        static const int kMaxResources = static_cast<int>(ResourceType_Count);

        // Fill array with oscillation streak counts (consecutive windows
        // a resource has been Oscillating or Declining). outStreaks must
        // be at least kMaxResources in size.
        void GetOscillationStreaks(int* outStreaks) const;

    private:
        FlowAccumulator m_accum[kMaxResources];
        float m_prevMeans[kMaxResources];
        int m_prevCount;
        int m_oscillationStreak[kMaxResources];
    };

    // Print a stability report for a set of window stats.
    // stats must be indexed by resource type (not sparse).
    // If oscillationStreaks is non-NULL, an extra "Streak" column is printed.
    void PrintStabilityReport(
        const WindowFlowStats* stats,
        int resourceCount,
        const char* name,
        const int* oscillationStreaks = NULL
    );

    // Print stability propagation analysis — walks the production
    // dependency graph and annotates each edge with upstream/downstream
    // stability classification. Identifies potential root causes where
    // upstream instability correlates with downstream oscillation.
    // If oscillationStreaks is non-NULL, recovery streaks are shown.
    void PrintStabilityPropagation(
        const WindowFlowStats* stats,
        int resourceCount,
        const char* name,
        const int* oscillationStreaks = NULL
    );

}


