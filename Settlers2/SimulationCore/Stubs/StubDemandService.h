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
        virtual void OnTaskCreated(uint32_t demandIndex, uint32_t taskId)
        {
            (void)demandIndex;
            (void)taskId;
        }
    };

} // namespace World
