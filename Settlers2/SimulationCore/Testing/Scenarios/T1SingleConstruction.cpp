#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/SimulationAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationState.h"
#include "../../World/WorldModel.h"
#include "../../Construction/ConstructionSystem.h"
#include "../../Worker/WorkerSystem.h"
#include <stdio.h>

namespace World {

    class T1SingleConstruction : public ISimulationScenario {
    public:
        const char* GetName() const { return "T1"; }

        void Initialize(Simulation& sim)
        {
            WorldModel& world = sim.GetWorld();
            if (world.pendingConstructionCount < kMaxConstructionRequests) {
                ConstructionRequest& req = world.pendingConstructionRequests[world.pendingConstructionCount++];
                req.type = BuildingType_Sawmill;
                req.position = Vector2i(10, 10);
                req.owner = 0;
                req.priority = 1;
                req.fulfilled = false;
                printf("  Tick %u: Add building at (10,10)\n", sim.GetState().tickCount);
            }
        }

        bool Tick(Simulation& sim)
        {
            if (!Assert::AllInvariants(sim.GetWorld(), sim.GetState().tickCount)) {
                printf("[FAIL] T1 failed at tick %u\n", sim.GetState().tickCount);
                return false;
            }

            if (sim.GetState().tickCount >= 200) {
                printf("[PASS] T1: Single construction pipeline established\n");
                return false;
            }
            return true;
        }
    };

}
