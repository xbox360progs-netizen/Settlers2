#include "ConstructionSystem.h"
#include "../World/WorldModel.h"
#include "../Definitions/BuildingDefinition.h"

namespace World {

    ConstructionSystem::ConstructionSystem()
        : m_tickCount(0)
    {
    }

    ConstructionSystem::~ConstructionSystem()
    {
    }

    void ConstructionSystem::Tick(WorldModel& world)
    {
        ++m_tickCount;

        ProcessDeliveryEvents(world);
        GenerateRequests(world);
        ProcessRequests(world);
        UpdateSites(world);
        PublishResourceRequests(world);
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

    void ConstructionSystem::UpdateSites(WorldModel& world)
    {
        for (int i = 0; i < world.activeSiteCount; ++i) {
            ConstructionSite& site = world.activeSites[i];
            switch (site.state) {
                case CS_Pending:
                    site.state = CS_WaitingForResources;
                    InitializeSiteResources(site);
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
                    }
                    break;
                }
                case CS_Building:
                    if (site.builderAssigned) {
                        site.progress++;
                        if (site.progress >= site.requiredProgress) {
                            site.state = CS_Completed;
                        }
                    }
                    break;
                case CS_Completed:
                    break;
            }
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

    void ConstructionSystem::PublishResourceRequests(WorldModel& world)
    {
        for (int i = 0; i < world.activeSiteCount; ++i) {
            ConstructionSite& site = world.activeSites[i];
            if (site.state != CS_WaitingForResources) continue;

            for (int r = 0; r < site.resourceCount; ++r) {
                BuildResourceSlot& slot = site.resources[r];
                if (slot.requested) continue;
                if (slot.delivered >= slot.required) continue;

                if (world.pendingRequestCount >= kMaxPendingRequests) break;

                TransportRequest& req = world.pendingRequests[world.pendingRequestCount++];
                req.resource = slot.resource;
                req.origin = 0;
                req.destination = 0;
                req.reason = TTR_Construction;
                req.fulfilled = false;

                slot.requested = true;
            }
        }
    }

} // namespace World
