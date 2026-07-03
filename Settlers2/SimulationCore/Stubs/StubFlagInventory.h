#pragma once
#include "../Interfaces/IFlagInventory.h"

namespace World {

    class StubFlagInventory : public IFlagInventory {
    public:
        StubFlagInventory() {}
        virtual bool ReceiveDelivery(FlagId flagId, ResourceType type, uint8_t amount, uint32_t cargoId)
        {
            (void)flagId;
            (void)type;
            (void)amount;
            (void)cargoId;
            return false;
        }
    };

} // namespace World
