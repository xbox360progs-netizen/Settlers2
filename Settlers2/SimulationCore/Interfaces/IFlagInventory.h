#pragma once
#include <stdint.h>
#include "../Transport/TransportTypes.h"
#include "../Core/ResourceTypes.h"

namespace World {

    class IFlagInventory {
    public:
        virtual ~IFlagInventory() {}
        virtual bool ReceiveDelivery(FlagId flagId, ResourceType type, uint8_t amount, uint32_t cargoId) = 0;
    };

} // namespace World
