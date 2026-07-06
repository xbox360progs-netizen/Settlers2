#include "stdafx.h"
#include "SimulationCoreWorkerSource.h"
#include "../../../SimulationCore/World/WorldModel.h"

namespace Scene {

void SimulationCoreWorkerSource::SetWorldModel(const World::WorldModel* world)
{
    m_world = world;
}

uint32_t SimulationCoreWorkerSource::GetWorkerCount() const
{
    if (!m_world) return 0;
    return static_cast<uint32_t>(m_world->workerCount);
}

bool SimulationCoreWorkerSource::GetWorker(uint32_t index, WorkerView& out) const
{
    if (!m_world || index >= static_cast<uint32_t>(m_world->workerCount))
        return false;

    const World::Worker& w = m_world->workers[index];
    out.workerId = static_cast<uint8_t>(w.id);
    out.state = static_cast<uint8_t>(w.state);
    out.currentJob = w.currentJob;
    return true;
}

} // namespace Scene
