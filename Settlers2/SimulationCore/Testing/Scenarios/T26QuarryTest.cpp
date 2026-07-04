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

class T26QuarryTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T26"; }

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
            printf("[FAIL] T26 failed at tick %u\n", currentTick);
            return false;
        }

        // Deliver 2 Wood to Sawmill and Stonemason construction sites (stub for transport)
        {
            WorldModel& world = sim.GetWorld();
            for (int i = 0; i < world.activeSiteCount; ++i) {
                ConstructionSite& site = world.activeSites[i];
                if (site.state != CS_WaitingForResources) continue;

                int woodNeeded = 0;
                for (int r = 0; r < site.resourceCount; ++r) {
                    BuildResourceSlot& slot = site.resources[r];
                    if (slot.resource == ResourceType_Wood && slot.delivered < slot.required) {
                        woodNeeded += slot.required - slot.delivered;
                    }
                }
                if (woodNeeded == 0) continue;

                // Find Woodcutter to consume the resources
                int woodcutterIdx = -1;
                for (int b = 0; b < world.productionBuildingCount; ++b) {
                    if (world.productionBuildings[b].type == BuildingType_Woodcutter && world.productionBuildings[b].active) {
                        woodcutterIdx = b;
                        break;
                    }
                }
                if (woodcutterIdx < 0) continue;

                ProductionBuilding& wc = world.productionBuildings[woodcutterIdx];
                if (wc.outputBuffer[0] < woodNeeded) continue;

                // Deliver Wood to site and decrement Woodcutter outputBuffer
                for (int r = 0; r < site.resourceCount; ++r) {
                    BuildResourceSlot& slot = site.resources[r];
                    if (slot.resource == ResourceType_Wood && slot.delivered < slot.required) {
                        int deliverAmt = slot.required - slot.delivered;
                        slot.delivered += deliverAmt;
                        slot.requested = false;
                        wc.outputBuffer[0] -= deliverAmt;
                    }
                }
            }
        }

        if (currentTick >= 300) {
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
            printf("[FAIL][T26] JobManager not available\n");
            return false;
        }

        // Check 1: Woodcutter exists and is active
        bool foundWoodcutter = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == BuildingType_Woodcutter && world.productionBuildings[i].active) {
                foundWoodcutter = true;
                break;
            }
        }
        if (!foundWoodcutter) {
            printf("[FAIL][T26.A] Woodcutter missing\n");
            ok = false;
        } else {
            printf("[PASS][T26.A] Woodcutter active\n");
        }

        // Check 2: Sawmill exists and is active
        bool foundSawmill = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == BuildingType_Sawmill && world.productionBuildings[i].active) {
                foundSawmill = true;
                break;
            }
        }
        if (!foundSawmill) {
            printf("[FAIL][T26.B] Sawmill not built — BootstrapIndustry failed\n");
            ok = false;
        } else {
            printf("[PASS][T26.B] Sawmill exists\n");
        }

        // Check 3: Stonemason exists and is active
        bool foundStonemason = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == BuildingType_Stonemason && world.productionBuildings[i].active) {
                foundStonemason = true;
                break;
            }
        }
        if (!foundStonemason) {
            printf("[FAIL][T26.C] Stonemason not built — BootstrapMining failed\n");
            ok = false;
        } else {
            printf("[PASS][T26.C] Stonemason exists\n");
        }

        // Check 4: No duplicate jobs for any building type
        int woodcutterJobCount = 0;
        int sawmillJobCount = 0;
        int stonemasonJobCount = 0;
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            const Job& job = jm->GetJob(i);
            if (job.state == JobState_Completed) continue;
            if (job.type != JobType_Construction) continue;
            if (job.buildingIndex == (uint8_t)BuildingType_Woodcutter) woodcutterJobCount++;
            if (job.buildingIndex == (uint8_t)BuildingType_Sawmill) sawmillJobCount++;
            if (job.buildingIndex == (uint8_t)BuildingType_Stonemason) stonemasonJobCount++;
        }

        bool noDuplicates = true;
        if (woodcutterJobCount > 1) {
            printf("[FAIL][T26.D] %d pending BuildWoodcutter jobs\n", woodcutterJobCount);
            noDuplicates = false;
        }
        if (sawmillJobCount > 1) {
            printf("[FAIL][T26.D] %d pending BuildSawmill jobs\n", sawmillJobCount);
            noDuplicates = false;
        }
        if (stonemasonJobCount > 1) {
            printf("[FAIL][T26.D] %d pending BuildStonemason jobs\n", stonemasonJobCount);
            noDuplicates = false;
        }
        if (noDuplicates) {
            printf("[PASS][T26.D] No duplicate jobs — all three guards work\n");
        } else {
            ok = false;
        }

        // Check 5: No active construction sites remain
        if (world.activeSiteCount != 0) {
            printf("[FAIL][T26.E] %d active sites remain (expected 0)\n", world.activeSiteCount);
            ok = false;
        } else {
            printf("[PASS][T26.E] All sites completed — no orphan sites\n");
        }

        // Check 6: EconomySystem reports Wood and Stone
        if (es != NULL) {
            int woodAvailable = es->GetAvailable(ResourceType_Wood, world);
            int stoneAvailable = es->GetAvailable(ResourceType_Stone, world);
            printf("[INFO][T26.F] GetAvailable(Wood) = %d, GetAvailable(Stone) = %d\n",
                woodAvailable, stoneAvailable);
            if (foundStonemason && stoneAvailable > 0) {
                printf("[PASS][T26.F] Stone being produced by Stonemason\n");
            } else if (foundStonemason && stoneAvailable == 0) {
                printf("[INFO][T26.F] Stonemason exists but no Stone output yet (cycle not completed)\n");
            }
        }

        // Check 7: All three buildings exist — no rule regressed another rule
        int buildingCount = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].active) buildingCount++;
        }
        if (buildingCount >= 3) {
            printf("[PASS][T26.G] %d active buildings — all three rules fired independently\n", buildingCount);
        } else {
            printf("[FAIL][T26.G] Only %d active buildings (expected >= 3)\n", buildingCount);
            ok = false;
        }

        if (ok) {
            printf("[PASS] T26: Quarry integration — two independent resource chains verified\n");
            printf("  Logging: Woodcutter->Wood->Sawmill\n");
            printf("  Mining:  Stonemason->Stone\n");
        }
        return ok;
    }
};

static T26QuarryTest g_t26QuarryTest;

} // namespace World
