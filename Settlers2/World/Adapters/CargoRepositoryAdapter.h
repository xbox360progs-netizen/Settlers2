#pragma once
#include "../../SimulationCore/Interfaces/ICargoRepository.h"

namespace World {

    class CargoManager;

    class CargoRepositoryAdapter : public ICargoRepository {
    public:
        explicit CargoRepositoryAdapter(CargoManager& cargo);
        virtual void Release(uint32_t cargoId);

    private:
        CargoManager& m_cargo;
    };

} // namespace World
