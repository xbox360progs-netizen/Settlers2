#pragma once
#include "../SimulationCore/Interfaces/IFlagInventory.h"

namespace World {
    class FlagManager;
    class CargoManager;

    class FlagInventoryAdapter : public IFlagInventory {
    public:
        FlagInventoryAdapter(FlagManager& flags, CargoManager& cargo);
        bool ReceiveDelivery(FlagId flagId, ResourceType type, uint8_t amount, uint32_t cargoId) override;
    private:
        FlagManager& m_flags;
        CargoManager& m_cargo;
    };
}
