#include "stdafx.h"
#include "DemandServiceAdapter.h"
#include "../DemandManager.h"

namespace World {

    DemandServiceAdapter::DemandServiceAdapter(DemandManager& demand)
        : m_demand(demand)
    {
    }

    void DemandServiceAdapter::CompleteDemand(uint32_t observerTicketId)
    {
        DemandTicket* ticket = m_demand.GetTicket(observerTicketId);
        if (ticket)
            m_demand.Deliver(ticket);
    }

} // namespace World
