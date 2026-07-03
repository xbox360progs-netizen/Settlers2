#pragma once
#include "../../SimulationCore/Interfaces/IDemandService.h"

namespace World {

    class DemandManager;

    class DemandServiceAdapter : public IDemandService {
    public:
        explicit DemandServiceAdapter(DemandManager& demand);
        virtual void CompleteDemand(uint32_t observerTicketId);

    private:
        DemandManager& m_demand;
    };

} // namespace World
