#pragma once
#include <stdint.h>

// Temporary DTO used only during World → SimulationCore migration.
// Remove after LegacyFlagSource is deleted.

namespace Scene {

struct FlagView {
    int nodeX;
    int nodeY;

    // Resource inventory (stacks visible on flag)
    static const int kMaxSlots = 8;
    uint8_t slotTypes[kMaxSlots];
    int     slotAmounts[kMaxSlots];
    int     slotCount;

    FlagView()
        : nodeX(0)
        , nodeY(0)
        , slotCount(0)
    {
        for (int i = 0; i < kMaxSlots; ++i) {
            slotTypes[i] = 0;
            slotAmounts[i] = 0;
        }
    }
};

} // namespace Scene
