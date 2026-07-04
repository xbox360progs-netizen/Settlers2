#include "../../Testing/ISimulationScenario.h"
#include "../../Definitions/ProductionDefinition.h"
#include "../../Definitions/BuildingDefinition.h"
#include "../../Core/ResourceTypes.h"
#include <stdio.h>

namespace World {

    // Economic Independence Test
    //
    // Verifies that removing any single industry degrades only its transitive
    // dependencies — the remaining graph stays complete and cycle-free.
    //
    // Industry groups are defined by their ProductionType members.
    // For each group, the test:
    //   1. Excludes the group's production definitions
    //   2. Verifies remaining resources outside the removed component stay reachable
    //   3. Verifies no new orphan cycles appear
    //   4. Reports the exact set of resources that became unreachable (expected loss)

    struct IndustryGroup {
        const char* name;
        ProductionType members[8];
    };

    static bool IsRenewable(ResourceType r)
    {
        switch (r) {
            case ResourceType_Wood:
            case ResourceType_Fish:
            case ResourceType_Meat:
            case ResourceType_Wheat:
            case ResourceType_Water:
                return true;
            default:
                return false;
        }
    }

    static bool IsNoneOrZero(const ResourceAmount& ra)
    {
        return ra.resource == ResourceType_None || ra.amount <= 0;
    }

    // Check if a ProductionType is excluded
    static bool IsExcluded(ProductionType pt, const IndustryGroup& group)
    {
        for (int i = 0; i < 8; ++i) {
            if (group.members[i] == PT_None) break;
            if (group.members[i] == pt) return true;
        }
        return false;
    }

    // Custom GetProducer that skips excluded production types
    static ProductionType GetProducerExcluding(ResourceType resource, const IndustryGroup& group)
    {
        for (int t = 1; t < PT_Count; ++t) {
            ProductionType pt = static_cast<ProductionType>(t);
            if (IsExcluded(pt, group)) continue;
            const ProductionDefinition& def = GetProductionDefinition(pt);
            for (int p = 0; p < 4; ++p) {
                if (def.produces[p].resource == resource && def.produces[p].amount > 0) {
                    return pt;
                }
            }
        }
        return PT_None;
    }

    // Custom reachability check with excluded production types
    static bool IsReachableExcluding(ResourceType resource, bool visited[], int depth,
        const IndustryGroup& group)
    {
        if (depth > 32) return false;
        if (IsRenewable(resource)) return true;

        int idx = static_cast<int>(resource);
        if (idx < 0 || idx >= ResourceType_Count) return false;
        if (visited[idx]) return false;
        visited[idx] = true;

        ProductionType producer = GetProducerExcluding(resource, group);
        if (producer == PT_None) return false;

        const ProductionDefinition& def = GetProductionDefinition(producer);

        for (int c = 0; c < 4; ++c) {
            const ResourceAmount& input = def.consumes[c];
            if (IsNoneOrZero(input)) continue;

            bool branchVisited[ResourceType_Count];
            for (int i = 0; i < ResourceType_Count; ++i)
                branchVisited[i] = visited[i];

            if (!IsReachableExcluding(input.resource, branchVisited, depth + 1, group))
                return false;
        }

        return true;
    }

    // Count how many producers a resource has (excluding a group)
    static int CountProducersExcluding(ResourceType resource, const IndustryGroup& group)
    {
        int count = 0;
        for (int t = 1; t < PT_Count; ++t) {
            ProductionType pt = static_cast<ProductionType>(t);
            if (IsExcluded(pt, group)) continue;
            const ProductionDefinition& def = GetProductionDefinition(pt);
            for (int p = 0; p < 4; ++p) {
                if (def.produces[p].resource == resource && def.produces[p].amount > 0) {
                    ++count;
                }
            }
        }
        return count;
    }

