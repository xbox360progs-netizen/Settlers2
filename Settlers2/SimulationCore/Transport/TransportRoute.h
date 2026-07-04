#pragma once
#include <stdint.h>
#include "TransportTypes.h"

#ifndef SIMCORE_TRANSPORT_ROUTE_H_
#define SIMCORE_TRANSPORT_ROUTE_H_

namespace World {

    static const int kMaxRouteLength = 64;

    struct TransportRoute {
        uint8_t count;
        FlagId flags[kMaxRouteLength];
    };

} // namespace World

#endif // SIMCORE_TRANSPORT_ROUTE_H_
