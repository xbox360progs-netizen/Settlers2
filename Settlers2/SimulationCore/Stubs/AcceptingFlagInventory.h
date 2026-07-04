#pragma once
#include "../Interfaces/IFlagInventory.h"

namespace World {

    class AcceptingFlagInventory : public IFlagInventory {
    public:
        AcceptingFlagInventory() {}
        virtual bool ReceiveDelivery(FlagId, ResourceType, uint8_t, uint32_t) {
            return true;
        }
    };

} // namespace World