    // Industry group definitions
    static const IndustryGroup kIndustries[] = {
        { "Forestry",   { PT_Woodcutter, PT_Forester, PT_Sawmill, PT_None } },
        { "Quarrying",  { PT_Stonemason, PT_None } },
        { "Hunting",    { PT_Hunter, PT_None } },
        { "Fishing",    { PT_Fisher, PT_None } },
        { "Tools",      { PT_Toolmaker, PT_None } },
        { "Mining",     { PT_CoalMine, PT_IronMine, PT_GoldMine, PT_None } },
        { "Agriculture",{ PT_Farm, PT_Mill, PT_Bakery, PT_None } },
        { "Metallurgy", { PT_IronSmelter, PT_WeaponSmith, PT_None } },
    };
    static const int kIndustryCount = sizeof(kIndustries) / sizeof(kIndustries[0]);

    class T50EconomicIndependenceTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T50"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = false;
            config.enableEconomy = false;
        }

        void Initialize(Simulation&) {}

        bool Tick(Simulation&)
        {
            printf("\n=== T50: Economic Independence Test ===\n\n");

            bool allOk = true;

            for (int ind = 0; ind < kIndustryCount; ++ind) {
                const IndustryGroup& group = kIndustries[ind];
                printf("--- Industry: %s ---\n", group.name);

                bool groupOk = true;

                // Track which resources lost all producers
                int lostProduction[ResourceType_Count] = { 0 };

                for (int r = 1; r < ResourceType_Count; ++r) {
                    ResourceType res = static_cast<ResourceType>(r);
                    if (IsRenewable(res)) continue;

                    int before = CountProducersExcluding(res, group);
                    (void)before;
                    // The resource may have no producer at all (never produced).
                    // That's fine — skip.
                    ProductionType origProducer = GetProducer(res);
                    if (origProducer == PT_None) continue;

                    ProductionType remainingProducer = GetProducerExcluding(res, group);
                    if (remainingProducer == PT_None) {
                        lostProduction[r] = 1;
                    }
                }

                // Check reachability for resources that still have a producer
                int reachableCount = 0;
                int unreachableCount = 0;
                bool cycleDetected = false;

                for (int r = 1; r < ResourceType_Count; ++r) {
                    ResourceType res = static_cast<ResourceType>(r);
                    if (IsRenewable(res)) continue;
                    if (lostProduction[r]) continue; // expected loss

                    ProductionType remainingProducer = GetProducerExcluding(res, group);
                    if (remainingProducer == PT_None) continue; // not produced

                    bool visited[ResourceType_Count] = { false };
                    if (IsReachableExcluding(res, visited, 0, group)) {
                        ++reachableCount;
                    } else {
                        // Unreachable despite having a producer — check if it's a cycle
                        bool allCyclic = true;
                        for (int v = 1; v < ResourceType_Count; ++v) {
                            if (visited[v]) {
                                if (IsRenewable(static_cast<ResourceType>(v))) {
                                    allCyclic = false;
                                    break;
                                }
                            }
                        }
                        if (allCyclic) {
                            printf("  [FAIL] Resource %d is in orphan cycle after removing %s\n",
                                r, group.name);
                            cycleDetected = true;
                            groupOk = false;
                        } else {
                            // Still has a producer but unreachable — unexpected degradation
                            printf("  [FAIL] Resource %d has producer PT_%d but is unreachable after removing %s\n",
                                r, remainingProducer, group.name);
                            groupOk = false;
                        }
                        ++unreachableCount;
                    }
                }

                // Report expected losses
                printf("  Resources with lost producers: ");
                int lostCount = 0;
                for (int r = 1; r < ResourceType_Count; ++r) {
                    if (lostProduction[r]) {
                        if (lostCount > 0) printf(", ");
                        printf("%d", r);
                        ++lostCount;
                    }
                }
                printf(" (%d resources)\n", lostCount);

                if (reachableCount > 0) {
                    printf("  Resources still reachable: %d\n", reachableCount);
                }

                if (groupOk) {
                    printf("  [PASS] %s removal — only transitive dependencies affected\n", group.name);
                } else {
                    printf("  [FAIL] %s removal — unexpected graph degradation\n", group.name);
                    allOk = false;
                }
                printf("\n");
            }

            if (allOk) {
                printf("[PASS] T50: All industries are economically independent\n");
            } else {
                printf("[FAIL] T50: Some industries cause unexpected degradation\n");
            }

            return false;
        }
    };

    T50EconomicIndependenceTest g_t50;

} // namespace World
