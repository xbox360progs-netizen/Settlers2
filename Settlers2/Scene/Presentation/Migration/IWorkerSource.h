#pragma once
#include <stdint.h>
#include "WorkerView.h"

// Temporary abstraction used only during World → SimulationCore migration.
// Remove after LegacyWorkerSource is deleted.

namespace Scene {

class IWorkerSource {
public:
    virtual ~IWorkerSource() {}

    virtual uint32_t GetWorkerCount() const = 0;
    virtual bool GetWorker(uint32_t index, WorkerView& out) const = 0;
};

} // namespace Scene
