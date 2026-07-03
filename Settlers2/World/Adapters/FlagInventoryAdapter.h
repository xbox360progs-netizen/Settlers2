#pragma once
#include "../../SimulationCore/Interfaces/IFlagInventory.h"

namespace World {

    class FlagManager;
    class CargoManager;

    class FlagInventoryAdapter : public IFlagInventory {
    public:
        FlagInventoryAdapter(CargoManager& cargo, FlagManager& flags);
        virtual bool ReceiveDelivery(FlagId flagId, ResourceType type, uint8_t amount, uint32_t cargoId);

    private:
        CargoManager& m_cargo;
        FlagManager& m_flags;
    };

} // namespace World
