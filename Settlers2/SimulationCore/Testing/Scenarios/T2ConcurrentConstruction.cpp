#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/SimulationAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationState.h"
#include "../../World/WorldModel.h"
#include "../../Construction/ConstructionSystem.h"
#include "../../Worker/WorkerSystem.h"
#include <stdio.h>

namespace World {

    class T2ConcurrentConstruction : public ISimulationScenario {
    public:
        const char* GetName() const { return "T2"; }

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
                req2.type = BuildingType_Sawmill;
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
            if (!Assert::AllInvariants(sim.GetWorld(), sim.GetState().tickCount)) {
                printf("[FAIL] T2 failed at tick %u\n", sim.GetState().tickCount);
                return false;
            }

            if (sim.GetState().tickCount >= 200) {
                printf("[PASS] T2: Two concurrent sites\n");
                return false;
            }
            return true;
        }
    };

}
