#pragma once
#include "../Interfaces/ICargoRepository.h"

namespace World {

    class StubCargoRepository : public ICargoRepository {
    public:
        StubCargoRepository() {}
        virtual void Release(uint32_t cargoId)
        {
            (void)cargoId;
        }
    };

} // namespace World
