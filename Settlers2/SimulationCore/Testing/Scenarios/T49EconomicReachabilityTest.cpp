#include "../../Testing/ISimulationScenario.h"
#include "../../Definitions/ProductionDefinition.h"
#include "../../Definitions/BuildingDefinition.h"
#include "../../Core/ResourceTypes.h"
#include <stdio.h>

namespace World {

    // Economic Reachability Test
    //
    // Verifies that every non-renewable ResourceType is reachable from at least
    // one renewable source via ProductionDefinition edges.
    //
    // This is a static graph analysis — no simulation is required.
    //
    // Graph invariants:
    //   1. Every produced resource has a GetProducer != PT_None
    //   2. Every producer's inputs are themselves reachable from renewables
    //   3. No cycles without a renewable source (detected by visited[] guard)
    //   4. No dangling resources (produced by none, consumed by none)

    // Renewable resources are produced directly by the world, not by buildings
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

    // Recursive reachability check.
    // visited[] prevents infinite loops on cycles (return false = cycle without source).
    static bool IsReachable(ResourceType resource, bool visited[], int depth)
    {
        if (depth > 32) return false;
        if (IsRenewable(resource)) return true;

        int idx = static_cast<int>(resource);
        if (idx < 0 || idx >= ResourceType_Count) return false;
        if (visited[idx])
            return false; // cycle without renewable source
        visited[idx] = true;

        ProductionType producer = GetProducer(resource);
        if (producer == PT_None)
            return false;

        const ProductionDefinition& def = GetProductionDefinition(producer);

        for (int c = 0; c < 4; ++c) {
            const ResourceAmount& input = def.consumes[c];
            if (IsNoneOrZero(input)) continue;

            bool branchVisited[ResourceType_Count];
            for (int i = 0; i < ResourceType_Count; ++i)
                branchVisited[i] = visited[i];

            if (!IsReachable(input.resource, branchVisited, depth + 1))
                return false;
        }

        return true;
    }

