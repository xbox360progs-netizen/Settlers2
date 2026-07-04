#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/SimulationAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationState.h"
#include "../../World/WorldModel.h"
#include "../../Construction/ConstructionSystem.h"
#include "../../Worker/WorkerSystem.h"
#include <stdio.h>

namespace World {

    class T6MassConstruction : public ISimulationScenario {
    public:
        const char* GetName() const { return "T6"; }

        void Initialize(Simulation& sim)
        {
            WorldModel& world = sim.GetWorld();
            for (int i = 0; i < 20 && world.pendingConstructionCount < kMaxConstructionRequests; ++i) {
                ConstructionRequest& req = world.pendingConstructionRequests[world.pendingConstructionCount++];
                req.type = BuildingType_Sawmill;
                req.position = Vector2i(10 + i, 10);
                req.owner = 0;
                req.priority = 1;
                req.fulfilled = false;
                printf("  Tick %u: Add building at (%d,10)\n", sim.GetState().tickCount, 10 + i);
            }
        }

        bool Tick(Simulation& sim)
        {
            uint32_t tick = sim.GetState().tickCount;

            if ((tick % 1000) == 0) {
                if (!Assert::AllInvariants(sim.GetWorld(), tick)) {
                    printf("[FAIL] T6 failed at tick %u\n", tick);
                    return false;
                }
            }

            if (tick >= 5000) {
                printf("[PASS] T6: Mass construction stable\n");
                return false;
            }
            return true;
        }
    };

}
