#include <stddef.h>
#include "DemandManager.h"
#include "../World/WorldModel.h"
#include "../Transport/TransportTypes.h"

namespace World {

    DemandManager::DemandManager()
        : m_demandCount(0)
        , m_tickCount(0)
    {
    }

    DemandManager::~DemandManager()
    {
    }

    void DemandManager::SetDemand(ResourceType type, uint32_t amount, FlagId targetFlag, int priority, DemandOwner owner, TransportTaskReason reason)
    {
        for (int i = 0; i < m_demandCount; ++i) {
            if (m_demands[i].targetFlag == targetFlag && m_demands[i].type == type) {
                m_demands[i].remaining += amount;
                m_demands[i].totalRequested += amount;
                if (priority > m_demands[i].priority)
                    m_demands[i].priority = priority;
                return;
            }
        }

        if (m_demandCount < kMaxDemands) {
            Demand& d = m_demands[m_demandCount++];
            d.type = type;
            d.remaining = amount;
            d.totalRequested = amount;
            d.targetFlag = targetFlag;
            d.priority = priority;
            d.owner = owner;
            d.reason = reason;
            d.activeTask = 0;
        }
    }

    void DemandManager::ClearDemand(FlagId targetFlag)
    {
        int write = 0;
        for (int i = 0; i < m_demandCount; ++i) {
            if (m_demands[i].targetFlag != targetFlag)
                m_demands[write++] = m_demands[i];
        }
        m_demandCount = write;
    }

    void DemandManager::ClearDemand(ResourceType type, FlagId targetFlag)
    {
        int write = 0;
        for (int i = 0; i < m_demandCount; ++i) {
            if (m_demands[i].targetFlag != targetFlag || m_demands[i].type != type)
                m_demands[write++] = m_demands[i];
        }
        m_demandCount = write;
    }

    void DemandManager::Tick(WorldModel& world)
    {
        // Read pendingDemand from all TransportNodes
        // Only creates a new demand if no active demand exists for the same (type, flag).
        // This prevents duplicate demands when domain systems also call SetDemand directly.
        for (int n = 0; n < world.transportNodeCount; ++n) {
            const TransportNode& node = world.transportNodes[n];
            for (int d = 0; d < kMaxNodeDemands; ++d) {
                const DemandSlot& slot = node.pendingDemand[d];
                if (!slot.active) continue;
                if (slot.resource == ResourceType_None) continue;

                // Check if an active demand already exists for this (type, targetFlag)
                bool hasActiveDemand = false;
                for (int i = 0; i < m_demandCount; ++i) {
                    if (m_demands[i].type == slot.resource &&
                        m_demands[i].targetFlag == slot.targetFlag &&
                        m_demands[i].remaining > 0)
                    {
                        hasActiveDemand = true;
                        break;
                    }
                }

                if (!hasActiveDemand) {
                    SetDemand(slot.resource, static_cast<uint32_t>(slot.amount),
                              slot.targetFlag, TBP_Normal,
                              DemandOwner_Production, TTR_Production);
                }
            }
        }

        ++m_tickCount;
        PublishTransportRequests(world);
    }

    void DemandManager::OnTaskCreated(uint32_t demandIndex, uint32_t taskId)
    {
        if (static_cast<int>(demandIndex) >= m_demandCount)
            return;
        m_demands[demandIndex].activeTask = taskId;
    }

    void DemandManager::CompleteDemand(uint32_t observerTicketId)
    {
        if (observerTicketId == 0)
            return;
        int index = static_cast<int>(observerTicketId - 1);
        if (index < 0 || index >= m_demandCount)
            return;
        Demand& d = m_demands[index];
        if (d.remaining > 0) {
            d.remaining--;
        }
        d.activeTask = 0;
    }

    void DemandManager::PublishTransportRequests(WorldModel& world)
    {
        for (int i = 0; i < m_demandCount; ++i) {
            Demand& d = m_demands[i];

            if (d.remaining == 0)
                continue;

            if (d.activeTask != 0)
                continue;

            if (world.pendingRequestCount >= kMaxPendingRequests)
                break;

            TransportRequest& req = world.pendingRequests[world.pendingRequestCount++];
            req.resource = d.type;
            req.origin = 0;
            req.destination = d.targetFlag;
            req.reason = d.reason;
            req.owner = d.owner;
            req.fulfilled = false;
            req.demandIndex = static_cast<uint8_t>(i);
        }
    }

} // namespace World
