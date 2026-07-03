#include "stdafx.h"
#include "FlagInventoryAdapter.h"
#include "FlagManager.h"
#include "CargoManager.h"
#include "Cargo.h"
#include "Flag.h"

namespace World {

    FlagInventoryAdapter::FlagInventoryAdapter(FlagManager& flags, CargoManager& cargo)
        : m_flags(flags)
        , m_cargo(cargo)
    {
    }

    bool FlagInventoryAdapter::ReceiveDelivery(FlagId flagId, ResourceType type, uint8_t amount, uint32_t cargoId)
    {
        Flag* flag = m_flags.GetFlagById(flagId);
        Cargo* cargo = m_cargo.GetById(cargoId);
        if (!flag || !cargo) return false;

        flag->AcceptCargo(cargo);
        return flag->AddResource(type, amount);
    }

}
