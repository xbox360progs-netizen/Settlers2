#pragma once
#include "IWorkerSource.h"

// Temporary — reads SimulationCore WorldModel for migrated worker data.
// Remove after WorldModel is the sole source and LegacyWorkerSource is deleted.
//
// SimulationCore workers have no position data (no movement path).
// BuildingWorkerPresentation computes visual positions from building assignments.
// This source exposes worker state; positions computed in Presentation.

namespace World {
    struct WorldModel;
}

namespace Scene {

class SimulationCoreWorkerSource : public IWorkerSource {
public:
    void SetWorldModel(const World::WorldModel* world);

    virtual uint32_t GetWorkerCount() const;
    virtual bool GetWorker(uint32_t index, WorkerView& out) const;

private:
    const World::WorldModel* m_world;
};

} // namespace Scene
