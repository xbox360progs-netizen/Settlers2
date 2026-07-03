#include "stdafx.h"
#include "CargoRepositoryAdapter.h"
#include "../CargoManager.h"

namespace World {

    CargoRepositoryAdapter::CargoRepositoryAdapter(CargoManager& cargo)
        : m_cargo(cargo)
    {
    }

    void CargoRepositoryAdapter::Release(uint32_t cargoId)
    {
        m_cargo.Release(cargoId);
    }

} // namespace World
