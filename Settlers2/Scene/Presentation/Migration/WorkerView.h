#pragma once
#include <stdint.h>

// Temporary DTO used only during World → SimulationCore migration.
// Remove after LegacyWorkerSource is deleted.
//
// Worker visualization will be redesigned with a position-computing model
// (building-based positioning, not legacy movement path).
// This view reflects the current minimal needs for migration.

namespace Scene {

struct WorkerView {
    uint8_t workerId;
    uint8_t state;       // WorkerState value
    uint8_t currentJob;

    WorkerView()
        : workerId(0)
        , state(0)
        , currentJob(0)
    {
    }
};

} // namespace Scene
