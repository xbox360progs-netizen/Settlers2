#pragma once
#include <stdint.h>
#include "../Transport/TransportTypes.h"
#include "../Transport/TransportRoute.h"

namespace World {

    class IRoadGraph {
    public:
        virtual ~IRoadGraph() {}
        virtual bool FindRoute(FlagId source, FlagId destination, TransportRoute& outRoute) = 0;
    };

} // namespace World
