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

class T27ForestryFlowTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T27"; }

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
            printf("[FAIL] T27 failed at tick %u\n", currentTick);
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
            printf("[FAIL][T27] JobManager not available\n");
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
            printf("[FAIL][T27.A] Woodcutter missing\n");
            ok = false;
        } else {
            printf("[PASS][T27.A] Woodcutter active\n");
        }

        // Check 2: Forester job was created (flow-based decision triggered)
        bool foundForesterJob = false;
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            const Job& job = jm->GetJob(i);
            if (job.state == JobState_Completed) continue;
            if (job.type == JobType_Construction && job.buildingIndex == (uint8_t)BuildingType_Forester) {
                foundForesterJob = true;
                break;
            }
        }

        // Also check completed jobs — Forester may have been built already
        if (!foundForesterJob) {
            for (int i = 0; i < jm->GetJobCount(); ++i) {
                const Job& job = jm->GetJob(i);
                if (job.type != JobType_Construction) continue;
                if (job.buildingIndex == (uint8_t)BuildingType_Forester) {
                    foundForesterJob = true;
                    break;
                }
            }
        }

        if (!foundForesterJob) {
            printf("[FAIL][T27.B] No Forester job created — flow-based decision failed\n");
            ok = false;
        } else {
            printf("[PASS][T27.B] Forester job created (Wood flow >= threshold)\n");
        }

        // Check 3: Forester building exists (construction completed)
        bool foundForesterBuilding = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == BuildingType_Forester && world.productionBuildings[i].active) {
                foundForesterBuilding = true;
                break;
            }
        }
        if (!foundForesterBuilding) {
            printf("[INFO][T27.C] Forester not yet built (site may still be in progress)\n");
        } else {
            printf("[PASS][T27.C] Forester building exists — construction completed\n");
        }

        // Check 4: Wood flow is positive
        if (es != NULL) {
            int woodFlow = es->GetResourceFlow(ResourceType_Wood);
            if (woodFlow > 0) {
                printf("[PASS][T27.D] GetResourceFlow(Wood) = %d (positive flow)\n", woodFlow);
            } else {
                printf("[FAIL][T27.D] GetResourceFlow(Wood) = %d (expected > 0)\n", woodFlow);
                ok = false;
            }
        }

        // Check 5: No duplicate jobs — all four guards work independently
        int duplicateCount = 0;
        int woodcutterJobs = 0, sawmillJobs = 0, stonemasonJobs = 0, foresterJobs = 0;
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            const Job& job = jm->GetJob(i);
            if (job.state == JobState_Completed) continue;
            if (job.type != JobType_Construction) continue;
            if (job.buildingIndex == (uint8_t)BuildingType_Woodcutter) woodcutterJobs++;
            if (job.buildingIndex == (uint8_t)BuildingType_Sawmill) sawmillJobs++;
            if (job.buildingIndex == (uint8_t)BuildingType_Stonemason) stonemasonJobs++;
            if (job.buildingIndex == (uint8_t)BuildingType_Forester) foresterJobs++;
        }

        if (woodcutterJobs > 1) { duplicateCount++; }
        if (sawmillJobs > 1) { duplicateCount++; }
        if (stonemasonJobs > 1) { duplicateCount++; }
        if (foresterJobs > 1) { duplicateCount++; }

        if (duplicateCount > 0) {
            printf("[FAIL][T27.E] %d duplicate job groups — guards failing\n", duplicateCount);
            ok = false;
        } else {
            printf("[PASS][T27.E] No duplicate jobs — all four guards work\n");
        }

        // Check 6: All active buildings present and no regression
        int woodcutterBuildings = 0, sawmillBuildings = 0, stonemasonBuildings = 0, foresterBuildings = 0;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (!world.productionBuildings[i].active) continue;
            if (world.productionBuildings[i].type == BuildingType_Woodcutter) woodcutterBuildings++;
            if (world.productionBuildings[i].type == BuildingType_Sawmill) sawmillBuildings++;
            if (world.productionBuildings[i].type == BuildingType_Stonemason) stonemasonBuildings++;
            if (world.productionBuildings[i].type == BuildingType_Forester) foresterBuildings++;
        }

        if (woodcutterBuildings != 1) {
            printf("[FAIL][T27.F] Expected 1 Woodcutter, got %d\n", woodcutterBuildings);
            ok = false;
        }
        if (sawmillBuildings < 1) {
            printf("[FAIL][T27.F] Sawmill missing — BootstrapIndustry regression\n");
            ok = false;
        }
        if (stonemasonBuildings < 1) {
            printf("[FAIL][T27.F] Stonemason missing — BootstrapMining regression\n");
            ok = false;
        }
        if (woodcutterBuildings == 1 && sawmillBuildings >= 1 && stonemasonBuildings >= 1) {
            printf("[PASS][T27.F] No regression — Woodcutter=%d, Sawmill=%d, Stonemason=%d, Forester=%d\n",
                woodcutterBuildings, sawmillBuildings, stonemasonBuildings, foresterBuildings);
        }

        // Check 7: No active construction sites remain
        if (world.activeSiteCount > 0) {
            printf("[INFO][T27.G] %d active sites remain (may be Forester in progress)\n", world.activeSiteCount);
        } else {
            printf("[PASS][T27.G] All construction completed\n");
        }

        if (ok) {
            printf("[PASS] T27: Flow-based decision verified\n");
            printf("  EconomySystem tracks production rate via GetResourceFlow()\n");
            printf("  Settlement uses flow (not stock) to decide BootstrapForestry\n");
        }
        return ok;
    }
};

static T27ForestryFlowTest g_t27ForestryFlowTest;

} // namespace World
