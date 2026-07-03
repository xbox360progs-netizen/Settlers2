#pragma once
#include <stdint.h>

namespace World {

    class IDemandService {
    public:
        virtual ~IDemandService() {}
        virtual void CompleteDemand(uint32_t observerTicketId) = 0;
    };

} // namespace World