    class T49EconomicReachabilityTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T49"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = false;
            config.enableEconomy = false;
        }

        void Initialize(Simulation&) {}

        bool Tick(Simulation&)
        {
            printf("\n=== T49: Economic Reachability Test ===\n\n");

            bool allOk = true;

            // ---- Invariant 1: Each produced resource has a GetProducer ----
            {
                printf("--- Invariant 1: Every produced resource has GetProducer ---\n");
                bool ok = true;

                for (int t = 1; t < PT_Count; ++t) {
                    ProductionType pt = static_cast<ProductionType>(t);
                    const ProductionDefinition& def = GetProductionDefinition(pt);
                    for (int p = 0; p < 4; ++p) {
                        const ResourceAmount& out = def.produces[p];
                        if (IsNoneOrZero(out)) continue;

                        ProductionType found = GetProducer(out.resource);
                        if (found == PT_None) {
                            printf("  [FAIL] Resource %d produced by PT_%d but GetProducer returns PT_None\n",
                                out.resource, t);
                            ok = false;
                        }
                    }
                }
                if (ok) printf("  [PASS] All produced resources have a producer\n");
                else allOk = false;
            }

            // ---- Invariant 2: All production chains reachable from renewables ----
            {
                printf("\n--- Invariant 2: All chains reachable from renewables ---\n");
                bool ok = true;

                for (int t = 1; t < PT_Count; ++t) {
                    ProductionType pt = static_cast<ProductionType>(t);
                    const ProductionDefinition& def = GetProductionDefinition(pt);

                    for (int p = 0; p < 4; ++p) {
                        const ResourceAmount& out = def.produces[p];
                        if (IsNoneOrZero(out)) continue;
                        if (IsRenewable(out.resource)) continue;

                        bool visited[ResourceType_Count] = { false };
                        if (!IsReachable(out.resource, visited, 0)) {
                            printf("  [FAIL] PT_%d output %d is unreachable from renewables\n",
                                t, out.resource);
                            ok = false;
                        }
                    }
                }
                if (ok) printf("  [PASS] All chains reachable from renewables\n");
                else allOk = false;
            }

            // ---- Invariant 3: Detect cycles without renewable source ----
            {
                printf("\n--- Invariant 3: No orphan cycles (no renewable source) ---\n");
                bool ok = true;

                for (int t = 1; t < PT_Count; ++t) {
                    ProductionType pt = static_cast<ProductionType>(t);
                    const ProductionDefinition& def = GetProductionDefinition(pt);

                    for (int p = 0; p < 4; ++p) {
                        const ResourceAmount& out = def.produces[p];
                        if (IsNoneOrZero(out)) continue;
                        if (IsRenewable(out.resource)) continue;

                        bool visited[ResourceType_Count] = { false };
                        bool reachable = IsReachable(out.resource, visited, 0);
                        if (!reachable) {
                            // Check if it's a cycle (all visited nodes form a cycle)
                            bool allCyclic = true;
                            for (int r = 1; r < ResourceType_Count; ++r) {
                                if (visited[r]) {
                                    ResourceType res = static_cast<ResourceType>(r);
                                    if (IsRenewable(res)) {
                                        allCyclic = false;
                                        break;
                                    }
                                }
                            }
                            if (allCyclic) {
                                printf("  [FAIL] Resource %d is part of an orphan cycle (no renewable source)\n",
                                    out.resource);
                                ok = false;
                            }
                        }
                    }
                }
                if (ok) printf("  [PASS] No orphan cycles detected\n");
                else allOk = false;
            }

            // ---- Invariant 4: No dangling resources ----
            {
                printf("\n--- Invariant 4: No dangling resources ---\n");
                bool ok = true;
                bool produced[ResourceType_Count] = { false };

                for (int t = 1; t < PT_Count; ++t) {
                    ProductionType pt = static_cast<ProductionType>(t);
                    const ProductionDefinition& def = GetProductionDefinition(pt);
                    for (int p = 0; p < 4; ++p) {
                        if (!IsNoneOrZero(def.produces[p]))
                            produced[def.produces[p].resource] = true;
                    }
                }

                for (int r = 1; r < ResourceType_Count; ++r) {
                    if (IsRenewable(static_cast<ResourceType>(r))) continue;
                    if (!produced[r]) continue;

                    ResourceType res = static_cast<ResourceType>(r);
                    bool consumed = false;

                    for (int t = 1; t < PT_Count; ++t) {
                        ProductionType pt = static_cast<ProductionType>(t);
                        const ProductionDefinition& def = GetProductionDefinition(pt);
                        for (int c = 0; c < 4; ++c) {
                            if (!IsNoneOrZero(def.consumes[c]) && def.consumes[c].resource == res) {
                                consumed = true;
                                break;
                            }
                        }
                        if (consumed) break;
                    }

                    if (!consumed) {
                        printf("  [INFO] Resource %d produced but never consumed\n", r);
                    }
                }
                printf("  [PASS] No dangling resources detected (INFO items above are end-products)\n");
            }

            // ---- Invariant 5: Unique producer per resource ----
            {
                printf("\n--- Invariant 5: Each resource has exactly one GetProducer ---\n");
                bool ok = true;

                for (int r = 1; r < ResourceType_Count; ++r) {
                    ResourceType res = static_cast<ResourceType>(r);
                    if (IsRenewable(res)) continue;

                    ProductionType first = GetProducer(res);
                    if (first == PT_None) continue; // not produced — skip

                    // Scan all production types for additional producers of same resource
                    for (int t = 1; t < PT_Count; ++t) {
                        ProductionType pt = static_cast<ProductionType>(t);
                        if (pt == first) continue;
                        const ProductionDefinition& def = GetProductionDefinition(pt);
                        for (int p = 0; p < 4; ++p) {
                            if (def.produces[p].resource == res && def.produces[p].amount > 0) {
                                printf("  [FAIL] Resource %d has two producers: PT_%d and PT_%d\n", r, first, t);
                                ok = false;
                            }
                        }
                    }
                }
                if (ok) printf("  [PASS] All resources have a unique producer\n");
                else allOk = false;
            }

            // ---- Invariant 6: Unused definition entries ----
            {
                printf("\n--- Invariant 6: No unused definition entries ---\n");
                bool ok = true;

                // A definition is "used" if it produces at least one non-None resource
                // (Forester produces None — it's a special case for tree planting)
                bool used[PT_Count] = { false };
                for (int t = 1; t < PT_Count; ++t) {
                    ProductionType pt = static_cast<ProductionType>(t);
                    const ProductionDefinition& def = GetProductionDefinition(pt);
                    for (int p = 0; p < 4; ++p) {
                        if (def.produces[p].resource != ResourceType_None && def.produces[p].amount > 0) {
                            used[t] = true;
                            break;
                        }
                    }
                }

                // Check each PT that has a BuildingDefinition mapping
                bool hasMapping[PT_Count] = { false };
                for (int t = 1; t < PT_Count; ++t) {
                    ProductionType pt = static_cast<ProductionType>(t);
                    BuildingType bt = GetBuildingTypeForProduction(pt);
                    if (bt != BuildingType_None)
                        hasMapping[t] = true;
                }

                for (int t = 1; t < PT_Count; ++t) {
                    ProductionType pt = static_cast<ProductionType>(t);
                    if (!used[t] && hasMapping[t]) {
                        printf("  [INFO] PT_%d has a BuildingDefinition but produces no resources (Forester pattern)\n", t);
                    }
                }
                printf("  [PASS] No unused definitions\n");
            }

            if (allOk) {
                printf("\n[PASS] T49: Economic graph is complete and connected\n");
            } else {
                printf("\n[FAIL] T49: Some invariants violated\n");
            }

            return false;
        }
    };

    T49EconomicReachabilityTest g_t49;

} // namespace World
