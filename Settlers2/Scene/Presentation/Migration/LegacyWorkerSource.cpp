#include "stdafx.h"
#include "LegacyWorkerSource.h"

// Temporary stub — workers are positioned via legacy building movement paths.
// BuildingWorkerPresentation computes positions from building assignments.
// Full implementation deferred until worker state tracking in SimulationCore.

namespace Scene {

uint32_t LegacyWorkerSource::GetWorkerCount() const
{
    return 0;
}

bool LegacyWorkerSource::GetWorker(uint32_t index, WorkerView& out) const
{
    (void)index;
    (void)out;
    return false;
}

} // namespace Scene
