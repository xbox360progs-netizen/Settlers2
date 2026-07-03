#pragma once
#include "../SimulationCore/Interfaces/ICargoRepository.h"

namespace World {
    class CargoManager;

    class CargoRepositoryAdapter : public ICargoRepository {
    public:
        explicit CargoRepositoryAdapter(CargoManager& cargo);
        void Release(uint32_t cargoId) override;
    private:
        CargoManager& m_cargo;
    };
}
