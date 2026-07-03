#pragma once
#include "../SimulationCore/Interfaces/IDemandService.h"

namespace World {
    class DemandManager;

    class DemandServiceAdapter : public IDemandService {
    public:
        explicit DemandServiceAdapter(DemandManager& demand);
        void CompleteDemand(uint32_t observerTicketId) override;
    private:
        DemandManager& m_demand;
    };
}
