#include "stdafx.h"
#include "FlagInventoryAdapter.h"
#include "../FlagManager.h"
#include "../CargoManager.h"
#include "../Flag.h"

namespace World {

    FlagInventoryAdapter::FlagInventoryAdapter(CargoManager& cargo, FlagManager& flags)
        : m_cargo(cargo)
        , m_flags(flags)
    {
    }

    bool FlagInventoryAdapter::ReceiveDelivery(FlagId flagId, ResourceType type, uint8_t amount, uint32_t cargoId)
    {
        Flag* flag = m_flags.GetFlagById(flagId);
        if (!flag)
            return false;

        bool added = flag->AddResource(type, amount);
        if (added && cargoId > 0)
            m_cargo.Release(cargoId);

        return added;
    }

} // namespace World
