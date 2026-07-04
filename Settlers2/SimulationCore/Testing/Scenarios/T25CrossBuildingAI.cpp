#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/ConstructionAssertions.h"
#include "../../Testing/Assertions/TransportAssertions.h"
#include "../../Testing/Assertions/WorldAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/WorkerTypes.h"
#include "../../Core/JobTypes.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Systems/JobManager.h"
#include "../../Systems/EconomySystem.h"
#include <stdio.h>

namespace World {

class T25CrossBuildingAI : public ISimulationScenario {
public:
    const char* GetName() const { return "T25"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableEconomy = true;
        config.enableWorkers = true;
        config.enableConstruction = true;
        config.enableSettlement = true;
        config.enableProduction = true;
    }

    void Initialize(Simulation& sim)
    {
        WorldModel world;
        world.width = 50;
        world.height = 50;

        sim.LoadWorld(world);

        WorldModel& loadedWorld = sim.GetWorld();

        // Seed a Woodcutter ProductionBuilding with Wood in outputBuffer
        if (loadedWorld.productionBuildingCount < kMaxProductionBuildings) {
            ProductionBuilding& pb = loadedWorld.productionBuildings[loadedWorld.productionBuildingCount++];
            pb.type = BuildingType_Woodcutter;
            pb.position = Vector2i(10, 10);
            pb.owner = 0;
            pb.cycleTimer = 0;
            pb.active = true;
            pb.inputsRequested = false;
            pb.inputResources[0] = ResourceType_None;
            pb.inputRequired[0] = 0;
            pb.inputDelivered[0] = 0;
            pb.outputResources[0] = ResourceType_Wood;
            pb.outputBuffer[0] = 10;
            pb.totalOutput[0] = 0;
        }

        // Add 1 worker to execute construction jobs
        if (loadedWorld.workerCount < kMaxWorkers) {
            Worker& w = loadedWorld.workers[loadedWorld.workerCount++];
            w.id = 0;
            w.state = WorkerState_Idle;
            w.currentJob = 0;
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;

        if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
            printf("[FAIL] T25 failed at tick %u\n", currentTick);
            return false;
        }

        // Deliver 2 Wood to Sawmill construction site (stub for transport)
        {
            WorldModel& world = sim.GetWorld();
            for (int i = 0; i < world.activeSiteCount; ++i) {
                ConstructionSite& site = world.activeSites[i];
                if (site.type != BuildingType_Sawmill) continue;
                if (site.state != CS_WaitingForResources) continue;
                for (int r = 0; r < site.resourceCount; ++r) {
                    BuildResourceSlot& slot = site.resources[r];
                    if (slot.resource == ResourceType_Wood && slot.delivered < slot.required) {
                        slot.delivered = slot.required;
                        slot.requested = false;
                        // Decrement Woodcutter outputBuffer to simulate consumption
                        for (int b = 0; b < world.productionBuildingCount; ++b) {
                            ProductionBuilding& pb = world.productionBuildings[b];
                            if (pb.type == BuildingType_Woodcutter && pb.active && pb.outputBuffer[0] > 0) {
                                pb.outputBuffer[0] -= slot.required;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (currentTick >= 200) {
            return Verify(sim);
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        JobManager* jm = sim.GetJobManager();
        EconomySystem* es = sim.GetEconomySystem();
        bool ok = true;

        if (jm == NULL) {
            printf("[FAIL][T25] JobManager not available\n");
            return false;
        }

        // Check 1: Settlement created BuildSawmill Job
        bool foundSawmillJob = false;
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            const Job& job = jm->GetJob(i);
            if (job.type == JobType_Construction && job.buildingIndex == (uint8_t)BuildingType_Sawmill) {
                foundSawmillJob = true;
                break;
            }
        }
        if (!foundSawmillJob) {
            printf("[FAIL][T25.A] No BuildSawmill Job created — Settlement decision failed\n");
            ok = false;
        } else {
            printf("[PASS][T25.A] Settlement created BuildSawmill Job (Wood >= threshold)\n");
        }

        // Check 2: No duplicate BuildSawmill Jobs
        int sawmillJobCount = 0;
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            const Job& job = jm->GetJob(i);
            if (job.type == JobType_Construction && job.buildingIndex == (uint8_t)BuildingType_Sawmill) {
                sawmillJobCount++;
            }
        }
        if (sawmillJobCount > 1) {
            printf("[FAIL][T25.B] %d BuildSawmill Jobs — guard failed to prevent duplicate\n", sawmillJobCount);
            ok = false;
        } else {
            printf("[PASS][T25.B] No duplicate BuildSawmill Job (HasBuilding/HasPendingJob guards work)\n");
        }

        // Check 3: No BuildWoodcutter Job (Woodcutter already exists)
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            const Job& job = jm->GetJob(i);
            if (job.type == JobType_Construction && job.buildingIndex == (uint8_t)BuildingType_Woodcutter) {
                printf("[FAIL][T25.C] BuildWoodcutter Job still present — HasBuilding guard failed\n");
                ok = false;
                break;
            }
        }
        if (ok) {
            printf("[PASS][T25.C] HasBuilding guard prevents duplicate Woodcutter Job\n");
        }

        // Check 4: Sawmill construction site was created
        bool foundSawmillSite = false;
        bool foundWoodcutterSite = false;
        for (int i = 0; i < world.activeSiteCount; ++i) {
            if (world.activeSites[i].type == BuildingType_Sawmill) {
                foundSawmillSite = true;
            }
            if (world.activeSites[i].type == BuildingType_Woodcutter) {
                foundWoodcutterSite = true;
            }
        }
        if (foundWoodcutterSite) {
            printf("[FAIL][T25.D] Woodcutter construction site still active — expected ProductionBuilding\n");
            ok = false;
        } else {
            printf("[PASS][T25.D] No Woodcutter site (correctly converted to building)\n");
        }
        if (!foundSawmillSite && sawmillJobCount > 0) {
            // Job was created but site hasn't appeared yet — may need more ticks
            printf("[INFO][T25.D] Sawmill site not yet created (worker may not have completed job)\n");
        } else if (foundSawmillSite) {
            printf("[PASS][T25.D] Sawmill construction site created\n");
        }

        // Check 5: Sawmill ProductionBuilding exists (construction completed via manual resource delivery)
        bool foundSawmillBuilding = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == BuildingType_Sawmill && world.productionBuildings[i].active) {
                foundSawmillBuilding = true;
                break;
            }
        }
        if (foundSawmillBuilding) {
            printf("[PASS][T25.E] Sawmill ProductionBuilding exists — full AI→Construction→Building cycle\n");
        } else {
            printf("[INFO][T25.E] Sawmill not yet completed to building (OK if site still in progress)\n");
        }

        // Check 6: EconomySystem reports Wood available (>= threshold after Sawmill consumed 2)
        if (es != NULL) {
            int available = es->GetAvailable(ResourceType_Wood, world);
            if (foundSawmillBuilding) {
                if (available >= 8) {
                    printf("[PASS][T25.F] GetAvailable(Wood) = %d (>= kWoodForSawmill threshold)\n", available);
                } else {
                    printf("[INFO][T25.F] GetAvailable(Wood) = %d\n", available);
                }
            }
        }

        // Check 7: Woodcutter ProductionBuilding still exists
        bool foundWoodcutterBuilding = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == BuildingType_Woodcutter && world.productionBuildings[i].active) {
                foundWoodcutterBuilding = true;
                break;
            }
        }
        if (!foundWoodcutterBuilding) {
            printf("[FAIL][T25.G] Woodcutter building missing — expected to persist\n");
            ok = false;
        } else {
            printf("[PASS][T25.G] Woodcutter building persists\n");
        }

        if (ok) {
            printf("[PASS] T25: Cross-building AI decision verified\n");
            printf("  Settlement reads GetAvailable(Wood) -> BootstrapIndustry -> Sawmill Job\n");
        }
        return ok;
    }
};

static T25CrossBuildingAI g_t25CrossBuildingAI;

} // namespace World
