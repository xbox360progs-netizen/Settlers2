#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/SimulationAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationState.h"
#include "../../World/WorldModel.h"
#include "../../Construction/ConstructionSystem.h"
#include "../../Worker/WorkerSystem.h"
#include <stdio.h>

namespace World {

    class T7LongSoak : public ISimulationScenario {
    public:
        const char* GetName() const { return "T7"; }

        void Initialize(Simulation& sim)
        {
            WorldModel& world = sim.GetWorld();
            if (world.pendingConstructionCount + 2 < kMaxConstructionRequests) {
                ConstructionRequest& req1 = world.pendingConstructionRequests[world.pendingConstructionCount++];
                req1.type = BuildingType_Sawmill;
                req1.position = Vector2i(10, 10);
                req1.owner = 0;
                req1.priority = 1;
                req1.fulfilled = false;

                ConstructionRequest& req2 = world.pendingConstructionRequests[world.pendingConstructionCount++];
                req2.type = BuildingType_Stonemason;
                req2.position = Vector2i(20, 10);
                req2.owner = 0;
                req2.priority = 1;
                req2.fulfilled = false;

                printf("  Tick %u: Add building at (10,10)\n", sim.GetState().tickCount);
                printf("  Tick %u: Add building at (20,10)\n", sim.GetState().tickCount);
            }
        }

        bool Tick(Simulation& sim)
        {
            uint32_t tick = sim.GetState().tickCount;
            static const uint32_t kCheckInterval = 10000;
            static const uint32_t kSoakTicks = 100000;

            if ((tick % kCheckInterval) == 0) {
                if (!Assert::AllInvariants(sim.GetWorld(), tick)) {
                    printf("[FAIL] T7 failed at tick %u\n", tick);
                    return false;
                }
                printf("  ... tick %u / %u\n", tick, kSoakTicks);
            }

            if (tick >= kSoakTicks) {
                printf("[PASS] T7: Soak test completed\n");
                return false;
            }
            return true;
        }
    };

}
