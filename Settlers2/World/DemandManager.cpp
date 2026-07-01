#include "stdafx.h"
#include <assert.h>
#include "DemandManager.h"
#include "FlagManager.h"
#include "Flag.h"
#include "TransportController.h"

namespace World {

    DemandManager::DemandManager()
        : m_freeCount(MAX_TICKETS)
        , m_nextTicketId(1)
        , m_controller(NULL)
        , m_flagManager(NULL)
    {
        for (int i = 0; i < MAX_TICKETS; ++i) {
            m_freeSlots[i] = i;
            m_inUse[i] = false;
        }
    }

    int DemandManager::AllocSlot()
    {
        if (m_freeCount <= 0) {
            assert(!"DemandTicket pool exhausted");
            return -1;
        }
        int idx = m_freeSlots[--m_freeCount];
        m_inUse[idx] = true;
        return idx;
    }

    void DemandManager::FreeSlot(int index)
    {
        if (index < 0 || index >= MAX_TICKETS) return;
        if (!m_inUse[index]) return;
        m_pool[index] = DemandTicket();
        m_inUse[index] = false;
        m_freeSlots[m_freeCount++] = index;
    }

    void DemandManager::SetDemand(ResourceType type, uint32_t amount, Handle<Flag> targetFlag, int priority)
    {
        Demand* d = FindDemand(type, targetFlag);
        if (d) {
            d->requested = amount;
            d->priority = priority;
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Demand] Update type=%s amount=%u flag=%u priority=%d\n",
                ResourceTypeToString(type), amount, targetFlag.index, priority);
            OutputDebugStringA(buf);
            return;
        }

        Demand nd;
        nd.type = type;
        nd.requested = amount;
        nd.reserved = 0;
        nd.delivered = 0;
        nd.targetFlag = targetFlag;
        nd.priority = priority;
        m_demands.push_back(nd);

        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Demand] Set type=%s amount=%u flag=%u priority=%d\n",
            ResourceTypeToString(type), amount, targetFlag.index, priority);
        OutputDebugStringA(buf);
    }

    void DemandManager::ClearDemand(Handle<Flag> targetFlag)
    {
        // Cancel all active tickets for this demand
        for (int i = 0; i < MAX_TICKETS; ++i) {
            if (m_inUse[i] &&
                m_pool[i].demand &&
                m_pool[i].demand->targetFlag.index == targetFlag.index &&
                m_pool[i].state == Ticket_Active)
            {
                m_pool[i].state = Ticket_Cancelled;
                if (m_pool[i].demand->reserved > 0)
                    m_pool[i].demand->reserved--;
                m_pool[i].demand = NULL;

                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Demand] Cancel ticket=%u flag=%u\n",
                    m_pool[i].id, targetFlag.index);
                OutputDebugStringA(buf);
            }
        }

        // Remove the demand entry
        for (std::list<Demand>::iterator it = m_demands.begin(); it != m_demands.end(); ++it) {
            if (it->targetFlag.index == targetFlag.index) {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Demand] Clear type=%s flag=%u (delivered=%u/%u reserved=%u)\n",
                    ResourceTypeToString(it->type),
                    targetFlag.index,
                    it->delivered, it->requested,
                    it->reserved);
                OutputDebugStringA(buf);

                m_demands.erase(it);
                return;
            }
        }
    }

    void DemandManager::ClearDemand(ResourceType type, Handle<Flag> targetFlag)
    {
        // Cancel active tickets for this specific type+flag
        for (int i = 0; i < MAX_TICKETS; ++i) {
            if (m_inUse[i] &&
                m_pool[i].demand &&
                m_pool[i].demand->type == type &&
                m_pool[i].demand->targetFlag.index == targetFlag.index &&
                m_pool[i].state == Ticket_Active)
            {
                m_pool[i].state = Ticket_Cancelled;
                if (m_pool[i].demand->reserved > 0)
                    m_pool[i].demand->reserved--;
                m_pool[i].demand = NULL;
            }
        }

        for (std::list<Demand>::iterator it = m_demands.begin(); it != m_demands.end(); ++it) {
            if (it->type == type && it->targetFlag.index == targetFlag.index) {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Demand] Clear type=%s flag=%u (delivered=%u/%u reserved=%u)\n",
                    ResourceTypeToString(type), targetFlag.index,
                    it->delivered, it->requested,
                    it->reserved);
                OutputDebugStringA(buf);
                m_demands.erase(it);
                return;
            }
        }
    }

    // Phase 8.1 — map resource type to transport task reason
    TransportTaskReason DemandManager::ReasonForResource(ResourceType type) const
    {
        switch (type) {
            case ResourceType_Fish:
            case ResourceType_Meat:
            case ResourceType_Wheat:
            case ResourceType_Flour:
            case ResourceType_Bread:
            case ResourceType_Water:
                return TTR_Food;
            case ResourceType_Wood:
            case ResourceType_RealWood:
            case ResourceType_ExoticWood:
            case ResourceType_Planks:
            case ResourceType_Stone:
            case ResourceType_Marble:
            case ResourceType_Granite:
                return TTR_Construction;
            case ResourceType_Tools:
                return TTR_Military;
            case ResourceType_Coal:
            case ResourceType_IronOre:
            case ResourceType_IronBar:
            case ResourceType_GoldOre:
            case ResourceType_GoldBar:
            case ResourceType_BronzeOre:
            case ResourceType_BronzeBar:
            case ResourceType_Titanium:
            case ResourceType_Salpeter:
                return TTR_Production;
            default:
                return TTR_WarehouseBalance;
        }
    }

    DemandTicket* DemandManager::Reserve(ResourceType type, FlagId originFlag)
    {
        Demand* best = NULL;
        // Phase 8.2 — if origin is known, skip demands targeting the same flag (no-op)
        if (originFlag > 0) {
            for (std::list<Demand>::iterator it = m_demands.begin(); it != m_demands.end(); ++it) {
                if (it->type != type) continue;
                if (it->reserved >= it->requested) continue;
                FlagId targetId = 0;
                if (m_flagManager) {
                    Flag* f = m_flagManager->ResolveFlag(it->targetFlag);
                    if (f) targetId = f->id;
                }
                if (targetId == originFlag) continue;
                if (!best || it->priority > best->priority)
                    best = &*it;
            }
        } else {
            best = FindBestDemand(type);
        }
        if (!best) {
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Demand] Reserve(%s) FAILED: no matching demand (m_demands.size=%u)\n",
                ResourceTypeToString(type), (uint32_t)m_demands.size());
            OutputDebugStringA(buf);
            return NULL;
        }
        if (best->reserved >= best->requested) return NULL;

        int idx = AllocSlot();
        if (idx < 0) return NULL;

        DemandTicket* t = &m_pool[idx];
        t->id = m_nextTicketId++;
        t->type = type;
        t->demand = best;
        t->state = Ticket_Active;

        best->reserved++;

        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Demand] Reserve ticket=%u type=%s flag=%u reserved=%u/%u\n",
            t->id, ResourceTypeToString(type),
            best->targetFlag.index, best->reserved, best->requested);
        OutputDebugStringA(buf);

        // Phase 8.1 — bridge: create TransportTask alongside DemandTicket
        if (m_controller && originFlag > 0) {
            // Resolve the handle to a flag ID (handle.index ≠ flagId!)
            FlagId destFlagId = 0;
            if (m_flagManager) {
                Flag* f = m_flagManager->ResolveFlag(best->targetFlag);
                if (f) destFlagId = f->id;
            }
            if (destFlagId > 0) {
                TransportTaskReason reason = ReasonForResource(type);
                TransportTask* task = m_controller->CreateTask(type, originFlag, destFlagId, reason);
                if (task) {
                    t->transportTaskId = task->id;
                    task->observerTicketId = t->id;         // Phase 8.2 — bidirectional link
                    char dbg[256];
                    _snprintf(dbg, sizeof(dbg),
                        "[Adapter] demand=%u ticket=%u type=%s flag=%u destFlagId=%u -> task=%u\n",
                        best->targetFlag.index, t->id,
                        ResourceTypeToString(type), originFlag, destFlagId, task->id);
                    OutputDebugStringA(dbg);
                }
            } else {
                char dbg[256];
                _snprintf(dbg, sizeof(dbg),
                    "[Adapter] FAILED: cannot resolve flag for demand=%u\n",
                    best->targetFlag.index);
                OutputDebugStringA(dbg);
            }
        }

        return t;
    }

    void DemandManager::ReleaseTicket(DemandTicket* ticket)
    {
        if (!ticket) return;

        if (ticket->state == Ticket_Active && ticket->demand && ticket->demand->reserved > 0)
            ticket->demand->reserved--;

        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Demand] Release ticket=%u\n", ticket->id);
        OutputDebugStringA(buf);

        int idx = (int)(ticket - m_pool);
        assert(idx >= 0 && idx < MAX_TICKETS);
        assert(m_inUse[idx] && "Double-free or invalid ticket in ReleaseTicket");
        FreeSlot(idx);
    }

    void DemandManager::Deliver(DemandTicket* ticket)
    {
        if (!ticket) return;
        if (ticket->state != Ticket_Active) {
            ReleaseTicket(ticket);
            return;
        }
        if (!ticket->demand) {
            ReleaseTicket(ticket);
            return;
        }

        ticket->state = Ticket_Delivered;
        ticket->demand->delivered++;

        if (ticket->demand->reserved > 0)
            ticket->demand->reserved--;

        if (ticket->demand->delivered > ticket->demand->requested) {
            char buf[256];
            _snprintf(buf, sizeof(buf),
                "[Demand] OVERDELIVER ticket=%u type=%s delivered=%u/%u\n",
                ticket->id, ResourceTypeToString(ticket->type),
                ticket->demand->delivered, ticket->demand->requested);
            OutputDebugStringA(buf);
        }

        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Demand] Deliver ticket=%u type=%s delivered=%u/%u\n",
            ticket->id, ResourceTypeToString(ticket->type),
            ticket->demand->delivered, ticket->demand->requested);
        OutputDebugStringA(buf);

        ReleaseTicket(ticket);
    }

    Demand* DemandManager::FindDemand(Handle<Flag> targetFlag)
    {
        for (std::list<Demand>::iterator it = m_demands.begin(); it != m_demands.end(); ++it) {
            if (it->targetFlag.index == targetFlag.index)
                return &*it;
        }
        return NULL;
    }

    Demand* DemandManager::FindDemand(ResourceType type, Handle<Flag> targetFlag)
    {
        for (std::list<Demand>::iterator it = m_demands.begin(); it != m_demands.end(); ++it) {
            if (it->type == type && it->targetFlag.index == targetFlag.index)
                return &*it;
        }
        return NULL;
    }

    Demand* DemandManager::FindBestDemand(ResourceType type)
    {
        Demand* best = NULL;
        for (std::list<Demand>::iterator it = m_demands.begin(); it != m_demands.end(); ++it) {
            if (it->type != type) continue;
            if (it->reserved >= it->requested) continue;
            if (!best || it->priority > best->priority)
                best = &*it;
        }
        return best;
    }

    bool DemandManager::HasDemand(ResourceType type)
    {
        for (std::list<Demand>::iterator it = m_demands.begin(); it != m_demands.end(); ++it) {
            if (it->type == type && it->reserved < it->requested)
                return true;
        }
        return false;
    }

    bool DemandManager::HasDemandFromOtherFlag(ResourceType type, Handle<Flag> currentFlag)
    {
        for (std::list<Demand>::iterator it = m_demands.begin(); it != m_demands.end(); ++it) {
            if (it->type != type) continue;
            if (it->reserved >= it->requested) continue;
            if (it->targetFlag.index == currentFlag.index) continue;
            return true;
        }
        return false;
    }

    void DemandManager::Clear()
    {
        m_freeCount = 0;
        for (int i = 0; i < MAX_TICKETS; ++i) {
            m_pool[i] = DemandTicket();
            m_inUse[i] = false;
            m_freeSlots[m_freeCount++] = i;
        }
        m_demands.clear();

        assert(m_freeCount == MAX_TICKETS);
    }

}
