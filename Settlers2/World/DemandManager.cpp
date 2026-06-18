#include "stdafx.h"
#include "DemandManager.h"

namespace World {

    DemandManager::DemandManager()
        : m_nextTicketId(1)
    {
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
        // Cancel all tickets for this demand
        for (size_t ti = 0; ti < m_tickets.size(); ++ti) {
            if (m_tickets[ti]->demand &&
                m_tickets[ti]->demand->targetFlag.index == targetFlag.index &&
                m_tickets[ti]->state == Ticket_Active)
            {
                m_tickets[ti]->state = Ticket_Cancelled;
                if (m_tickets[ti]->demand->reserved > 0)
                    m_tickets[ti]->demand->reserved--;
                m_tickets[ti]->demand = NULL;

                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Demand] Cancel ticket=%u flag=%u\n",
                    m_tickets[ti]->id, targetFlag.index);
                OutputDebugStringA(buf);
            }
        }

        // Remove the demand entry
        for (size_t i = 0; i < m_demands.size(); ++i) {
            if (m_demands[i].targetFlag.index == targetFlag.index) {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Demand] Clear type=%s flag=%u (delivered=%u/%u reserved=%u)\n",
                    ResourceTypeToString(m_demands[i].type),
                    targetFlag.index,
                    m_demands[i].delivered, m_demands[i].requested,
                    m_demands[i].reserved);
                OutputDebugStringA(buf);

                m_demands.erase(m_demands.begin() + i);
                return;
            }
        }
    }

    void DemandManager::ClearDemand(ResourceType type, Handle<Flag> targetFlag)
    {
        // Cancel tickets for this specific type+flag
        for (size_t ti = 0; ti < m_tickets.size(); ++ti) {
            if (m_tickets[ti]->demand &&
                m_tickets[ti]->demand->type == type &&
                m_tickets[ti]->demand->targetFlag.index == targetFlag.index &&
                m_tickets[ti]->state == Ticket_Active)
            {
                m_tickets[ti]->state = Ticket_Cancelled;
                if (m_tickets[ti]->demand->reserved > 0)
                    m_tickets[ti]->demand->reserved--;
                m_tickets[ti]->demand = NULL;
            }
        }

        for (size_t i = 0; i < m_demands.size(); ++i) {
            if (m_demands[i].type == type && m_demands[i].targetFlag.index == targetFlag.index) {
                char buf[256];
                _snprintf(buf, sizeof(buf),
                    "[Demand] Clear type=%s flag=%u (delivered=%u/%u reserved=%u)\n",
                    ResourceTypeToString(type), targetFlag.index,
                    m_demands[i].delivered, m_demands[i].requested,
                    m_demands[i].reserved);
                OutputDebugStringA(buf);
                m_demands.erase(m_demands.begin() + i);
                return;
            }
        }
    }

    DemandTicket* DemandManager::Reserve(ResourceType type)
    {
    Demand* best = FindBestDemand(type);
    if (!best) {
        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Demand] Reserve(%s) FAILED: no matching demand (m_demands.size=%u)\n",
            ResourceTypeToString(type), (uint32_t)m_demands.size());
        OutputDebugStringA(buf);
        return NULL;
    }
        if (best->reserved >= best->requested) return NULL;

        DemandTicket* t = new DemandTicket();
        t->id = m_nextTicketId++;
        t->type = type;
        t->demand = best;
        t->state = Ticket_Active;
        m_tickets.push_back(t);

        best->reserved++;

        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Demand] Reserve ticket=%u type=%s flag=%u reserved=%u/%u\n",
            t->id, ResourceTypeToString(type),
            best->targetFlag.index, best->reserved, best->requested);
        OutputDebugStringA(buf);

        return t;
    }

    void DemandManager::ReleaseTicket(DemandTicket* ticket)
    {
        if (!ticket) return;

        if (ticket->state == Ticket_Active && ticket->demand && ticket->demand->reserved > 0)
            ticket->demand->reserved--;

        for (size_t i = 0; i < m_tickets.size(); ++i) {
            if (m_tickets[i] == ticket) {
                m_tickets.erase(m_tickets.begin() + i);
                break;
            }
        }

        char buf[256];
        _snprintf(buf, sizeof(buf),
            "[Demand] Release ticket=%u\n", ticket->id);
        OutputDebugStringA(buf);

        delete ticket;
    }

    void DemandManager::Deliver(DemandTicket* ticket)
    {
        if (!ticket) return;
        if (ticket->state != Ticket_Active) {
            // Ticket was cancelled — just release it, no delivery credit
            ReleaseTicket(ticket);
            return;
        }
        if (!ticket->demand) {
            // Demand was removed — no flag to credit, but cargo is delivered
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
        for (size_t i = 0; i < m_demands.size(); ++i) {
            if (m_demands[i].targetFlag.index == targetFlag.index)
                return &m_demands[i];
        }
        return NULL;
    }

    Demand* DemandManager::FindDemand(ResourceType type, Handle<Flag> targetFlag)
    {
        for (size_t i = 0; i < m_demands.size(); ++i) {
            if (m_demands[i].type == type && m_demands[i].targetFlag.index == targetFlag.index)
                return &m_demands[i];
        }
        return NULL;
    }

    Demand* DemandManager::FindBestDemand(ResourceType type)
    {
        Demand* best = NULL;
        for (size_t i = 0; i < m_demands.size(); ++i) {
            if (m_demands[i].type != type) continue;
            if (m_demands[i].reserved >= m_demands[i].requested) continue;
            if (!best || m_demands[i].priority > best->priority)
                best = &m_demands[i];
        }
        return best;
    }

    bool DemandManager::HasDemand(ResourceType type)
    {
        for (size_t i = 0; i < m_demands.size(); ++i) {
            if (m_demands[i].type == type && m_demands[i].reserved < m_demands[i].requested)
                return true;
        }
        return false;
    }

    void DemandManager::Clear()
    {
        for (size_t i = 0; i < m_tickets.size(); ++i)
            delete m_tickets[i];
        m_tickets.clear();
        m_demands.clear();
    }

}
