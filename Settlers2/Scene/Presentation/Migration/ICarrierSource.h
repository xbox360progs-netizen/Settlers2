#pragma once
#include <stdint.h>
#include "CarrierView.h"

// Temporary abstraction used only during World → SimulationCore migration.
// Remove after LegacyCarrierSource is deleted.

namespace Scene {

class ICarrierSource {
public:
    virtual ~ICarrierSource() {}

    virtual uint32_t GetCarrierCount() const = 0;
    virtual bool GetCarrier(uint32_t index, CarrierView& out) const = 0;
};

} // namespace Scene
