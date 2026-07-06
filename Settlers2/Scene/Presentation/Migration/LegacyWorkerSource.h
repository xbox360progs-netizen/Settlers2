#pragma once
#include "IWorkerSource.h"

// Temporary — wraps legacy World managers for migration.
// Remove when all PresentationSystems read from SimulationCore.
//
// Workers in the legacy system are positioned via building movement paths.
// This source provides basic worker state; the BuildingWorkerPresentation
// rewrite will compute positions from building assignments instead.

namespace Scene {

class LegacyWorkerSource : public IWorkerSource {
public:
    virtual uint32_t GetWorkerCount() const;
    virtual bool GetWorker(uint32_t index, WorkerView& out) const;
};

} // namespace Scene
