#include <stdio.h>
#include "../Simulation/Simulation.h"
#include "../World/WorldModel.h"
#include "../Construction/ConstructionSystem.h"
#include "SimulationAssertions.h"

namespace World {

    namespace {

        struct TrackedRequest {
            BuildingType type;
            int x;
            int y;
            bool seenAsSite;
        };

        static const int kMaxTrackedRequests = 32;

        static void AddConstructionRequest(Simulation& sim, BuildingType type, int x, int y,
            TrackedRequest* tracked, int& trackedCount)
        {
            WorldModel& world = sim.GetWorld();
            if (world.pendingConstructionCount >= kMaxConstructionRequests) return;
            ConstructionRequest& req = world.pendingConstructionRequests[world.pendingConstructionCount++];
            req.type = type;
            req.position = Vector2i(x, y);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
            printf("  Tick %u: Add %s at (%d,%d)\n",
                sim.GetState().tickCount, "building", x, y);

            if (tracked && trackedCount < kMaxTrackedRequests) {
                tracked[trackedCount].type = type;
                tracked[trackedCount].x = x;
                tracked[trackedCount].y = y;
                tracked[trackedCount].seenAsSite = false;
                trackedCount++;
            }
        }

        // Check that all manually-tracked construction requests produced active sites.
        static bool AllTrackedRequestsFulfilled(const WorldModel& world,
            const TrackedRequest* tracked, int trackedCount)
        {
            for (int t = 0; t < trackedCount; ++t) {
                bool found = false;
                for (int s = 0; s < world.activeSiteCount; ++s) {
                    const ConstructionSite& site = world.activeSites[s];
                    if (site.type == tracked[t].type
                        && site.position.x == tracked[t].x
                        && site.position.y == tracked[t].y)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    printf("[FAIL] Tracked request (%d,%d) not found as active site\n",
                        tracked[t].x, tracked[t].y);
                    return false;
                }
            }
            return true;
        }

    }

    // T1 — Single construction: request → site → resources → transport request
    static bool RunT1(Simulation& sim, WorldModel& world)
    {
        printf("--- T1: Single construction ---\n");
        TrackedRequest tracked[4];
        int trackedCount = 0;
        AddConstructionRequest(sim, BuildingType_Sawmill, 10, 10, tracked, trackedCount);

        for (int tick = 0; tick < 200; ++tick) {
            sim.Tick();
            if (!Assert::AllInvariants(sim.GetWorld())) {
                printf("[FAIL] T1 failed at tick %u\n", sim.GetState().tickCount);
                return false;
            }
        }

        if (!AllTrackedRequestsFulfilled(sim.GetWorld(), tracked, trackedCount)) return false;
        printf("[PASS] T1: Single construction pipeline established\n");
        return true;
    }

    // T2 — Two concurrent constructions
    static bool RunT2(Simulation& sim, WorldModel& world)
    {
        printf("--- T2: Two concurrent constructions ---\n");
        TrackedRequest tracked[4];
        int trackedCount = 0;
        AddConstructionRequest(sim, BuildingType_Sawmill, 10, 10, tracked, trackedCount);
        AddConstructionRequest(sim, BuildingType_Sawmill, 20, 10, tracked, trackedCount);

        for (int tick = 0; tick < 200; ++tick) {
            sim.Tick();
            if (!Assert::AllInvariants(sim.GetWorld())) return false;
        }

        if (!AllTrackedRequestsFulfilled(sim.GetWorld(), tracked, trackedCount)) return false;
        printf("[PASS] T2: Two concurrent sites\n");
        return true;
    }

    // T6 — Mass construction: 20 sites
    static bool RunT6(Simulation& sim, WorldModel& world)
    {
        printf("--- T6: Mass construction (20 sites) ---\n");
        TrackedRequest tracked[20];
        int trackedCount = 0;
        for (int i = 0; i < 20; ++i) {
            AddConstructionRequest(sim, BuildingType_Sawmill, 10 + i, 10, tracked, trackedCount);
        }

        for (int tick = 0; tick < 5000; ++tick) {
            sim.Tick();
            if ((tick % 1000) == 0) {
                if (!Assert::AllInvariants(sim.GetWorld())) return false;
            }
        }

        if (!AllTrackedRequestsFulfilled(sim.GetWorld(), tracked, trackedCount)) return false;
        printf("[PASS] T6: Mass construction stable\n");
        return true;
    }

    // T7 — Long soak test
    static bool RunT7(Simulation& sim, WorldModel& world)
    {
        printf("--- T7: Long soak test (100000 ticks) ---\n");
        TrackedRequest tracked[4];
        int trackedCount = 0;
        AddConstructionRequest(sim, BuildingType_Sawmill, 10, 10, tracked, trackedCount);
        AddConstructionRequest(sim, BuildingType_Stonemason, 20, 10, tracked, trackedCount);

        const uint32_t soakTicks = 100000;
        const uint32_t checkInterval = 10000;

        for (uint32_t tick = 0; tick < soakTicks; ++tick) {
            sim.Tick();
            if ((tick % checkInterval) == 0) {
                if (!Assert::AllInvariants(sim.GetWorld())) return false;
                printf("  ... tick %u / %u\n", tick, soakTicks);
            }
        }

        if (!AllTrackedRequestsFulfilled(sim.GetWorld(), tracked, trackedCount)) return false;
        printf("[PASS] T7: Soak test completed\n");
        return true;
    }

    struct ScenarioDef {
        const char* name;
        bool (*run)(Simulation&, WorldModel&);
        const char* description;
    };

    static const ScenarioDef kScenarios[] = {
        { "T1", RunT1, "Single construction pipeline" },
        { "T2", RunT2, "Two concurrent constructions" },
        { "T6", RunT6, "Mass construction (20 sites, 5000 ticks)" },
        { "T7", RunT7, "Long soak test (100000 ticks)" },
    };

    static const int kScenarioCount = sizeof(kScenarios) / sizeof(kScenarios[0]);

    void ListScenarios()
    {
        printf("Available scenarios:\n");
        for (int i = 0; i < kScenarioCount; ++i) {
            printf("  %s: %s\n", kScenarios[i].name, kScenarios[i].description);
        }
    }

    int FindScenario(const char* name)
    {
        for (int i = 0; i < kScenarioCount; ++i) {
            const char* a = kScenarios[i].name;
            const char* b = name;
            while (*a && *b && *a == *b) { ++a; ++b; }
            if (*a == 0 && *b == 0) return i;
        }
        return -1;
    }

    bool RunScenario(const char* name, Simulation& sim, WorldModel& world)
    {
        int idx = FindScenario(name);
        if (idx < 0) {
            printf("Unknown scenario: %s\n", name);
            ListScenarios();
            return false;
        }
        return kScenarios[idx].run(sim, world);
    }

} // namespace World
