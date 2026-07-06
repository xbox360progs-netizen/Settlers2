#pragma once
#include <stdint.h>
#include "FlagView.h"

// Temporary abstraction used only during World → SimulationCore migration.
// Remove after LegacyFlagSource is deleted.

namespace Scene {

class IFlagSource {
public:
    virtual ~IFlagSource() {}

    virtual uint32_t GetFlagCount() const = 0;
    virtual bool GetFlag(uint32_t index, FlagView& out) const = 0;
};

} // namespace Scene
