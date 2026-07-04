#include "EconomyMetrics.h"
#include "../Systems/EconomySystem.h"
#include "../Warehouse/WarehouseSystem.h"
#include "../Core/ResourceTypes.h"
#include "../Definitions/ProductionDefinition.h"
#include "../Definitions/BuildingDefinition.h"
#include "../Core/ResourceDebug.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace World {

    EconomySnapshot CollectSnapshot(
        const WorldModel& world,
        const EconomySystem* eco,
        const WarehouseSystem* wh)
    {
        EconomySnapshot s;
        s.tick = 0;
        s.resourceCount = 0;
        s.buildingCount = 0;
        s.idleWorkers = 0;
        s.pendingRequests = 0;

        memset(s.resources, 0, sizeof(s.resources));
        memset(s.buildings, 0, sizeof(s.buildings));
        memset(&s.ecology, 0, sizeof(s.ecology));

        if (eco == NULL) return s;

        // Resources — scan all defined types that have a producer
        int resIdx = 0;
        for (int r = 1; r < static_cast<int>(ResourceType_Count); ++r) {
            ResourceType rt = static_cast<ResourceType>(r);
            if (GetProducer(rt) == PT_None) continue; // only tracked resources

            if (resIdx >= static_cast<int>(ResourceType_Count)) break;
            ResourceMetrics& rm = s.resources[resIdx];
            rm.type = rt;
            rm.totalProduced = eco->GetTotalProduced(rt);
            rm.totalConsumed = eco->GetTotalConsumed(rt);
            rm.flow = eco->GetResourceFlow(rt);
            rm.stockpile = (wh != NULL) ? wh->GetStockpileAmount(rt) : 0;
            rm.outputBuffer = eco->GetAvailable(rt, world);
            resIdx++;
        }
        s.resourceCount = resIdx;

        // Buildings — scan all production types
        int bldIdx = 0;
        for (int p = 1; p < static_cast<int>(PT_Count); ++p) {
            ProductionType pt = static_cast<ProductionType>(p);
            BuildingType bt = GetBuildingTypeForProduction(pt);
            if (bt == BuildingType_None) continue;

            if (bldIdx >= static_cast<int>(PT_Count)) break;
            BuildingMetrics& bm = s.buildings[bldIdx];
            bm.pt = pt;
            bm.count = 0;
            bm.activeCount = 0;

            for (int i = 0; i < world.productionBuildingCount; ++i) {
                if (world.productionBuildings[i].type == bt) {
                    bm.count++;
                    if (world.productionBuildings[i].active) {
                        bm.activeCount++;
                    }
                }
            }
            bldIdx++;
        }
        s.buildingCount = bldIdx;

        // Ecology
        s.ecology.matureTrees = world.treeMatureCount;
        s.ecology.emptySpots = world.treeEmptySpots;
        s.ecology.animals = world.animalCount;
        s.ecology.fish = world.fishCount;

        // Workers
        for (int i = 0; i < world.workerCount; ++i) {
            if (world.workers[i].state == WorkerState_Idle) {
                s.idleWorkers++;
            }
        }

        s.pendingRequests = world.pendingRequestCount;

        return s;
    }

    void RecordSnapshot(
        EconomySnapshotHistory& history,
        const WorldModel& world,
        const EconomySystem* eco,
        const WarehouseSystem* wh)
    {
        if (history.snapshotCount >= EconomySnapshotHistory::kMaxSnapshots) return;

        EconomySnapshot s = CollectSnapshot(world, eco, wh);
        s.tick = 0; // caller sets tick
        history.snapshots[history.snapshotCount] = s;
        history.snapshotCount++;
    }

    static bool IsEssentialResource(ResourceType type)
    {
        return type == ResourceType_Wood ||
               type == ResourceType_Planks ||
               type == ResourceType_Stone ||
               type == ResourceType_Meat ||
               type == ResourceType_Fish ||
               type == ResourceType_Bread ||
               type == ResourceType_Tools ||
               type == ResourceType_Coal ||
               type == ResourceType_Soldiers;
    }

    bool ReportEcologyMetrics(const EcologyMetrics& e, uint32_t tick)
    {
        printf("  [%6u] Ecology: trees=%d/%d  animals=%d  fish=%d\n",
            tick, e.matureTrees, e.matureTrees + e.emptySpots, e.animals, e.fish);

        // Check: no tree depletion
        if (e.matureTrees < 5 && e.emptySpots > 100) {
            printf("    [WARN] Tree population critically low\n");
        }

        return true;
    }

    bool ReportSnapshotComparison(
        const EconomySnapshotHistory& history,
        uint32_t finalTick,
        const char* name)
    {
        if (history.snapshotCount < 2) {
            printf("[%s] Not enough snapshots (%d) for trend analysis\n",
                name, history.snapshotCount);
            return true;
        }

        bool allOk = true;

        printf("\n=== [%s] Economy Snapshot Report over %u ticks ===\n", name, finalTick);
        printf("  Checkpoints: ");
        for (int i = 0; i < history.snapshotCount; ++i) {
            printf("%u ", history.snapshots[i].tick);
        }
        printf("(%d snapshots)\n", history.snapshotCount);

        // ---- Resource trends ----
        printf("\n  Resource trends:\n");

        const EconomySnapshot& first = history.snapshots[0];
        const EconomySnapshot& last  = history.snapshots[history.snapshotCount - 1];

        for (int r = 0; r < last.resourceCount; ++r) {
            const ResourceMetrics& rm = last.resources[r];
            // Find matching first-snapshot resource
            int firstIdx = -1;
            for (int f = 0; f < first.resourceCount; ++f) {
                if (first.resources[f].type == rm.type) {
                    firstIdx = f;
                    break;
                }
            }

            int produced = rm.totalProduced;
            int flow = rm.flow;
            int stock = rm.stockpile;
            int firstFlow = (firstIdx >= 0) ? first.resources[firstIdx].flow : 0;
            int firstProduced = (firstIdx >= 0) ? first.resources[firstIdx].totalProduced : 0;
            int producedDelta = produced - firstProduced;

            const char* trend = "stable";
            if (flow == 0 && firstFlow > 0) {
                trend = "STOPPED";
                allOk = false;
            } else if (flow < firstFlow / 2 && firstFlow > 5) {
                trend = "declining";
            } else if (flow > firstFlow * 2 && firstFlow > 0) {
                trend = "growing";
            }

            printf("    %-12s produced=%+6d  flow=%3d (was %3d)  stock=%4d  [%s]\n",
                ResourceTypeToString(rm.type),
                producedDelta, flow, firstFlow, stock, trend);
        }

        // ---- Building trends ----
        printf("\n  Building counts:\n");
        for (int b = 0; b < last.buildingCount; ++b) {
            const BuildingMetrics& bm = last.buildings[b];
            printf("    PT_%d: count=%d active=%d\n",
                (int)bm.pt, bm.count, bm.activeCount);
        }

        // ---- Ecology final ----
        printf("\n  Ecology:\n");
        ReportEcologyMetrics(last.ecology, finalTick);

        // ---- Supply crisis detection ----
        printf("\n  Supply crisis check:\n");
        bool crisis = false;
        for (int s = 1; s < history.snapshotCount; ++s) {
            const EconomySnapshot& prev = history.snapshots[s - 1];
            const EconomySnapshot& curr = history.snapshots[s];

            for (int r = 0; r < curr.resourceCount; ++r) {
                const ResourceMetrics& cr = curr.resources[r];
                // Find matching prev resource
                int prevIdx = -1;
                for (int p = 0; p < prev.resourceCount; ++p) {
                    if (prev.resources[p].type == cr.type) {
                        prevIdx = p;
                        break;
                    }
                }
                if (prevIdx < 0) continue;
                const ResourceMetrics& pr = prev.resources[prevIdx];

                // Crisis: flow was positive, now zero, and it's essential
                if (pr.flow > 0 && cr.flow == 0 && IsEssentialResource(cr.type)) {
                    printf("    [CRISIS] %s stopped flowing between tick %u and %u\n",
                        ResourceTypeToString(cr.type), prev.tick, curr.tick);
                    crisis = true;
                    allOk = false;
                }

                // Crisis: stockpile dropped to zero for essential resource
                if (pr.stockpile > 0 && cr.stockpile == 0 && IsEssentialResource(cr.type) && cr.totalProduced > 0) {
                    printf("    [CRISIS] %s stockpile depleted at tick %u\n",
                        ResourceTypeToString(cr.type), curr.tick);
                    crisis = true;
                    allOk = false;
                }
            }
        }
        if (!crisis) {
            printf("    No supply crises detected [PASS]\n");
        }

        // ---- Workers & demand ----
        printf("\n  Workers & demand:\n");
        printf("    Idle workers: %d  Pending requests: %d\n",
            last.idleWorkers, last.pendingRequests);

        if (last.pendingRequests > 200) {
            printf("    [WARN] High demand backlog (%d)\n", last.pendingRequests);
        }

        if (allOk) {
            printf("\n[PASS][%s] Economy stable across %u ticks, %d snapshots\n",
                name, finalTick, history.snapshotCount);
        } else {
            printf("\n[FAIL][%s] Economy issues detected\n", name);
        }

        return allOk;
    }

    // ──────────────────────────────────────────────
    // EconomyFlowTracker implementation
    // ──────────────────────────────────────────────

    EconomyFlowTracker::EconomyFlowTracker()
        : m_prevCount(0)
    {
        for (int i = 0; i < kMaxResources; ++i) {
            FlowAccumulator& a = m_accum[i];
            a.sum = 0; a.min = 0; a.max = 0; a.count = 0; a.sumSq = 0;
            m_prevMeans[i] = 0.0f;
            m_oscillationStreak[i] = 0;
        }
    }

    void EconomyFlowTracker::AccumulateTick(const EconomySystem* eco)
    {
        if (eco == NULL) return;

        for (int r = 1; r < kMaxResources; ++r) {
            int flow = eco->GetResourceFlow(static_cast<ResourceType>(r));
            FlowAccumulator& a = m_accum[r];
            if (a.count == 0) {
                a.min = flow;
                a.max = flow;
            } else {
                if (flow < a.min) a.min = flow;
                if (flow > a.max) a.max = flow;
            }
            a.sum += flow;
            a.sumSq += flow * flow;
            a.count++;
        }
    }

    int EconomyFlowTracker::FlushWindow(WindowFlowStats* outStats)
    {
        int written = 0;

        for (int r = 1; r < kMaxResources; ++r) {
            FlowAccumulator& a = m_accum[r];
            WindowFlowStats& s = outStats[r];

            s.sampleCount = a.count;

            if (a.count == 0) {
                s.mean = 0.0f;
                s.stddev = 0.0f;
                s.cv = 0.0f;
                s.min = 0;
                s.max = 0;
                s.trend = 0.0f;
                written++;
                continue;
            }

            float n = (float)a.count;
            float mean = (float)a.sum / n;
            // population stddev = sqrt( sumSq/n - mean^2 )
            float variance = (float)a.sumSq / n - mean * mean;
            if (variance < 0.0f) variance = 0.0f;
            float stddev = (float)sqrt(variance);

            s.mean = mean;
            s.stddev = stddev;
            s.cv = (mean > 0.001f) ? (stddev / mean * 100.0f) : 0.0f;
            s.min = a.min;
            s.max = a.max;

            // Trend vs previous window
            if (m_prevCount > r && m_prevMeans[r] > 0.001f) {
                s.trend = (mean - m_prevMeans[r]) / m_prevMeans[r] * 100.0f;
            } else {
                s.trend = 0.0f;
            }

            // Store current mean for next trend computation
            m_prevMeans[r] = mean;

            // Update oscillation streak
            StabilityClass sc = ClassifyStability(s);
            if (sc == SC_Oscillating || sc == SC_Declining) {
                m_oscillationStreak[r]++;
            } else {
                m_oscillationStreak[r] = 0;
            }

            written++;
        }
        m_prevCount = kMaxResources;

        // Reset accumulators for next window
        for (int i = 0; i < kMaxResources; ++i) {
            FlowAccumulator& a = m_accum[i];
            a.sum = 0; a.min = 0; a.max = 0; a.count = 0; a.sumSq = 0;
        }

        return written;
    }

    StabilityClass EconomyFlowTracker::ClassifyStability(const WindowFlowStats& stats)
    {
        if (stats.sampleCount < 2) return SC_Unknown;

        // Oscillation dominates — check first
        if (stats.cv >= 30.0f) return SC_Oscillating;

        // Then check trend
        if (stats.trend > 10.0f) return SC_Growing;
        if (stats.trend < -10.0f) return SC_Declining;

        // Default
        if (stats.cv < 15.0f) return SC_Stable;

        // Between 15% and 30% CV with no strong trend → stable enough
        return SC_Stable;
    }

    static const char* StabilityLabel(StabilityClass sc)
    {
        switch (sc) {
            case SC_Stable:     return "Stable";
            case SC_Oscillating: return "Oscillating";
            case SC_Growing:    return "Growing";
            case SC_Declining:  return "Declining";
            default:            return "—";
        }
    }

    static const char* ResourceStabilityTrendLabel(const WindowFlowStats& stats)
    {
        if (stats.trend > 5.0f) return "↑";
        if (stats.trend < -5.0f) return "↓";
        return "→";
    }

    void EconomyFlowTracker::GetOscillationStreaks(int* outStreaks) const
    {
        for (int i = 0; i < kMaxResources; ++i) {
            outStreaks[i] = m_oscillationStreak[i];
        }
    }

    void PrintStabilityReport(
        const WindowFlowStats* stats,
        int resourceCount,
        const char* name,
        const int* oscillationStreaks)
    {
        if (resourceCount <= 0) return;

        bool showStreak = (oscillationStreaks != NULL);

        printf("\n=== [%s] Economy Stability Report ===\n", name);
        if (showStreak) {
            printf("  %-14s %6s %6s %5s %6s %6s %5s  %s\n",
                "Resource", "Mean", "StdDev", "CV%", "Min", "Max", "Streak", "Status");
            printf("  %s\n",
                "──────────────────────────────────────────────────────────────────");
        } else {
            printf("  %-14s %6s %6s %5s %6s %6s  %s\n",
                "Resource", "Mean", "StdDev", "CV%", "Min", "Max", "Status");
            printf("  %s\n",
                "──────────────────────────────────────────────────────────────");
        }

        for (int r = 1; r < resourceCount; ++r) {
            const WindowFlowStats& s = stats[r];
            if (s.sampleCount == 0) continue;

            StabilityClass sc = EconomyFlowTracker::ClassifyStability(s);
            const char* ts = ResourceStabilityTrendLabel(s);

            if (showStreak) {
                printf("  %-14s %6.1f %6.1f %5.1f %6d %6d %5d  %s %s\n",
                    ResourceTypeToString(static_cast<ResourceType>(r)),
                    s.mean, s.stddev, s.cv, s.min, s.max,
                    oscillationStreaks[r],
                    ts, StabilityLabel(sc));
            } else {
                printf("  %-14s %6.1f %6.1f %5.1f %6d %6d  %s %s\n",
                    ResourceTypeToString(static_cast<ResourceType>(r)),
                    s.mean, s.stddev, s.cv, s.min, s.max,
                    ts, StabilityLabel(sc));
            }
        }
        printf("\n");
    }

    // ──────────────────────────────────────────────
    // Stability Propagation Analysis
    // ──────────────────────────────────────────────

    void PrintStabilityPropagation(
        const WindowFlowStats* stats,
        int resourceCount,
        const char* name,
        const int* oscillationStreaks)
    {
        if (resourceCount <= 0) return;

        printf("\n=== [%s] Stability Propagation ===\n", name);

        // ---- Edges: for each production type with inputs + outputs ----
        printf("\n  %-42s %-14s %-12s\n",
            "Chain", "Status", "Pattern");
        printf("  %s\n",
            "──────────────────────────────────────────────────────────────────────────");

        // Track affected relationships for root-cause analysis
        static const int kMaxEdges = 64;
        int edgeCount = 0;
        struct {
            ResourceType from;
            ResourceType to;
            ProductionType pt;
        } edges[kMaxEdges];

        // Track which unstable resources have at least one unstable upstream
        bool hasUnstableUpstream[ResourceType_Count];
        bool isRootCause[ResourceType_Count];
        for (int i = 0; i < ResourceType_Count; ++i) {
            hasUnstableUpstream[i] = false;
            isRootCause[i] = false;
        }

        for (int pt = 1; pt < static_cast<int>(PT_Count); ++pt) {
            const ProductionDefinition& def = GetProductionDefinition(static_cast<ProductionType>(pt));

            // Collect outputs with stats
            ResourceType outputs[4];
            int outCount = 0;
            for (int o = 0; o < 4; ++o) {
                ResourceType r = def.produces[o].resource;
                if (r == ResourceType_None) break;
                if (static_cast<int>(r) < resourceCount && stats[r].sampleCount > 0) {
                    outputs[outCount++] = r;
                }
            }
            if (outCount == 0) continue;

            // Collect inputs with stats
            ResourceType inputs[4];
            int inCount = 0;
            for (int i = 0; i < 4; ++i) {
                ResourceType r = def.consumes[i].resource;
                if (r == ResourceType_None) break;
                if (static_cast<int>(r) < resourceCount && stats[r].sampleCount > 0) {
                    inputs[inCount++] = r;
                }
            }
            if (inCount == 0) continue;

            // Print each input → output edge
            for (int oi = 0; oi < outCount; ++oi) {
                ResourceType out = outputs[oi];
                StabilityClass outSC = EconomyFlowTracker::ClassifyStability(stats[out]);
                const char* outLabel = StabilityLabel(outSC);

                for (int ii = 0; ii < inCount; ++ii) {
                    ResourceType in = inputs[ii];
                    StabilityClass inSC = EconomyFlowTracker::ClassifyStability(stats[in]);
                    const char* inLabel = StabilityLabel(inSC);
                    float inCV = stats[in].cv;

                    // Determine propagation pattern
                    const char* pattern = "";
                    bool unstableUp = (inSC == SC_Oscillating || inSC == SC_Declining);
                    bool unstableDown = (outSC == SC_Oscillating || outSC == SC_Declining);

                    if (unstableUp && unstableDown) {
                        pattern = "↑ downstream";
                        hasUnstableUpstream[out] = true;
                    } else if (!unstableUp && unstableDown) {
                        pattern = "local";
                    } else if (unstableUp && !unstableDown) {
                        pattern = "absorbed";
                    }

                    char chain[64];
                    sprintf(chain, "%s (%s) → %s",
                        ResourceTypeToString(in), inLabel,
                        ResourceTypeToString(out));

                    printf("  %-42s %-14s %-12s\n",
                        chain, outLabel, pattern);

                    // Record edge for root-cause analysis
                    if (edgeCount < kMaxEdges) {
                        edges[edgeCount].from = in;
                        edges[edgeCount].to = out;
                        edges[edgeCount].pt = static_cast<ProductionType>(pt);
                        edgeCount++;
                    }
                }
            }
        }

        // ---- Root-cause candidates ----
        printf("\n  Root-cause analysis:\n");

        // Determine which resources are "affected" by upstream instability
        // (has at least one unstable upstream)
        bool foundRoot = false;

        // Check each resource that is unstable
        for (int r = 1; r < resourceCount; ++r) {
            if (stats[r].sampleCount == 0) continue;
            ResourceType rt = static_cast<ResourceType>(r);
            StabilityClass sc = EconomyFlowTracker::ClassifyStability(stats[r]);

            bool unstable = (sc == SC_Oscillating || sc == SC_Declining);
            if (!unstable) continue;

            // Does this resource have any upstream (inputs) at all?
            // Find what produces it, and whether that production type has inputs
            ProductionType producer = GetProducer(rt);
            bool hasUpstream = false;
            bool upstreamAllStable = true;
            if (producer != PT_None) {
                const ProductionDefinition& def = GetProductionDefinition(producer);
                for (int i = 0; i < 4; ++i) {
                    ResourceType inRes = def.consumes[i].resource;
                    if (inRes == ResourceType_None) break;
                    if (static_cast<int>(inRes) < resourceCount && stats[inRes].sampleCount > 0) {
                        hasUpstream = true;
                        StabilityClass inSC = EconomyFlowTracker::ClassifyStability(stats[inRes]);
                        if (inSC == SC_Oscillating || inSC == SC_Declining) {
                            upstreamAllStable = false;
                        }
                    }
                }
            }

            if (hasUnstableUpstream[rt]) {
                // This resource has at least one unstable upstream
                // Find the most likely upstream contributor
                printf("    %-14s %s — propagated (upstream instability detected)\n",
                    ResourceTypeToString(rt), StabilityLabel(sc));
                foundRoot = true;
            } else if (hasUpstream && upstreamAllStable) {
                // All inputs stable but this output is unstable → local issue
                printf("    %-14s %s — local (all upstream inputs stable)\n",
                    ResourceTypeToString(rt), StabilityLabel(sc));
                foundRoot = true;
            } else if (!hasUpstream) {
                // No upstream inputs at all (renewable/raw) but unstable
                printf("    %-14s %s — source (raw/renewable supply)\n",
                    ResourceTypeToString(rt), StabilityLabel(sc));
                foundRoot = true;
            }
        }

        if (!foundRoot) {
            printf("    No stability propagation detected\n");
        }

        // ---- List all upstream contributors ----
        printf("\n  Upstream contributors:\n");
        bool foundContributor = false;
        for (int r = 1; r < resourceCount; ++r) {
            if (stats[r].sampleCount == 0) continue;
            ResourceType rt = static_cast<ResourceType>(r);
            StabilityClass sc = EconomyFlowTracker::ClassifyStability(stats[r]);
            if (sc != SC_Oscillating && sc != SC_Declining) continue;

            // Find which downstream resources it affects
            bool affectsAny = false;
            for (int e = 0; e < edgeCount; ++e) {
                if (edges[e].from == rt) {
                    ResourceType to = edges[e].to;
                    StabilityClass toSC = EconomyFlowTracker::ClassifyStability(stats[to]);
                    if (toSC == SC_Oscillating || toSC == SC_Declining) {
                        affectsAny = true;
                        break;
                    }
                }
            }

            if (affectsAny) {
                printf("    %-14s (CV=%.0f%%, %s) — affects downstream consumers\n",
                    ResourceTypeToString(rt), stats[r].cv, StabilityLabel(sc));
                foundContributor = true;
            }
        }
        if (!foundContributor) {
            printf("    No upstream contributors detected\n");
        }

        // ---- Cascade depth ----
        printf("\n  Cascade analysis:\n");

        // Build adjacency: for each resource, which downstream resources depend on it?
        // Only count edges where both endpoints have stats data
        int downstream[ResourceType_Count][8];
        int downCount[ResourceType_Count];
        for (int i = 0; i < ResourceType_Count; ++i) downCount[i] = 0;

        for (int e = 0; e < edgeCount; ++e) {
            ResourceType from = edges[e].from;
            ResourceType to = edges[e].to;
            if (downCount[from] < 8) {
                downstream[from][downCount[from]++] = to;
            }
        }

        // DFS from each unstable root to find longest cascade
        int maxDepth = 0;
        ResourceType cascadeRoot = ResourceType_None;

        for (int r = 1; r < resourceCount; ++r) {
            if (stats[r].sampleCount == 0) continue;
            ResourceType rt = static_cast<ResourceType>(r);
            StabilityClass sc = EconomyFlowTracker::ClassifyStability(stats[r]);
            if (sc != SC_Oscillating && sc != SC_Declining) continue;
            // Only start from roots (no unstable upstream)
            if (hasUnstableUpstream[rt]) continue;

            // DFS from this root
            struct { ResourceType node; int depth; } stack[16];
            int stackSize = 0;
            stack[stackSize].node = rt;
            stack[stackSize].depth = 1;
            stackSize++;

            while (stackSize > 0) {
                stackSize--;
                ResourceType node = stack[stackSize].node;
                int depth = stack[stackSize].depth;

                bool hasUnstableChild = false;
                for (int d = 0; d < downCount[node]; ++d) {
                    ResourceType child = static_cast<ResourceType>(downstream[node][d]);
                    if (child >= resourceCount) continue;
                    if (stats[child].sampleCount == 0) continue;
                    StabilityClass childSC = EconomyFlowTracker::ClassifyStability(stats[child]);
                    if (childSC == SC_Oscillating || childSC == SC_Declining) {
                        hasUnstableChild = true;
                        if (stackSize < 16) {
                            stack[stackSize].node = child;
                            stack[stackSize].depth = depth + 1;
                            stackSize++;
                        }
                    }
                }

                if (!hasUnstableChild && depth > maxDepth) {
                    maxDepth = depth;
                    cascadeRoot = rt;
                }
            }
        }

        if (maxDepth > 0) {
            printf("    Longest cascade: %d nodes deep", maxDepth);
            if (cascadeRoot != ResourceType_None) {
                printf(" (root: %s)", ResourceTypeToString(cascadeRoot));
            }
            printf("\n");

            // Print the cascade chain
            // Follow edges from root to find the actual chain
            int chain[16];
            int chainLen = 0;
            chain[chainLen++] = cascadeRoot;
            ResourceType current = cascadeRoot;
            bool progressed = true;
            while (progressed && chainLen < 16) {
                progressed = false;
                for (int d = 0; d < downCount[current]; ++d) {
                    ResourceType child = static_cast<ResourceType>(downstream[current][d]);
                    if (child >= resourceCount) continue;
                    if (stats[child].sampleCount == 0) continue;
                    StabilityClass childSC = EconomyFlowTracker::ClassifyStability(stats[child]);
                    if (childSC == SC_Oscillating || childSC == SC_Declining) {
                        chain[chainLen++] = child;
                        current = child;
                        progressed = true;
                        break;
                    }
                }
            }

            if (chainLen > 1) {
                printf("    Chain: ");
                for (int c = 0; c < chainLen; ++c) {
                    if (c > 0) printf(" → ");
                    printf("%s", ResourceTypeToString(static_cast<ResourceType>(chain[c])));
                    if (oscillationStreaks != NULL && oscillationStreaks[chain[c]] > 1) {
                        printf("[%dw]", oscillationStreaks[chain[c]]);
                    }
                }
                printf("\n");
            }
        } else {
            printf("    No cascading instability detected\n");
        }

        printf("\n");
    }

}
