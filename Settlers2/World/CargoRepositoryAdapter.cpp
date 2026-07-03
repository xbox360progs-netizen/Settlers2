#include "stdafx.h"
#include "CargoRepositoryAdapter.h"
#include "CargoManager.h"
#include "Cargo.h"

namespace World {

    CargoRepositoryAdapter::CargoRepositoryAdapter(CargoManager& cargo)
        : m_cargo(cargo)
    {
    }

    void CargoRepositoryAdapter::Release(uint32_t cargoId)
    {
        Cargo* cargo = m_cargo.GetById(cargoId);
        if (cargo) {
            cargo->ownerTask = NULL;
        }
        m_cargo.Release(cargoId);
    }

}
