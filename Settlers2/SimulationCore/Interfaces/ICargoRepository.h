#pragma once
#include <stdint.h>

namespace World {

    class ICargoRepository {
    public:
        virtual ~ICargoRepository() {}
        virtual void Release(uint32_t cargoId) = 0;
    };

} // namespace World
