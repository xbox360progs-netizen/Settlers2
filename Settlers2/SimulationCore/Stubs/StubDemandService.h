#pragma once
#include "../Interfaces/IDemandService.h"

namespace World {

    class StubDemandService : public IDemandService {
    public:
        StubDemandService() {}
        virtual void CompleteDemand(uint32_t observerTicketId)
        {
            (void)observerTicketId;
        }
    };

} // namespace World
