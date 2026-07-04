#include <stddef.h>
#include "ConstructionSystem.h"
#include "../World/WorldModel.h"
#include "../Definitions/BuildingDefinition.h"
#include "../Definitions/ProductionDefinition.h"
#include "../Systems/DemandManager.h"
#include "../Systems/JobManager.h"
#include "../Core/JobTypes.h"
#include "../Core/BuildingTypes.h"

namespace World {

    ConstructionSystem::ConstructionSystem()
        : m_tickCount(0)
        , m_demandManager(NULL)
        , m_jobManager(NULL)
    {
    }

    ConstructionSystem::~ConstructionSystem()
    {
    }

    void ConstructionSystem::Tick(WorldModel& world)
    {
        ++m_tickCount;

        ProcessDeliveryEvents(world);
        ProcessJobEvents(world);
        GenerateRequests(world);
        ProcessRequests(world);
        UpdateSites(world, m_tickCount);
        CompleteSites(world);
        RequestResources(world);
    }

    void ConstructionSystem::ProcessJobEvents(WorldModel& world)
    {
        for (int i = 0; i < world.jobEventCount; ++i) {
            const JobEvent& ev = world.jobEvents[i];
            if (ev.type != JET_Completed) continue;
            if (ev.jobType != JobType_Construction) continue;
            if (m_jobManager == NULL) continue;

            const Job& job = m_jobManager->GetJob(ev.jobId);
            if (job.state != JobState_Completed) continue;

            // Convert JobEvent to ConstructionRequest
            if (world.pendingConstructionCount >= kMaxConstructionRequests) break;

            ConstructionRequest& req = world.pendingConstructionRequests[world.pendingConstructionCount++];
            req.type = static_cast<BuildingType>(job.buildingIndex);
            req.position = Vector2i(static_cast<int>(job.targetFlag), static_cast<int>(job.targetFlag));
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }
    }

    void ConstructionSystem::GenerateRequests(WorldModel& world)
    {
        if ((m_tickCount % 100) != 0) return;
        if (world.pendingConstructionCount >= kMaxConstructionRequests) return;

        ConstructionRequest& req = world.pendingConstructionRequests[world.pendingConstructionCount++];
        req.type = BuildingType_Sawmill;
        req.position = Vector2i(10 + (m_tickCount % 50), 10);
        req.owner = 0;
        req.priority = 1;
        req.fulfilled = false;
    }

    void ConstructionSystem::ProcessRequests(WorldModel& world)
    {
        for (int i = 0; i < world.pendingConstructionCount; ++i) {
            ConstructionRequest& req = world.pendingConstructionRequests[i];
            if (req.fulfilled) continue;
            if (world.activeSiteCount >= kMaxConstructionSites) break;

            ConstructionSite& site = world.activeSites[world.activeSiteCount++];
            site.type = req.type;
            site.position = req.position;
            site.owner = req.owner;
            site.state = CS_Pending;

            req.fulfilled = true;
        }
    }

    void ConstructionSystem::UpdateSites(WorldModel& world, uint32_t currentTick)
    {
        for (int i = 0; i < world.activeSiteCount; ++i) {
            ConstructionSite& site = world.activeSites[i];
            switch (site.state) {
                case CS_Pending:
                    site.state = CS_WaitingForResources;
                    InitializeSiteResources(site);
                    site.lastStateChangeTick = currentTick;
                    break;
                case CS_WaitingForResources: {
                    bool allDelivered = true;
                    for (int r = 0; r < site.resourceCount; ++r) {
                        if (site.resources[r].delivered < site.resources[r].required) {
                            allDelivered = false;
                            break;
                        }
                    }
                    if (allDelivered) {
                        site.state = CS_Building;
                        site.lastStateChangeTick = currentTick;
                    }
                    break;
                }
                case CS_Building:
                    site.builderAssigned = true;
                    site.progress++;
                    if (site.progress >= site.requiredProgress) {
                        site.state = CS_Completed;
                        site.lastStateChangeTick = currentTick;
                    }
                    break;
                case CS_Completed:
                    break;
            }
        }
    }

    void ConstructionSystem::CompleteSites(WorldModel& world)
    {
        for (int i = world.activeSiteCount - 1; i >= 0; --i) {
            ConstructionSite& site = world.activeSites[i];
            if (site.state != CS_Completed) continue;
            if (world.productionBuildingCount >= kMaxProductionBuildings) continue;

            const BuildingDefinition& bldDef = GetBuildingDefinition(site.type);
            int idx = world.productionBuildingCount;
            ProductionBuilding& pb = world.productionBuildings[idx];
            pb.type = site.type;
            pb.position = site.position;
            pb.owner = site.owner;
            pb.cycleTimer = 0;
            pb.active = true;
            pb.inputsRequested = false;

            if (bldDef.production != PT_None) {
                const ProductionDefinition& prodDef = GetProductionDefinition(bldDef.production);
                for (int c = 0; c < kMaxProductionInputs; ++c) {
                    pb.inputResources[c] = prodDef.consumes[c].resource;
                    pb.inputRequired[c] = prodDef.consumes[c].amount;
                    pb.inputDelivered[c] = 0;
                    pb.outputResources[c] = prodDef.produces[c].resource;
                    pb.outputBuffer[c] = 0;
                    pb.totalOutput[c] = 0;
                }
            }

            world.productionBuildingCount++;

            // Remove site from activeSites by compacting
            for (int j = i; j < world.activeSiteCount - 1; ++j) {
                world.activeSites[j] = world.activeSites[j + 1];
            }
            world.activeSiteCount--;
        }
    }

    void ConstructionSystem::ProcessDeliveryEvents(WorldModel& world)
    {
        for (int i = 0; i < world.deliveryEventCount; ++i) {
            const DeliveryEvent& ev = world.deliveryEvents[i];
            if (ev.type != DET_Completed) continue;
            if (ev.reason != TTR_Construction) continue;

            for (int s = 0; s < world.activeSiteCount; ++s) {
                ConstructionSite& site = world.activeSites[s];
                if (site.state != CS_WaitingForResources) continue;

                for (int r = 0; r < site.resourceCount; ++r) {
                    BuildResourceSlot& slot = site.resources[r];
                    if (slot.resource != ev.resource) continue;
                    if (slot.delivered >= slot.required) continue;

                    slot.delivered++;
                    slot.requested = false;
                    goto nextEvent;
                }
            }
        nextEvent:;
        }
    }

    void ConstructionSystem::InitializeSiteResources(ConstructionSite& site)
    {
        const BuildingDefinition& def = GetBuildingDefinition(site.type);
        site.resourceCount = 0;
        for (int i = 0; i < kMaxBuildResources; ++i) {
            site.resources[i] = def.buildCost[i];
            if (def.buildCost[i].resource != ResourceType_None) {
                site.resourceCount++;
            }
        }
        site.requiredProgress = def.buildTime;
    }

    void ConstructionSystem::RequestResources(WorldModel& world)
    {
        if (!m_demandManager) return;

        for (int i = 0; i < world.activeSiteCount; ++i) {
            ConstructionSite& site = world.activeSites[i];
            if (site.state != CS_WaitingForResources) continue;

            FlagId destFlag = 1 + i;

            for (int r = 0; r < site.resourceCount; ++r) {
                BuildResourceSlot& slot = site.resources[r];
                if (slot.requested) continue;
                if (slot.delivered >= slot.required) continue;

                uint32_t need = slot.required - slot.delivered;
                m_demandManager->SetDemand(slot.resource, need, destFlag, TBP_Normal);

                slot.requested = true;
            }
        }
    }

} // namespace World
